
/** Construct and return object which serializes and sends gui events
    to the server */
function WSEventHandler(wsocket, buffer) {
  let send = (b) => {if(wsocket.readyState === 1) wsocket.send(b)};
  return Object.freeze({
    onMouseDown(element, p, buttons, modifiers) {
      element.focus();
      serializeMouseDown(buffer, p, buttons, modifiers);
      send(buffer);
    },
    onMouseUp(element, p, buttons, modifiers) {
      serializeMouseUp(buffer, p, buttons, modifiers);
      send(buffer);
    },
    onMouseMove(element, p, buttons, modifiers) {
      if(buttons) {
        serializeMouseDrag(buffer, p, buttons, modifiers);
        send(buffer);
      }
    },
    onKeyDown(element, keydata) {
      serializeKeyDown(buffer, keydata);
      send(buffer);
    },
    onKeyUp(element, keydata) {
      serializeKeyUp(buffer, keydata);
      send(buffer);
    }});
}
