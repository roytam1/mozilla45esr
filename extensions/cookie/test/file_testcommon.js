var gExpectedCookies;
var gExpectedLoads;

var gPopup;

var gLoads = 0;

function setupTest(uri, cookies, loads) {
  SimpleTest.waitForExplicitFinish();

  SpecialPowers.Cc["@mozilla.org/preferences-service;1"]
               .getService(SpecialPowers.Ci.nsIPrefBranch)
               .setIntPref("network.cookie.cookieBehavior", 1);

  var cs = SpecialPowers.Cc["@mozilla.org/cookiemanager;1"]
                        .getService(SpecialPowers.Ci.nsICookieManager2);
  cs.removeAll();

  gExpectedCookies = cookies;
  gExpectedLoads = loads;

  // Listen for MessageEvents.
  window.addEventListener("message", messageReceiver, false);

  // load a window which contains an iframe; each will attempt to set
  // cookies from their respective domains.
  gPopup = window.open(uri, 'hai', 'width=100,height=100');
}

function finishTest()
{
  SpecialPowers.Cc["@mozilla.org/preferences-service;1"]
               .getService(SpecialPowers.Ci.nsIPrefBranch)
               .clearUserPref("network.cookie.cookieBehavior");

  SimpleTest.finish();
}

/** Receives MessageEvents to this window. */
// Count and check loads.
function messageReceiver(evt)
{
  is(evt.data, "message", "message data received from popup");
  if (evt.data != "message") {
    gPopup.close();
    window.removeEventListener("message", messageReceiver, false);

    finishTest();
    return;
  }

  // only run the test when all our children are done loading & setting cookies
  if (++gLoads == gExpectedLoads) {
    gPopup.close();
    window.removeEventListener("message", messageReceiver, false);

    runTest();
  }
}

// runTest() is run by messageReceiver().
// Count and check cookies.
function runTest() {
  // set a cookie from a domain of "localhost"
  document.cookie = "oh=hai";

  var cs = SpecialPowers.Cc["@mozilla.org/cookiemanager;1"]
                        .getService(SpecialPowers.Ci.nsICookieManager);
  var count = 0;
  for(var list = cs.enumerator; list.hasMoreElements(); list.getNext())
    ++count;
  is(count, gExpectedCookies, "total number of cookies");
  cs.removeAll();

  finishTest();
}
