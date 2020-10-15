

function fillMouseData(buf, id, p, buttons, modifiers) {
  buf[0] = eventIDs[id];
  buf[1] = p.x;
  buf[2] = p.y;
  buf[3] = buttons | (modifiers << 8);
}

function serializeMouseDrag(buf, p, buttons, modifiers) {
  fillMouseData(buf, 'mousedrag', p, buttons, modifiers);
}

function serializeMouseDown(buf, p, buttons, modifiers) {
  fillMouseData(buf, 'mousedown', p, buttons, modifiers);
}

function serializeMouseUp(buf, p, buttons, modifiers) {
  fillMouseData(buf, 'mouseup', p, buttons, modifiers);
}
// {key: evt.key or keyCode,
// alt: evt.altKey,
// ctrl: evt.ctrlKey,
// meta: evt.metaKey,
// shift: evt.shiftKey,
// code: evt.code,
// right: true if right modifier pressed};
function serializeKeyEvent(event, buf, keyData) {
  buf[0] = eventIDs[event];
  buf[1] = keyData.key;
  let modifierMask = 0;
  if(keyData.alt) modifierMask |= 0x1;
  if(keyData.ctrl) modifierMask |= 0x2;
  if(keyData.meta) modifierMask |= 0x4;
  if(keyData.shift) modifierMask |= 0x8;
  if(keyData.right) modifierMask |= 0x10;
  if(keyData.special) modifierMask |= 0x20;
  buf[2] = modifierMask;
}

function serializeKeyDown(buf, keyData) {
  serializeKeyEvent('keydown', buf, keyData);
}

function serializeKeyUp(buf, keyData) {
  serializeKeyEvent('keyup', buf, keyData);
}

function serializeFileUpload(arrayBufferData) {
  const sizeofInt = 4;
  let buf = new ArrayBuffer(arrayBufferData.byteLength + 3 * sizeofInt);
  let header = new Int32Array(buf, 0, 3);
  let data = new Uint8Array(buf, 3 * sizeofInt);
  const src = new Uint8Array(arrayBufferData);
  header[0] = eventIDs['commandID'];
  header[1] = serverCommandIDs['upload'];
  header[2] = src.length;
  //PROBLEM: serial copy of data into buffer, not scalable, in the
  //future do look at either a multipart message
  //(message 1 header, message 2 data) or at the ArrayBuffer.transfer method
  for(let i = 0; i !== src.length; ++i) {
    data[i] = src[i];
  }
  return buf;
}

function serializeFileSave() {
  let msg = new Int32Array(2);
  msg[0] = eventIDs['commandID'];
  msg[1] = serverCommandIDs['save'];
  return msg;
}
