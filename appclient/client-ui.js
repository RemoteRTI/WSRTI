//////
//Mainly intended to be used from within the terminal through: .view.*
//Image view wrapper
function View() {
  let fps = false;
  let imgSize = false;
  let viewPort = {width: 0, height: 0};
  return Object.freeze({
    set viewport(s) { viewPort = s; },
    get viewport() { return viewPort; },
    resize(width, height) {
      if(!width || width <= 0) throw Error("Invalid width: " + width);
      if(height === undefined) {
        const curHeight = $("#image")[0].height;
        const curWidth = $("#image")[0].width;
        const ar = curHeight / curWidth;
        height = width * ar;
      } else {
          if(height <= 0) throw Error("Invalid height: " + height);
      }
      $('#image')[0].width = width;
      $('#image')[0].height = height;
      $('#gui').width(width);
      updateLayout(width);
      return {width: width, height: height};
    },
    syncAspectRatio() {
      const img = $('#image')[0];
      const ar = viewPort.height / viewPort.width;
      $('#image')[0].height = Math.round($('#image')[0].width * ar);
      return {width: $('#image')[0].width, height: $('#image')[0].height};
    },
    serverSize() {
      return {width: viewPort.width,
              height: viewPort.height,
              toString: () => {
                return `width: ${viewPort.width} height: ${viewPort.height}`}
              };
    },
    toggleFPSOverlay() {
      const visible = $("#benchmark_text").is(':visible');
      if(visible) {
        FPS_GUI = null;
        $("#benchmark_text").hide();
      } else {
        $("#benchmark_text").show();
        FPS_GUI = $('#benchmark_text');
      }
    }
  });
}
let view = View();
function resize(width, height) { view.resize(width, height); }
function sync_aspect() { view.syncAspectRatio(); }
function viewport_size() { return view.serverSize(); }
function fps() { view.toggleFPSOverlay(); }
function about() {
  const a = appConfig.about;
  return `${a.name}
          version: ${a.version}
          description: ${a.description}
          authors: ${a.author}
          copyright: ${a.copyright}
          license: ${a.license}`;
}
function layout(l) {
  const lmap = {'v': 'vertical', 'h': 'horizontal'};
  applyConsoleLayout(lmap[l]);
}
function help() {
  return `HELP:
    .resize(width [,height]): resize viewport,
                              keep current aspect ratio if no 'height' specified
    .sync_aspect(): set to default aspect ratio,
    .viewport_size(): return actual server-side viewport size,
    .fps(): toggle display of Frames Per Second information,
    .IMAGE_SIZE: image size in bytes,
    .ELAPSED: elapsed time from previous frame in ms,
    Ctrl-Shift-O: toggle output console visibility,
    Ctrl-Shift-T: toggle terminal visibility,
    .about(): about this application,
    .layout(): console/output layout: 'v' - vertical or 'h' - horizontal'`;
}


function setupGlobalEventHandlers(appcfg, terminal, output) {
  //store original display option:
  let terminalDisplayStyle = terminal.style.display;
  let outputDisplayStyle = output.style.display;
  //show/hide terminal and output textareas
  window.addEventListener("keypress", (evt) => {
    function matchModifiers(evt, modifiers) {
      return evt.altKey === modifiers.altKey
             && evt.ctrlKey === modifiers.ctrlKey
             && evt.metaKey === modifiers.metaKey
             && evt.shiftKey === modifiers.shiftKey;
    }
    const toggleTerminal = appcfg.ui.toggleTerminalKey;
    const toggleOutput = appcfg.ui.toggleOutputKey;
    if(evt.code === toggleTerminal.code
       && matchModifiers(evt, toggleTerminal.modifiers)) {
       terminal.style.display =
        terminal.style.display !== "none" ? "none" : terminalDisplayStyle;
    } else if(evt.code === toggleOutput.code
       && matchModifiers(evt, toggleOutput.modifiers)) {
       output.style.display =
        output.style.display !== "none" ? "none" : terminalDisplayStyle;
    }
  //  console.log(evt, appcfg.ui.toggleTerminalKey.code, appcfg.ui.toggleTerminalKey.modifiers );
  });
}

//////
