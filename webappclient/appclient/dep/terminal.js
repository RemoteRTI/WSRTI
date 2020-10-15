
//Terminal implementation: creates a console terminal inside a textarea
// Author: Ugo Varetto
// License: GPL v3

/** Return the location of the last eol character, assuming that prompt
    always resides on the last added line in the textarea */
function findCaretLinePos(textarea, prompt) {
  return textarea.value.lastIndexOf("\n") + 1;
}
/** Return the location of the caret where the next typed character is going
*   to be inserted */
function findCaretPos(textarea, prompt) {
  return findCaretLinePos(textarea, prompt) + prompt.length;
}
/** Return the text currently on the command line stripped of the prompt */
function cmdLineText(textarea, prompt) {
  return textarea.value.slice(findCaretPos(textarea, prompt),
                              textarea.value.length).trim();
}
/** Return all text contained in the text area above the prompt line */
function alltext(textarea, prompt) {
  return textarea.value.slice(0, findCaretLinePos(textarea, prompt));
}
/** Put caret after prompt characters */
function resetCaret(textarea, prompt, event = null) {
  textarea.selectionStart = findCaretPos(textarea, prompt);
  textarea.selectionEnd = textarea.selectionStart;
  if(event) event.preventDefault();
}
/** Append text to console output above prompt line */
function appendText(textarea, txt, prompt) {
  const t = alltext(textarea, prompt);
  textarea.value = t.length > 0 ?
                      alltext(textarea, prompt) + txt + '\n' + prompt :
                      txt + '\n' + prompt;
  textarea.scrollTop = textarea.scrollHeight;
  resetCaret(textarea, prompt);
}
/** Set text on command line */
function setCmdLine(textarea, txt, prompt) {
  textarea.value = alltext(textarea, prompt) + prompt + txt;
}

/**
 * @typedef {object} WrappedTerminal
 * @property {function} append - append text to terminal
 * @property {function} callback - callback function receiving the entered text
 * @property {function} clearCommandLine - clears the command line text
 * @property {string} prompt - terminal prompt
 */

/** Create a terminal console inside an html textarea DOM object.
  The passed callback function is invoked with
    - a textarea wrapper which implements an @c append method which adds texta
      above the prompt and scrolls if needed
    - the current text on the command line
  @param {HTMLTextAreaElement} textarea - target textarea
  @param {function} cback - callback function: both a
         reference to a wrapped textarea object and the text on the
         command line are passed to this function when the enter key
         is pressed
  @param {boolean} resizeParent - resize parent element ?       
  @param {boolean} customStyle - use default console style
  @return {WrappedTerminal} wrapper for interacting with terminal instance
*/
//color: #c0ffc0;
function terminal(textarea, cback,
                  resizeParent = false, customStyle = false) {
  if(!textarea) throw Error("terminal() textarea undefined");
  let cback_ = cback;
  /** Default textarea terminal style */
  const TERMINAL_STYLE = `
    background-color: #303030;
    font-weight: bold;
    color: #c0ffc0;
    font-family: Monaco;
    font-size: 12px;
    margin: 0 auto;
    width: 100%;
    border: 2px solid gray;
    border-radius: 4px`
  //variables for history buffer management
  //command lines are stored into history together with the EOL character!
  let history = History(20);
  //command prompt
  const prompt = '>';
  //keycodes
  const up = 38;
  const down = 40;
  const left = 37;
  //const right = 39;
  const backspace = 8;
  const enter = 13;
  //!!!! returned object
  let wrapper = Object.freeze({
    append(txt) { appendText(textarea, txt, prompt); },
    set callback(cb) { cback_ = cb; },
    get prompt() { return prompt },
    set prompt(p) { prompt = p; },
    clearCommandLine() { setCmdLine(textarea, "", prompt); },
    clear() { textarea.value = prompt; },
    get history() { return history; }
  });
  //use default style only of boolean parameter is false and
  //no explicit 'style' attribute is set on textarea node
  if(!customStyle && !textarea.getAttribute('style')) {
    textarea.setAttribute("style", TERMINAL_STYLE);
  }
  textarea.value = prompt;
  textarea.spellcheck = false;
  //keydown event handler:
  // * up and down: move through history buffer and prevent the cursor from
  //   moving to the left of prompt
  textarea.onkeydown = (evt) => {
      evt = evt || window.event;
      if(evt.keyCode === enter) {
        const text = cmdLineText(textarea, prompt);
        if(text.length === 0) {
          resetCaret(textarea, prompt, evt);
          return;
        }
        resetCaret(textarea, prompt, evt);
        history.put(text);
        cback_(wrapper, text);
      } else if(evt.keyCode === down) {
        if(history.length > 0) {
          setCmdLine(textarea, history.get(-1), prompt);
        }
        evt.preventDefault();
      } else if(evt.keyCode === up) {
        if(history.length > 0) {
          setCmdLine(textarea, history.get(1), prompt);
        }
        evt.preventDefault();
      } else if(evt.keyCode === enter) {
        resetCaret(textarea, prompt, evt);
      } else if(evt.keyCode === backspace || evt.keyCode === left) {
        if(textarea.selectionStart === findCaretPos(textarea, prompt)) {
          resetCaret(textarea, prompt, evt);
        }
      } else if(evt.code === 'KeyA' && evt.ctrlKey) {
        resetCaret(textarea, prompt, evt);
      }

  }
  //mouse button events: disable default behavior
  textarea.onmousedown = (evt) => {
    resetCaret(textarea, prompt);
  }
  textarea.onmouseup = (evt) => {
    resetCaret(textarea, prompt);
  }
  if(resizeParent) {
    textarea.onmousemove = (evt) => {
      if(evt.buttons & 0x1) {
        textarea.parentNode.style.width = textarea.style.width;
        textarea.parentNode.style.height = textarea.style.height;
      }
    }
  }
  return wrapper;
}
