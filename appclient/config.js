const appConfig = {
  about: {
    name: "AppClient template app",
    version: "1.0.4",
    author: "Ugo Varetto",
    copyright: "(C) 2017 - Ugo Varetto",
    license: "GPL v3",
    website: "",
    description: "Remote web application client",
    documentationURL: "./doc/index.html",
    logo: "placeholder.jpg",
    title: "AppClient"
  },
  ui: {
    defaultWindowWidth: 800,
    defaultWindowHeight: 600,
    useServerAspectRatio: true,
    toggleTerminalKey: {code: "KeyT",
                        modifiers: {altKey: false,
                                    ctrlKey: true,
                                    metaKey: false,
                                    shiftKey: true}},
    toggleOutputKey: {code: "KeyO",
                      modifiers: {altKey: false,
                                  ctrlKey: true,
                                  metaKey: false,
                                  shiftKey: true}},
    consoleLayout: "vertical", //or "horizontal"
    defaultDownloadFileName: "appclient-config.txt",
    evalCode: true,
    //converts command line to js e.g. "resize 800 600" TO resize(800, 600)
    commandLineToJS: false,
    horizontalLayoutThreshold: 1000, //if image widh > threshold use horizontal
                                     //layout
  },
  connection: {
    wsHost: "localhost",
    wsImageStreamProto: "appclient-image-stream-protocol",
    wsControlStreamProto: "appclient-control-protocol",
    wsImageStreamPort: 8881,
    wsControlStreamPort: 8882
  },
  benchmark: true
};
