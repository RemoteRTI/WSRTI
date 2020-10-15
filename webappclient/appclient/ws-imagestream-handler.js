/** Configure websocket to receive blobs and display the received data
    through an img html element */
function setupWSImageStreamHandler(wsocket, image, log, benchmark) {
  wsocket.binaryType = "blob";
  let urlCreator = window.URL || window.webkitURL;
  let imageUrl = ['placeholder.jpg', 'placeholder.jpg'];
  let front = 1, back = 0;
  if(benchmark) {
    let prev = performance.now();
    FRAMES = 0;
    ELAPSED = 0;
    FPS = 0;
    IMAGE_SIZE = 0;
    wsocket.onmessage = function (e) {
      const cur = performance.now();
      ELAPSED += cur - prev;
      IMAGE_SIZE = e.data.size;
      if(FRAMES && FRAMES % 50 === 0) {
        FPS = (1000 * 50) / ELAPSED;
        ELAPSED /= 50;
        FRAMES = 1;
      }
      FRAMES++;
      prev = cur;
      if(FPS_GUI) FPS_GUI.html(FPS.toFixed());
      urlCreator.revokeObjectURL(imageUrl[back]);
      imageUrl[back] = urlCreator.createObjectURL(e.data);
      [front, back] = [back, front];
      image.src = imageUrl[front];
    }
  } else {
    wsocket.onmessage = function (e) {
      urlCreator.revokeObjectURL(imageUrl[back]);
      imageUrl[back] = urlCreator.createObjectURL(e.data);
      [front, back] = [back, front];
      image.src = imageUrl[front];
    }
  }
  wsocket.onopen = () => log("Image stream open");
  wsocket.onerror = (_) => log("Image stream error");
  wsocket.onclose = (e) => log(`Image stream closed ${e.code}`);
}
