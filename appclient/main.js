
function elementPosition(e) {
  const r = e.getBoundingClientRect();
  return {x: r.left + window.pageXOffset, y: r.top + window.pageYOffset};
}

function appendScrollText(textarea, txt) {
  textarea.value += textarea.length ? txt : txt + '\n';
  textarea.scrollTop = textarea.scrollHeight;
}

function mousePosition(mx, my, img) {
  const offset = elementPosition(img);
  return {x: Math.min(Math.max(mx - offset.x, 0), img.width - 1),
          y: Math.min(Math.max(my - offset.y, 0), img.height - 1)};
}

function setupViewportEvents(viewport, eventHandler) {

  function modifiers(evt) {
    let m = 0;
    if(evt.altKey) m |= 0x1;
    if(evt.ctrlKey) m |= 0x2;
    if(evt.metaKey) m |= 0x4;
    if(evt.shiftKey) m |= 0x8;
    return m;
  }
  function buildKeyEventData(evt) {
    let k = evt.key.length == 1 ?
            evt.key.charCodeAt(0) : evt.keyCode;
    let r = false;
    let s = false;
    if(evt.key.length > 1) {
      s = true;
      if(evt.code.endsWith("Right")) {
        r = true;
      }
    }
    return {key: k,
            alt: evt.altKey,
            ctrl: evt.ctrlKey,
            meta: evt.metaKey,
            shift: evt.shiftKey,
            code: evt.code,
            special: s,
            right: r};
  }
  viewport.onmousedown = (evt) => {
    if(CONSOLE_EVENT) return;
    evt.preventDefault();
    const p = mousePosition(evt.clientX, evt.clientY, viewport);
    eventHandler.onMouseDown(viewport, p, evt.buttons, modifiers(evt));
  }
  viewport.onmouseup = (evt) => {
    if(CONSOLE_EVENT) return;
    evt.preventDefault();
    const p = mousePosition(evt.clientX, evt.clientY, viewport);
    eventHandler.onMouseDown(viewport, p, evt.buttons, modifiers(evt));
  }
  viewport.oncontextmenu = (evt) => {return false;};
  viewport.onmousemove = (evt) => {
    if(CONSOLE_EVENT) return;
    evt.preventDefault();
    const p = mousePosition(evt.clientX, evt.clientY, viewport);
    eventHandler.onMouseMove(viewport, p, evt.buttons, modifiers(evt));
  }
  viewport.onkeydown = (evt) => {
    eventHandler.onKeyDown(viewport, buildKeyEventData(evt));
  }
  viewport.onkeyup = (evt) => {
    eventHandler.onKeyUp(viewport, buildKeyEventData(evt));
  }
}

function connect(connection, log) {
  const c = connection;
  const wsImageStreamURI = "ws://" + c.wsHost + ":" + c.wsImageStreamPort;
  const wsControlStreamURI = "ws://" + c.wsHost + ":" + c.wsControlStreamPort;

  let imageStreamChannel = new WebSocket(wsImageStreamURI,
                                         c.wsImageStreamProto);
  let controlChannel = new WebSocket(wsControlStreamURI,
                                     c.wsControlStreamProto);
  controlChannel.binaryType = 'arraybuffer';
  controlChannel.onopen = () => log('Control channel open');
  controlChannel.onerror = (_) => log(`Control channel error`);
  controlChannel.onclose = (e) => log(`Control channel closed: ${e.code}`);
  return Object.freeze({imageStream: imageStreamChannel,
                        ctrlStream: controlChannel});
}

function disconnect(streams) {
  streams.imageStream.close();
  streams.ctrlStream.close();
}

function applyConsoleLayout(layout) {
  if(layout === 'horizontal') {
    $("#output").css('order', '2');
    $("#console").css('order', '1');
    layout = 'row';
  } else if(layout === 'vertical') {
    $("#output").css('order', '1');
    $("#console").css('order', '2');
    layout = 'column';
  } else throw Error("Invalid layout " + layout);
  $("#console_container").css('flex-direction', layout);
  // if(layout === "horizontal") {
  //   $("#output").css({'width': '49.5%', 'float': 'right'});
  //   $("#console").css({'width': '50%', 'float': 'left'});
  // } else if(layout === "vertical") {
  //   $("#output").css({'width': '100%', 'float': ''});
  //   $("#console").css({'width': '100%', 'float': ''});
  // } else {
  //   throw Error("Invalid layout - " + layout);
  // }
}

function updateLayout(imgWidth) {
  const dynamicLayout = appConfig.ui.horizontalLayoutThreshold > 0;
  if(!dynamicLayout) return;
  if(imgWidth > appConfig.ui.horizontalLayoutThreshold) {
    applyConsoleLayout("horizontal");
  } else {
    applyConsoleLayout("vertical");
  }
}

$(function(){
try {

//GUI setup
CONSOLE_EVENT = false;
$('#gui').width(appConfig.ui.defaultWindowWidth);
$('#image')[0].width = appConfig.ui.defaultWindowWidth;
$('#image')[0].height = appConfig.ui.defaultWindowHeight;
$('#image')[0].src = 'placeholder.jpg';
let output = $('#log')[0];
// let log = (msg) => {appendScrollText(output, msg);};
let log = (msg) => {
  let c = $('#log');
  if(c.val().length > 0) {
    c.val(c.val() + '\n' + msg);
  } else {
    c.val(msg);
  }
  c.animate({scrollTop: c[0].scrollHeight - c.height()}, 600);
}
//create a terminal console in the 'term' textarea and invoke the command
//handler each time the enter key is pressed
const resizeParentOption = false;
const customStyleOption = true;
let term = terminal($('#term')[0], (t, a) => t.append(a),
                      resizeParentOption, customStyleOption);

let streams = connect(appConfig.connection, log);

//buffers used for serialization
// 4 byte id, 4 byte x, 4 byte y, 4 byte buttons
let eventBuffer = new Int32Array(4);
//256 bytes + 4 byte id + 4 byte length info
//let cmdStringBuffer = new ArrayBuffer(0xff + 4);
let cmdStringBuffer = new ArrayBuffer(256 + 4 + 4);

//event handler: forward GUI events to server
let eventHandler = WSEventHandler(streams.ctrlStream, eventBuffer);


//add file input to allow for file uploads
let fupload = $("<input type='file' id='fileElem' style='display:none'>");
//command handler: forward command line events to server, only the full
//command line is currently sent, in the future we need to support validation
//and possibly sending the parsed commands
let commandHandler = WSCommandHandler(streams.ctrlStream,
                                      cmdStringBuffer,
                                      fupload);

//setup asynchronouse image receiver: each received image is dislayed in the
//passed img element
setupWSImageStreamHandler(streams.imageStream, $('#image')[0], log,
                          appConfig.benchmark);
//event/command handlers for events/commands received asynchronously from the
//server, note that there is currently no way of matching a request to a
//specific reply
function resizeCBack(imgWidth, imgHeight) {
  if(appConfig.ui.useServerAspectRatio) {
    const ar = imgHeight / imgWidth
    $('#image')[0].height = Math.round($('#image')[0].width * ar);
  } else {
    $('#image')[0].width = imgWidth;
    $('#image')[0].height = imgHeight;
    $('#gui').width(imgWidth);
  }
  updateLayout(imgWidth);
}
setupWSServerEventHandler(streams.ctrlStream, resizeCBack, log, view);
setupViewportEvents($('#image')[0], eventHandler);

term.callback = function(t, txt) {
  commandHandler(t, $('#output')[0], txt,
                 appConfig.ui.evalCode,
                 appConfig.ui.commandLineToJS);
};

} catch (e) {
  appendScrollText($("#log")[0],
                   `${e.message} file: ${e.fileName} line: ${e.lineNumber}`);
}

setupGlobalEventHandlers(appConfig, $("#console")[0], output);

$("#console").draggable({
  start: (e) => {e.stopPropagation(); CONSOLE_EVENT = true;},
  stop: (e) => {e.stopPropagation(); CONSOLE_EVENT = false;},
  drag: (e) => {e.stopPropagation();}
});
//$("#console").click(function(e){ e.stopPropagation(); });
$("#console").resizable({
  start: (e) => {e.stopPropagation(); CONSOLE_EVENT = true;},
  stop: (e) => {e.stopPropagation(); CONSOLE_EVENT = false;},
  resize: (e) => {e.stopPropagation();}
  // handle: "#console",
  // cancel: "#term"
});
$("#output").resizable();
applyConsoleLayout(appConfig.ui.consoleLayout);

if(appConfig.benchmark) {
  let b = $("<p id='benchmark_text'>FPS</p>");
  b.hide();
  b.css({top: 10 + 'px',// elementPosition(img).y) + 'px',
         left: 10 + 'px',//elementPosition(img).x) + 'px',
         position: 'absolute',
         color: '#f0ffaa',
         padding: '0',
         margin: '0',
         'font-family': 'Monaco',
         'font-size': '22px'});
  $("#gui").append(b);
  FPS_GUI = null;
}

document.title = appConfig.about.title + " - " + appConfig.about.version;

DOWNLOAD_FILE_NAME = appConfig.ui.defaultDownloadFileName;

});
