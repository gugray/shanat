import express from "express";
import * as http from "http";
import { WebSocketServer } from "ws";
import cors from "cors";
import { promises as fs } from "fs";
import { createReadStream } from 'fs';
import * as path from "path";
import {truncate} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";
import * as storage from "./storage.js";

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
  wsEditor.on("connection", async (ws) => {
    console.log("Editor connected");
    if (webEditorSocket) {
      console.log("There is already an editor connection; closing it.");
      webEditorSocket.close();
    }
    webEditorSocket = ws;
    await sendSketchList();

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
  };

  let error = null;
  await storage.mutex.runExclusive(async () => {
    error = await storage.saveSketch(msg.sketchData, msg.author);
    resp.sketchListItems = getSketchListItems();
  });

  if (error) resp.error = error;
  else {
    resp.revision = msg.revision;
  }
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}

async function sckRenameSketch(msg) {

  const resp = {
    action: PROT.ACTION.RenameSketchResult,
  };

  let error = null;
  await storage.mutex.runExclusive(async () => {
    error = await storage.renameSketch(msg.name, msg.newName, msg.newTitle);
    resp.sketchListItems = getSketchListItems();
  });

  if (error) resp.error = error;
  else {
    resp.name = msg.newName;
    resp.title = msg.newTitle;
  }
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}

function getSketchListItems() {
  const items = [];
  for (const [name, info] of Object.entries(storage.sketchInfos))
    items.push({
      name: name,
      title: info.title,
      changedAt: info.changedAt.toISOString(),
      changedBy: info.author,
    });
  return items;
}

async function sendSketchList() {

  let sketchListItems;
  await storage.mutex.runExclusive(() => sketchListItems = getSketchListItems());

  const resp = {
    action: PROT.ACTION.SketchList,
    items: sketchListItems,
  };
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}

async function sckGetSketch(msg) {
  const sketchData = await storage.mutex.runExclusive(async () => await storage.getSketch(msg.name));
  const resp = { action: PROT.ACTION.Sketch };
  if (!sketchData) resp.error = `Failed to load sketch '${msg.name}'; maybe it doesn't exist?`;
  else resp.sketchData = sketchData;
  const outStr = JSON.stringify(resp);
  webEditorSocket.send(outStr);
}
