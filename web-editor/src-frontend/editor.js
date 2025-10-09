// Import CodeMirror
import CodeMirror from 'codemirror';
import 'codemirror/addon/search/search';
import 'codemirror/addon/search/searchcursor';
import 'codemirror/addon/comment/comment';
import 'codemirror/addon/dialog/dialog';
import 'codemirror/addon/edit/matchbrackets';
import 'codemirror/addon/edit/closebrackets';
import 'codemirror/addon/wrap/hardwrap';
import 'codemirror/addon/fold/foldcode';
import 'codemirror/addon/fold/foldgutter';
import 'codemirror/addon/fold/indent-fold';
import 'codemirror/addon/hint/show-hint';
import 'codemirror/addon/hint/javascript-hint';
import 'codemirror/addon/display/rulers';
import 'codemirror/addon/display/panel';
import 'codemirror/addon/hint/show-hint';
import 'codemirror/mode/clike/clike.js';
import 'codemirror/mode/javascript/javascript.js';
import 'codemirror/keymap/sublime';


class Editor {
  constructor(parent, mode) {

    this.parent = parent;
    this.revision = 0;
    this.onChange = null;
    this.onApply = null;
    this.onSave = null;

    const spaces = mode == "x-shader/x-fragment" ? 4 : 2;

    this.cm = CodeMirror(parent, {
      value: "void hello() {}",
      viewportMargin: Infinity,
      lineNumbers: true,
      matchBrackets: true,
      mode: mode,
      keyMap: 'sublime',
      autoCloseBrackets: true,
      showCursorWhenSelecting: true,
      theme: "monokai",
      dragDrop: false,
      indentUnit: spaces,
      tabSize: spaces,
      indentWithTabs: false,
      gutters: ["CodeMirror-linenumbers"],
      lineWrapping: false,
      autofocus: true,
      extraKeys: {
        "Cmd-Enter": () => this.onApply && this.onApply(),
        "Ctrl-Enter": () => this.onApply && this.onApply(),
        "Cmd-S": () => this.onSave && this.onSave(),
        "Ctrl-S": () => this.onSave && this.onSave(),
      },
    });

    this.cm.on("change", (e) => {
      ++this.revision;
      if (this.onChange) this.onChange();
    });

    this.cm.on("focus", () => {
      this.parent.classList.add("focused");
    });

    this.cm.on("blur", () => {
      this.parent.classList.remove("focused");
    });
  }
}

export {Editor}
