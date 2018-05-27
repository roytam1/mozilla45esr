var gStore;

function testGetDataStores() {
  navigator.getDataStores('foo').then(function(stores) {
    is(stores.length, 1, "getDataStores('foo') returns 1 element");
    is(stores[0].name, 'foo', 'The dataStore.name is foo');
    is(stores[0].readOnly, false, 'The dataStore foo is not in readonly');

    var store = stores[0];
    ok("get" in store, "store.get exists");
    ok("put" in store, "store.put exists");
    ok("add" in store, "store.add exists");
    ok("remove" in store, "store.remove exists");
    ok("clear" in store, "store.clear exists");
    ok("revisionId" in store, "store.revisionId exists");
    ok("getLength" in store, "store.getLength exists");
    ok("sync" in store, "store.sync exists");

    gStore = stores[0];

    runTest();
  }, cbError);
}

function testStoreGet(id, value) {
  gStore.get(id).then(function(what) {
    ok(true, "store.get() retrieves data");
    is(what, value, "store.get(" + id + ") returns " + value);
  }, function() {
    ok(false, "store.get(" + id + ") retrieves data");
  }).then(runTest, cbError);
}

function testStoreAdd(value) {
  return gStore.add(value).then(function(what) {
    ok(true, "store.add() is called");
    ok(what > 0, "store.add() returns something");
    return what;
  }, cbError);
}

function testStorePut(value, id) {
  return gStore.put(value, id).then(function() {
    ok(true, "store.put() is called");
  }, cbError);
}

function testStoreGetLength(number) {
  return gStore.getLength().then(function(n) {
    is(number, n, "store.getLength() returns the right number");
  }, cbError);
}

function testStoreRemove(id) {
  return gStore.remove(id).then(function() {
    ok(true, "store.remove() is called");
  }, cbError);
}

function testStoreClear() {
  return gStore.clear().then(function() {
    ok(true, "store.clear() is called");
  }, cbError);
}

var tests = [
  // Test for GetDataStore
  testGetDataStores,

  // Unknown ID
  function() { testStoreGet(42, undefined); },
  function() { testStoreGet(42, undefined); }, // twice

  // Add + Get - number
  function() { testStoreAdd(42).then(function(id) {
                 gId = id; runTest(); }, cbError); },
  function() { testStoreGet(gId, 42); },

  // Add + Get - boolean
  function() { testStoreAdd(true).then(function(id) {
                 gId = id; runTest(); }, cbError); },
  function() { testStoreGet(gId, true); },

  // Add + Get - string
  function() { testStoreAdd("hello world").then(function(id) {
                 gId = id; runTest(); }, cbError); },
  function() { testStoreGet(gId, "hello world"); },

  // Put + Get - string
  function() { testStorePut("hello world 2", gId).then(function() {
                 runTest(); }, cbError); },
  function() { testStoreGet(gId, "hello world 2"); },

  // getLength
  function() { testStoreGetLength(3).then(function() { runTest(); }, cbError); },

  // Remove
  function() { testStoreRemove(gId).then(function(what) {
                 runTest(); }, cbError); },
  function() { testStoreGet(gId, undefined); },

  // Remove - wrong ID
  function() { testStoreRemove(gId).then(function(what) {
                 runTest(); }, cbError); },

  // Clear
  function() { testStoreClear().then(function(what) {
                 runTest(); }, cbError); },
];

function runTest() {
  if (!tests.length) {
    finish();
    return;
  }

  var test = tests.shift();
  test();
}
