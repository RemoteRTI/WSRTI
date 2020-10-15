function strtoab(str) {
  return new Uint8Array(str.split('').map(ch => ch.charCodeAt(0))).buffer;
}

function abtostr(ab) {
  return new Uint8Array(ab).reduce((s, ch) => s + String.fromCharCode(ch), '');
}

/** Receive and parse messages from the server */
function setupWSServerEventHandler(wsocket, resizeCB, log, view) {
  wsocket.onmessage = (e) => {
    let id = new Int32Array(e.data, 0, 2);
    if(id[0] === serverEventIDs['viewportResize']) {
      let size = new Int32Array(e.data, 4, 2);
      resizeCB(size[0], size[1]);
      view.viewport = {width: size[0], height: size[1]};
    } else if(id[0] === serverEventIDs['print']) {
      let size = id[1];
      let chars = new Uint8Array(e.data, 8, size);
      //log(String.fromCharCode(...chars));
      log(abtostr(chars));
    } else if(id[0] === serverEventIDs['fileDownload']) {
      const size = id[1]; //size information is redundant if all the data
                          //is packages into a single bytearray
      const buffer = new Uint8Array(e.data, 2 * 4, e.data.byteLength - 2 * 4);
      if(navigator.appVersion.search('Safari') !== -1
         && navigator.appVersion.search('Chrome') === -1) {
        // download(buffer, DOWNLOAD_FILE_NAME + ".txt",
        //          "text/plain;charset=utf-8");
        //will open in new window 
        saveAs(new Blob([buffer], {type: "text/plain;charset=utf-8"}),
               DOWNLOAD_FILE_NAME);
      } else {
        //download(buffer, DOWNLOAD_FILE_NAME, "application/octect-stream");
        saveAs(new Blob([buffer], {type: "application/octect-stream"}),
               DOWNLOAD_FILE_NAME);
      }
    }
  }
}
