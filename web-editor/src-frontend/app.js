import { uniqueNamesGenerator, adjectives, animals } from 'unique-names-generator';
import {truncate} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";
import {Editor} from "./editor.js";

const logComms = true;
setTimeout(() => void init(), 50);

const sketchData = {
  name: null,
  title: null,
  frag: null,
};
let elmTitle;
let socket;
let editor;

async function init() {

  parseLocation();
  createEditor("~~ hang tight; loading ~~");
  initGui();
  initSocket();

  if (sketchData.title == null) return;

  elmTitle.innerText = sketchData.title;
  editor.cm.doc.setValue(sketchData.frag);
}

function parseLocation() {
  const path = window.location.pathname.replace(/\/$/, '');
  if (path == "") {
    fillNewSketch();
    return;
  }
  const m = path.match(/^\/sketch\/([A-Za-z0-9_-]+)$/);
  if (!m) {
    window.location.pathname = "/";
    return;
  }
  sketchData.name = m[1];
}

function fillNewSketch() {
  const title = uniqueNamesGenerator({
    dictionaries: [adjectives, animals],
    length: 2,
    separator: " ",
    style: "capital",
  });
  sketchData.title = title;
  sketchData.name = title.toLowerCase().replaceAll(" ", "-");
  sketchData.frag = "foo(); // bar";
}

function retrieveSketch(name) {
  const msg = {
    action: PROT.ACTION.GetSketch,
    name: name,
  };
  socket.send(JSON.stringify(msg));
}

function initGui() {
  elmTitle = document.querySelector("sketchHeader h2");
}

function createEditor(content) {

  const elmHost = document.getElementById("theEditor")
  elmHost.innerHTML = '<div class="editorBg"></div>';

  editor = new Editor(elmHost, "x-shader/x-fragment");
  // editor.onSubmit = () => submitShader(name);

  editor.cm.doc.setValue(content);
  editor.cm.refresh();
  editor.cm.display.input.focus();
}

function flashEditor(className) {
  const elmBg = document.querySelector("#theEditor .editorBg");
  if (!elmBg) return;
  if (elmBg.classList.contains(className)) return;
  elmBg.classList.add(className);
  setTimeout(() => elmBg.classList.remove(className), 200);
}

function initSocket() {
  const socketUrl = `ws://${window.location.hostname}:8090${PROT.kEditorSocketPath}`;
  socket = new WebSocket(socketUrl);
  socket.addEventListener("open", () => {
    if (logComms) console.log("[EDITOR] Socket open");
    if (sketchData.name !== null && sketchData.title === null) retrieveSketch(sketchData.name);
  });
  socket.addEventListener("message", (event) => {
    const msgStr = event.data;
    if (logComms) console.log(`[EDITOR] Message: ${truncate(msgStr, 64)}`);
    const msg = JSON.parse(msgStr);
    handleSocketMessage(msg);
  });
  socket.addEventListener("close", () => {
    if (logComms) console.log("[EDITOR] Socket closed");
    socket = null;
  });
}

function handleSocketMessage(msg) {
}

function getSocketUrl() {
  return "ws://" + window.location.hostname + ":8090/editor";
}
