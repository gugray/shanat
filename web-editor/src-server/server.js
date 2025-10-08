import express from "express";
import * as http from "http";
import { WebSocketServer } from "ws";
import cors from "cors";
import { promises as fs } from "fs";
import { createReadStream } from 'fs';
import * as path from "path";
import {truncate} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";

let editorSocket = null;

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
    if (editorSocket) {
      console.log("There is already an editor connection; closing it.");
      editorSocket.close();
    }
    editorSocket = ws;

    ws.on("close", () => {
      console.log("Editor disconnected");
      editorSocket = null;
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
}
