import { uniqueNamesGenerator, adjectives, animals } from 'unique-names-generator';
import {truncate, esc} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";
import {defaultSketch} from "./defaultSketch.js";
import {Editor} from "./editor.js";

const logComms = true;
setTimeout(() => void init(), 50);

const sketchData = {
  name: null,
  title: null,
  frag: null,
};
let elmSketchListItems;
let elmTitle, elmSketch;
let socket;
let editor;

async function init() {

  parseLocation();
  createEditor("~~ hang tight; loading ~~");
  initGui();
  initSocket();

  if (sketchData.title == null) return;
  else displaySketch();
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
  sketchData.name = getNameFromTitle(title);
  sketchData.frag = defaultSketch;
}

function displaySketch() {
  elmTitle.innerText = sketchData.title;
  editor.cm.doc.setValue(sketchData.frag);
  elmSketch.classList.remove("dirty");
}

function getNameFromTitle(title) {
  return title.toLowerCase().replaceAll(" ", "-");
}

function retrieveSketch(name) {
  const msg = {
    action: PROT.ACTION.GetSketch,
    name: name,
  };
  socket.send(JSON.stringify(msg));
}

function initGui() {

  elmSketchListItems = document.querySelector("sketchList items");
  elmTitle = document.querySelector("sketchHeader h2");
  elmSketch = document.querySelector("sketch");

  document.getElementById("btnApply").addEventListener("click", apply);
  document.getElementById("btnSave").addEventListener("click", save);
  editor.onApply = apply;
  editor.onSave = save;
  editor.onChange = () => elmSketch.classList.add("dirty");
}

function apply() {
  sketchData.frag = editor.cm.doc.getValue();
  focusEditor();
}

function save() {
  sketchData.frag = editor.cm.doc.getValue();
  const msg = {
    action: PROT.ACTION.SaveSketch,
    sketchData: sketchData,
    revision: editor.revision,
  };
  socket.send(JSON.stringify(msg));
  focusEditor();
}

function createEditor(content) {

  const elmHost = document.getElementById("theEditor")
  elmHost.innerHTML = '<div class="editorBg"></div>';

  editor = new Editor(elmHost, "x-shader/x-fragment");
  editor.cm.doc.setValue(content);
  editor.cm.refresh();
  editor.cm.display.input.focus();
}

function focusEditor() {
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
  if (msg.action == PROT.ACTION.Sketch) msgSketch(msg);
  else if (msg.action == PROT.ACTION.SaveSketchResult) msgSaveSketchResult(msg);
  else if (msg.action == PROT.ACTION.SketchList) msgSketchList(msg);
}

function msgSketch(msg) {
  if (msg.error) {
    alert(`Sorry; could not load this sketch! The server said:\n\n${msg.error}`);
    window.location.pathname = "/";
    return;
  }
  sketchData.name = msg.sketchData.name;
  sketchData.title = msg.sketchData.title;
  sketchData.frag = msg.sketchData.frag;
  displaySketch();
}

function msgSaveSketchResult(msg) {
  if (editor.revision == msg.revision) elmSketch.classList.remove("dirty");
}

function msgSketchList(msg) {
  let html = "";
  for (const item of msg.items) {
    const d = new Date(item.changedAt);
    const changedAt = `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}` +
      ` ${String(d.getHours()).padStart(2,'0')}:${String(d.getMinutes()).padStart(2,'0')}`;
    html += `
    <a href="/sketch/${encodeURIComponent(item.name)}">
      <h3>${esc(item.title)}</h3>
      <changedAt>${changedAt}</changedAt>
      <changedBy>${esc(item.changedBy)}</changedBy>
    </a>`;
  }
  elmSketchListItems.innerHTML = html;
}
