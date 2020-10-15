/** serialize and send command line to server or evaluate in client if
/*  command line starts with '.' */
function cmdline(cmd) {
  const r1 = /\w+$/;
  const r2 = /\w(\s+[\w'"]+)+$/;
  return r1.test(cmd) || r2.test(cmd);
}

function WSCommandHandler(wsocket, buffer, jqElement) {
  return function (term, output, text, evalCode = false, cmdlineToJS = false) {
    ///Client command
    if(evalCode  && text.trim().startsWith('.')) {
      let cmd = text.trim().slice(1);
      if(cmdlineToJS && cmdline(cmd)) {
        const args = cmd.split(/\s+/);
        cmd = `${args[0]}(` + args.slice(1).join(',') + ')';
      }
      try {
        const ret = eval.call(null, cmd);
        //ret = eval(text.slice(1));
        term.clearCommandLine();
        if(ret !== undefined) term.append(ret);
      } catch (e) {
        if(e.message) term.append(e.message);
        term.clearCommandLine();
      }
    ///Server command
    } else {
      const WSOPEN = 1;
      if(wsocket.readyState !== WSOPEN) {
        term.append("Not connected");
        return;
      }
      let id = new Int32Array(buffer, 0, 8);
      let textBuffer =
        new Uint8Array(buffer, 8, Math.min(text.length, buffer.byteLength - 8));
      id[0] = eventIDs['commandString'];
      id[1] = text.length;
      for(let i = 0; i !== text.length; ++i) {
        textBuffer[i] = text.charCodeAt(i);
      }
      ///File save
      if(text.startsWith("save")) {
        const fname = text.trim().match(/save\s+(.+)/);
        if(fname && fname.length > 1) DOWNLOAD_FILE_NAME = fname[1];
        wsocket.send(serializeFileSave());
        term.append("save");
        return;
      /// File Upload - turn async processing to sync w/ timeout
      } else if(text.startsWith("upload")) {
        let done = false;
        jqElement.change((e) => {
          if(e.target.files.length < 1 || !e.target.files[0]) return;
          let fr = new FileReader();
          let size = e.target.files[0].size;
          let sent = 0;
          fr.onloadend = (evt) => {
              const data = fr.result;
              const FILE_READER_DONE = 2;
              if(data !== 'undefined'
                 && data !== null
                 && fr.readyState === FILE_READER_DONE) {
                const buf = serializeFileUpload(data);
                wsocket.send(buf, {binary: true});
                sent += data.byteLength;
                if(sent === size) {
                  term.append(`sent file '${e.target.files[0].name}'`);
                }
              }
              done = true;
            };
          fr.readAsArrayBuffer(e.target.files[0]);
          let attempt = 0;
          const maxAttempts = 10;
          function wait() {
            if(!done && attempt < maxAttempts) {
              setTimeout(wait, 1000);
              ++attempt;
            } else {
              if(attempt > maxAttempts) term.append("File send timed out");
            }
          }
          wait();
        });
        //without the following line it is not possible to re-send a file
        //with the same name
        jqElement.attr("value", '');
        jqElement[0].click();
        return;
      }
      wsocket.send(buffer);
      term.append(text);
    }
  }
}
