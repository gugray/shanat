import {Editor} from "./editor.js";

const logComms = true;
setTimeout(() => void init(), 50);

let socket;
let editor;

async function init() {
  createEditor("henlo();\n");
  initSocket();
}

function createEditor(content) {

  const elmHost = document.getElementById("theEditor")
  elmHost.innerHTML = '<div class="editorBg"></div>';

  editor = new Editor(elmHost, "x-shader/x-fragment");
  // editor.onSubmit = () => submitShader(name);
  // editor.onToggleAnimate = () => toggleAnimate();
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
  const socketUrl = "ws://" + window.location.hostname + ":8090/editor";
  socket = new WebSocket(socketUrl);
  socket.addEventListener("open", () => {
    if (logComms) console.log("[EDITOR] Socket open");
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

function truncate(str, num) {
  if (str.length > num) {
    return str.slice(0, num) + "...";
  } else {
    return str;
  }
}

function getSocketUrl() {
  return "ws://" + window.location.hostname + ":8090/editor";
}
