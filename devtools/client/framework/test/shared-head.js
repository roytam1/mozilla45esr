/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// This shared-head.js file is used for multiple directories in devtools.

var {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

function scopedCuImport(path) {
  const scope = {};
  Cu.import(path, scope);
  return scope;
}

const {Services} = scopedCuImport("resource://gre/modules/Services.jsm");
const {gDevTools} = scopedCuImport("resource://devtools/client/framework/gDevTools.jsm");
const {console} = scopedCuImport("resource://gre/modules/Console.jsm");
const {ScratchpadManager} = scopedCuImport("resource://devtools/client/scratchpad/scratchpad-manager.jsm");
const {require} = scopedCuImport("resource://devtools/shared/Loader.jsm");

const {TargetFactory} = require("devtools/client/framework/target");
const DevToolsUtils = require("devtools/shared/DevToolsUtils");
let promise = require("promise");

const TEST_DIR = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
const CHROME_URL_ROOT = TEST_DIR + "/";
const URL_ROOT = CHROME_URL_ROOT.replace("chrome://mochitests/content/", "http://example.com/");

// All test are asynchronous
waitForExplicitFinish();

// Uncomment this pref to dump all devtools emitted events to the console.
// Services.prefs.setBoolPref("devtools.dump.emit", true);

function getFrameScript() {
  let mm = gBrowser.selectedBrowser.messageManager;
  let frameURL = "chrome://devtools/content/shared/frame-script-utils.js";
  mm.loadFrameScript(frameURL, false);
  SimpleTest.registerCleanupFunction(() => {
    mm = null;
  });
  return mm;
}

DevToolsUtils.testing = true;
registerCleanupFunction(() => {
  DevToolsUtils.testing = false;
  Services.prefs.clearUserPref("devtools.dump.emit");
  Services.prefs.clearUserPref("devtools.toolbox.host");
  Services.prefs.clearUserPref("devtools.toolbox.previousHost");
});

registerCleanupFunction(function* cleanup() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  yield gDevTools.closeToolbox(target);

  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});

/**
 * Add a new test tab in the browser and load the given url.
 * @param {String} url The url to be loaded in the new tab
 * @return a promise that resolves to the tab object when the url is loaded
 */
function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab(url);
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab);
  }, true);

  return def.promise;
}

/**
 * Remove the given tab.
 * @param {Object} tab The tab to be removed.
 * @return Promise<undefined> resolved when the tab is successfully removed.
 */
function removeTab(tab) {
  info("Removing tab.");
  return new Promise(resolve => {
    let tabContainer = gBrowser.tabContainer;
    tabContainer.addEventListener("TabClose", function onClose(aEvent) {
      tabContainer.removeEventListener("TabClose", onClose, false);
      info("Tab removed and finished closing.");
      resolve();
    }, false);

    gBrowser.removeTab(tab);
  });
}

function synthesizeKeyFromKeyTag(key) {
  is(key && key.tagName, "key", "Successfully retrieved the <key> node");

  let modifiersAttr = key.getAttribute("modifiers");

  let name = null;

  if (key.getAttribute("keycode"))
    name = key.getAttribute("keycode");
  else if (key.getAttribute("key"))
    name = key.getAttribute("key");

  isnot(name, null, "Successfully retrieved keycode/key");

  let modifiers = {
    shiftKey: !!modifiersAttr.match("shift"),
    ctrlKey: !!modifiersAttr.match("control"),
    altKey: !!modifiersAttr.match("alt"),
    metaKey: !!modifiersAttr.match("meta"),
    accelKey: !!modifiersAttr.match("accel")
  };

  info("Synthesizing key " + name + " " + JSON.stringify(modifiers));
  EventUtils.synthesizeKey(name, modifiers);
}

/**
 * Wait for eventName on target.
 * @param {Object} target An observable object that either supports on/off or
 * addEventListener/removeEventListener
 * @param {String} eventName
 * @param {Boolean} useCapture Optional, for addEventListener/removeEventListener
 * @return A promise that resolves when the event has been handled
 */
function once(target, eventName, useCapture=false) {
  info("Waiting for event: '" + eventName + "' on " + target + ".");

  let deferred = promise.defer();

  for (let [add, remove] of [
    ["addEventListener", "removeEventListener"],
    ["addListener", "removeListener"],
    ["on", "off"]
  ]) {
    if ((add in target) && (remove in target)) {
      target[add](eventName, function onEvent(...aArgs) {
        info("Got event: '" + eventName + "' on " + target + ".");
        target[remove](eventName, onEvent, useCapture);
        deferred.resolve.apply(deferred, aArgs);
      }, useCapture);
      break;
    }
  }

  return deferred.promise;
}

/**
 * Some tests may need to import one or more of the test helper scripts.
 * A test helper script is simply a js file that contains common test code that
 * is either not common-enough to be in head.js, or that is located in a separate
 * directory.
 * The script will be loaded synchronously and in the test's scope.
 * @param {String} filePath The file path, relative to the current directory.
 *                 Examples:
 *                 - "helper_attributes_test_runner.js"
 *                 - "../../../commandline/test/helpers.js"
 */
function loadHelperScript(filePath) {
  let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
  Services.scriptloader.loadSubScript(testDir + "/" + filePath, this);
}

function waitForTick() {
  let deferred = promise.defer();
  executeSoon(deferred.resolve);
  return deferred.promise;
}

function loadToolbox (url) {
  let { promise: p, resolve } = promise.defer();
  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    gDevTools.showToolbox(target).then(resolve);
  }, true);

  content.location = url;
  return p;
}

function unloadToolbox (toolbox) {
  return toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();
  });
}
