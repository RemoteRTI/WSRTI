/**
/* @typedef HistoryObject
/* @property {number} maxSize - maximum size of circular buffer
/* @property {number} length - current size of circular buffer
/* @property {function} put - put data into history buffer
/* @property {function} get - receives a negative ('down') or positive ('up') \
                        direction parameter and returns the next element in
                        history buffer
*/

/**
/* History constructor: history implementation with circular buffer.
/* @param {number} size - max number of elements in history buffer
/* @return {HistoryObject}
*/
function History(size) {
  if(size <= 0) throw Error("History: invalid size " + size);
  let buffer_ = [];
  let currentIndex_ = 0;
  function put(data) {
    if(buffer_.length < size) {
      buffer_.unshift(data);
      currentIndex_ = 0;
    } else {
      buffer_.pop();
      put(data);
    }
  }
  return Object.freeze({
    get maxSize() { return size; },
    get length() { return buffer_.length; },
    data() { return buffer_;tory },
    get(dir) {
      if(dir === 0) throw Error("History.get: Invalid direction");
      if(buffer_.length === 0) return null;
      const inc = Math.sign(dir);
      const ret = buffer_[currentIndex_];
      currentIndex_ = (currentIndex_ + inc + buffer_.length) % buffer_.length;
      return ret;
    },
    put
  });
}

//assert implementation
function assert(condition, message) {
  if(!condition) throw Error(message);
}

//unit test history object
function historyTest() {
  const up = 1;
  const down = -1;
  let history = History(2);
  assert(history.maxSize === 2);
  assert(history.length === 0);
  history.put('one');
  assert(history.length === 1);
  history.put('two');
  assert(history.length === 2);
  assert(history.get(up) === 'two');
  assert(history.get(up) === 'one');
  assert(history.get(down) === 'two');
  assert(history.get(down) === 'one');
  history.put('three');
  assert(history.length === 2);
  assert(history.get(up) === 'three');
  assert(history.get(up) === 'two');
}

historyTest();
