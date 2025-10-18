import { uniqueNamesGenerator, adjectives, animals } from 'unique-names-generator';
import {truncate, esc} from "../src-common/utils.js";
import * as PROT from "../src-common/protocol.js";
import {defaultSketchFrag, defaultSketchHydra} from "./defaultSketchFrag.js";
import {getUserName, setUserName} from "./userName.js";
import {Editor} from "./editor.js";

// const editorMode = "x-shader/x-fragment";
const editorMode = "javascript";
const logComms = true;
setTimeout(() => void init(), 50);

const sketchData = {
  name: null,
  title: null,
  frag: null,
  hydra: null,
};
let isSketchNew = false;

let elmSketchListItems;
let elmTitle, elmBtnChangeTitle, elmUserName, elmBtnChangeName, elmSketch;
let elmModal, elmNoSuchSketch, elmChangeTitle, elmChangeName;
let elmTxtName, elmTxtTitle;
let socket;
let editor;

async function init() {

  parseLocation();
  createEditor("~~ hang tight; loading ~~");
  initGui();
  initSocket();

  // DBG
  // setTimeout(() => showModal(elmChangeTitle), 1000);

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
  // TODO: compile shader
  sketchData.frag = defaultSketchFrag;
  sketchData.hydra = defaultSketchHydra;
  isSketchNew = true;
}

function displaySketch() {
  elmTitle.innerText = sketchData.title;
  editor.cm.doc.setValue(sketchData.hydra);
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
  elmBtnChangeTitle = document.getElementById("btnChangeTitle");
  elmUserName = document.querySelector("sketchHeader userName");
  elmBtnChangeName = document.getElementById("btnChangeName");
  elmSketch = document.querySelector("sketch");
  elmModal = document.querySelector("modal");
  elmNoSuchSketch = document.getElementById("noSuchSketch");
  elmChangeTitle = document.getElementById("changeTitle");
  elmChangeName = document.getElementById("changeName");
  elmTxtName = document.getElementById("txtName");
  elmTxtTitle = document.getElementById("txtTitle");

  elmUserName.innerText = getUserName();
  elmBtnChangeName.addEventListener("click", () => showModal(elmChangeName));
  elmBtnChangeTitle.addEventListener("click", () => showModal(elmChangeTitle));

  elmModal.addEventListener('keydown', e => {
    if (e.key === 'Enter' || e.key === 'Escape') {
      e.stopPropagation();
      e.preventDefault();
      if (e.key == "Enter") handleModalOK();
      else if (e.key == "Escape") handleModalCancel();
    }
  });
  document.querySelectorAll("modal .ok").forEach(e => e.addEventListener("click", handleModalOK));
  document.querySelectorAll("modal .cancel").forEach(e => e.addEventListener("click", handleModalCancel));

  document.getElementById("btnApply").addEventListener("click", apply);
  document.getElementById("btnSave").addEventListener("click", save);
  editor.onApply = apply;
  editor.onSave = save;
  editor.onChange = () => elmSketch.classList.add("dirty");
}

function apply() {
  // TODO: compile shader
  sketchData.hydra = editor.cm.doc.getValue();
  focusEditor();
}

function save() {
  // Not dirty, not new: not saving
  if (!elmSketch.classList.contains("dirty") && !isSketchNew) return;

  // TODO: compile shader
  sketchData.hydra = editor.cm.doc.getValue();
  const msg = {
    action: PROT.ACTION.SaveSketch,
    sketchData: sketchData,
    author: getUserName(),
    revision: editor.revision,
  };
  socket.send(JSON.stringify(msg));
  focusEditor();
}

function createEditor(content) {

  const elmHost = document.getElementById("theEditor")
  elmHost.innerHTML = '<div class="editorBg"></div>';

  editor = new Editor(elmHost, editorMode);
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

function renderSketchList(items) {
  items.sort((a, b) => new Date(b.changedAt) - new Date(a.changedAt));
  let html = "";
  for (const item of items) {
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

function showModal(elm) {
  elmModal.classList.add("visible");
  elm.classList.add("visible");
  elmModal.focus();
  elm.querySelectorAll(".feedback").forEach(e => e.classList.remove("visible"));
  if (elm === elmNoSuchSketch) return;
  else if (elm === elmChangeName) {
    elmTxtName.value = elmUserName.innerText;
    elmTxtName.focus();
    elmTxtName.select();
  }
  else if (elm === elmChangeTitle) {
    elmTxtTitle.value = sketchData.title;
    elmTxtTitle.focus();
    elmTxtTitle.select();
  }
}

function closeModal() {
  document.querySelector("modal content.visible").classList.remove("visible");
  document.querySelector("modal").classList.remove("visible");
  editor.cm.focus();
}

function handleModalOK() {
  const elm = document.querySelector("modal content.visible");
  if (!elm || elm === elmNoSuchSketch) return;
  if (elm === elmChangeName) {
    setUserName(elmTxtName.value);
    elmUserName.innerText = getUserName();
    closeModal();
  }
  else if (elm === elmChangeTitle) {
    const newTitle = elmTxtTitle.value.trim();
    if (newTitle == "") {
      closeModal();
      return;
    }
    const newName = getNameFromTitle(newTitle);
    elm.querySelector(".feedback.progress").classList.add("visible");
    const msg = {
      action: PROT.ACTION.RenameSketch,
      name: sketchData.name,
      newName: newName,
      newTitle: newTitle,
    };
    socket.send(JSON.stringify(msg));
  }
}

function handleModalCancel() {
  const elm = document.querySelector("modal content.visible");
  if (!elm || elm === elmNoSuchSketch) return;
  closeModal();
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
  else if (msg.action == PROT.ACTION.RenameSketchResult) msgRenameSketchResult(msg);
  else if (msg.action == PROT.ACTION.SketchList) msgSketchList(msg);
}

function msgSketch(msg) {
  if (msg.error) {
    console.log(`SCK/Sketch has error: ${msg.error}`);
    document.getElementById("sketchLoadError").innerText = msg.error;
    showModal(elmNoSuchSketch);
    return;
  }
  sketchData.name = msg.sketchData.name;
  sketchData.title = msg.sketchData.title;
  sketchData.frag = msg.sketchData.frag;
  sketchData.hydra = msg.sketchData.hydra;
  displaySketch();
}

function msgSaveSketchResult(msg) {
  if (msg.error) {
    // TODO: error layover
    console.log(`SCK/SaveSketchResult has error: ${msg.error}`);
    alert(msg.error);
    return;
  }
  if (editor.revision >= msg.revision) elmSketch.classList.remove("dirty");
  renderSketchList(msg.sketchListItems);
  isSketchNew = false;
}

function msgRenameSketchResult(msg) {
  if (msg.error) {
    console.log(`SCK/RenameSketchResult has error: ${msg.error}`);
    if (!elmChangeTitle.classList.contains("visible")) return;
    elmChangeTitle.querySelectorAll(".feedback").forEach(e => e.classList.remove("visible"));
    elmChangeTitle.querySelector(".feedback.error").classList.add("visible");
    return;
  }
  else {
    const newPath = `/sketch/${encodeURIComponent(msg.name)}`;
    window.location.pathname = newPath;
  }
  closeModal();
}

function msgSketchList(msg) {
  renderSketchList(msg.items);
}
