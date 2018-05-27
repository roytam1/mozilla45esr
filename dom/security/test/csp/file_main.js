function doXHR(uri) {
  try {
    var xhr = new XMLHttpRequest();
    xhr.open("GET", uri);
    xhr.send();
  } catch(ex) {}
}

doXHR("http://mochi.test:8888/tests/dom/security/test/csp/file_CSP.sjs?testid=xhr_good");
doXHR("http://example.com/tests/dom/security/test/csp/file_CSP.sjs?testid=xhr_bad");
fetch("http://mochi.test:8888/tests/dom/security/test/csp/file_CSP.sjs?testid=fetch_good");
fetch("http://example.com/tests/dom/security/test/csp/file_CSP.sjs?testid=fetch_bad");
navigator.sendBeacon("http://mochi.test:8888/tests/dom/security/test/csp/file_CSP.sjs?testid=beacon_good");
try {
  navigator.sendBeacon("http://example.com/tests/dom/security/test/csp/file_CSP.sjs?testid=beacon_bad");
} catch(ex) {}


new Worker("file_main_worker.js").postMessage({inherited : false});


var blobxhr = new XMLHttpRequest();
blobxhr.open("GET", "file_main_worker.js")
blobxhr.responseType = "blob";
blobxhr.send();
blobxhr.onload = () => {
  new Worker(URL.createObjectURL(blobxhr.response)).postMessage({inherited : true});
}
