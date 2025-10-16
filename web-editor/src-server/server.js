import express from "express";
import * as http from "http";
import { WebSocketServer } from "ws";
import cors from "cors";
import { promises as fs } from "fs";
import { createReadStream } from 'fs';
import * as path from "path";
import {truncate} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";

let webEditorSocket = null;

// This is the entry point: starts servers
export async function run(port) {

  // Use custom data folder, if envvar present
  // if (process.env.hasOwnProperty(dataDirEnv))
  //   dataDir = process.env[dataDirEnv];
  // console.log(`Using data directory: ${dataDir}`);

  // Create app, server, web socket servers
  const app = express();
  app.use(cors());
  const server = http.createServer(app);
  const wsEditor = new WebSocketServer({ noServer: true });

  // app.get("/clips/:name", handleGetClip);

  // Upgrade connections to web socker
  server.on("upgrade", (request, socket, head) => {
    if (request.url === PROT.kEditorSocketPath) {
      wsEditor.handleUpgrade(request, socket, head, (ws) => {
        wsEditor.emit("connection", ws, request);
      });
    }
    else {
      socket.destroy(); // Close the connection for other paths
    }
  });

  // Web socket event handlers
  wsEditor.on("connection", (ws) => {
    console.log("Editor connected");
    if (webEditorSocket) {
      console.log("There is already an editor connection; closing it.");
      webEditorSocket.close();
    }
    webEditorSocket = ws;
    sendSketchList();

    ws.on("close", () => {
      console.log("Editor disconnected");
      webEditorSocket = null;
    });

    ws.on("message", (msgStr) => {
      console.log(`Editor: ${truncate(msgStr, 64)}`);
      handleEditorMessage(JSON.parse(msgStr));
    });
  });

  // Run
  try {
    await listen(server, port);
    console.log(`Server is listening on port ${port}`);
  }
  catch (err) {
    console.error(`Server failed to start; error:\n${err}`);
  }
}

function listen(server, port) {
  return new Promise((resolve, reject) => {
    server.listen(port)
      .once('listening', resolve)
      .once('error', reject);
  });
}

async function handleEditorMessage(msg) {
  if (msg.action == PROT.ACTION.GetSketch) await sckGetSketch(msg);
  else if (msg.action == PROT.ACTION.SaveSketch) await sckSaveSketch(msg);
  else if (msg.action == PROT.ACTION.RenameSketch) await sckRenameSketch(msg);
}

async function sckSaveSketch(msg) {
  const resp = {
    action: PROT.ACTION.SaveSketchResult,
    revision: msg.revision,
  }
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}

async function sckRenameSketch(msg) {
  const resp = {
    action: PROT.ACTION.RenameSketchResult,
    error: "not implemented"
  }
  // TODO
  if (msg.newName == "barf") delete resp.error;
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}


function sendSketchList() {
  const resp = {
    action: PROT.ACTION.SketchList,
    items: [
      {
        name: "lazy-afternoon",
        title: "Lazy Afternoon",
        changedAt: new Date().toISOString(),
        changedBy: "shady artist",
      },
      {
        name: "fizzy-balls",
        title: "Fizzy Balls",
        changedAt: new Date().toISOString(),
        changedBy: "shady artist",
      },
    ],
  }
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}

async function sckGetSketch(msg) {
  // TODO: implement for real lol
  const resp = { action: PROT.ACTION.Sketch };
  if (msg.name == "fizzy-balls") {
    resp.sketchData= {
        name: msg.name,
        title: "Fizzy Balls",
        frag: "void foo() {}\n",
      };
  }
  else {
    resp.error = `Sketch '${msg.name}' not found`;
  }
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}
