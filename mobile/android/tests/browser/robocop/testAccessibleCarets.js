﻿// -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
"use strict";

var { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import('resource://gre/modules/Geometry.jsm');

const ACCESSIBLECARET_PREF = "layout.accessiblecaret.enabled";
const TEST_URL = "http://mochi.test:8888/tests/robocop/testAccessibleCarets.html";

// Ensures Tabs are completely loaded, viewport and zoom constraints updated, etc.
const TAB_CHANGE_EVENT = "testAccessibleCarets:TabChange";
const TAB_STOP_EVENT = "STOP";

const gChromeWin = Services.wm.getMostRecentWindow("navigator:browser");

/**
 * Wait for and return, when an expected tab change event occurs.
 *
 * @param tabId, The id of the target tab we're observing.
 * @param eventType, The event type we expect.
 * @return {Promise}
 * @resolves The tab change object, including the matched tab id and event.
 */
function do_promiseTabChangeEvent(tabId, eventType) {
  return new Promise(resolve => {
    let observer = (subject, topic, data) => {
      let message = JSON.parse(data);

      if (message.event === eventType && message.tabId === tabId) {
        Services.obs.removeObserver(observer, TAB_CHANGE_EVENT);
        resolve(data);
      }
    }

    Services.obs.addObserver(observer, TAB_CHANGE_EVENT, false);
  });
}

/**
 * Selection methods vary if we have an input / textarea element,
 * or if we have basic content.
 */
function isInputOrTextarea(element) {
  return ((element instanceof Ci.nsIDOMHTMLInputElement) ||
          (element instanceof Ci.nsIDOMHTMLTextAreaElement));
}

/**
 * Return the selection controller based on element.
 */
function elementSelection(element) {
  return (isInputOrTextarea(element)) ?
    element.editor.selection :
    element.ownerDocument.defaultView.getSelection();
}

/**
 * Select the first character of a target element, w/o affecting focus.
 */
function selectElementFirstChar(doc, element) {
  if (isInputOrTextarea(element)) {
    element.setSelectionRange(0, 1);
    return;
  }

  // Simple test cases designed firstChild == #text node.
  let range = doc.createRange();
  range.setStart(element.firstChild, 0);
  range.setEnd(element.firstChild, 1);

  let selection = elementSelection(element);
  selection.removeAllRanges();
  selection.addRange(range);
}

/**
 * Get longpress point. Determine the midpoint in the first character of
 * the content in the element. X will be midpoint from left to right.
 * Y will be 1/3 of the height up from the bottom to account for both
 * LTR and smaller RTL characters. ie: |X| vs. |א|
 */
function getFirstCharPressPoint(doc, element, expected) {
  // Select the first char in the element.
  selectElementFirstChar(doc, element);

  // Reality check selected char to expected.
  let selection = elementSelection(element);
  is(selection.toString(), expected, "Selected char should match expected char.");

  // Return a point where long press should select entire word.
  let rect = selection.getRangeAt(0).getBoundingClientRect();
  let r = new Point(rect.left + (rect.width / 2), rect.bottom - (rect.height / 3));

  return r;
}

/**
 * Long press an element (RTL/LTR) at its calculated first character
 * position, and return the result.
 *
 * @param midPoint, The screen coord for the longpress.
 * @return {Promise}
 * @resolves The ActionBar status, including its target focused element, and
 *           the selected text that it sees.
 */
function getLongPressResult(browser, midPoint) {
  let domWinUtils = browser.contentWindow.
    QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);

  // AccessibleCarets expect longtap between touchstart/end.
  domWinUtils.sendTouchEventToWindow("touchstart", [0], [midPoint.x], [midPoint.y],
                                     [1], [1], [0], [1], 1, 0);
  domWinUtils.sendMouseEventToWindow("mouselongtap", midPoint.x, midPoint.y,
                                     0, 1, 0);
  domWinUtils.sendTouchEventToWindow("touchend", [0], [midPoint.x], [midPoint.y],
                                     [1], [1], [0], [1], 1, 0);

  let ActionBarHandler = gChromeWin.ActionBarHandler;
  return { focusedElement: ActionBarHandler._targetElement,
           text: ActionBarHandler._getSelectedText(),
           selectionID: ActionBarHandler._selectionID,
  };
}

/**
 * Main test method.
 */
add_task(function* testAccessibleCarets() {
  // Wait to start loading our test page until after the initial browser tab is
  // completely loaded. This allows each tab to complete its layer initialization,
  // importantly, its viewport and zoomContraints info.
  let BrowserApp = gChromeWin.BrowserApp;
  yield do_promiseTabChangeEvent(BrowserApp.selectedTab.id, TAB_STOP_EVENT);

  // Ensure Gecko Selection and Touch carets are enabled.
  Services.prefs.setBoolPref(ACCESSIBLECARET_PREF, true);

  // Load test page, wait for load completion, register cleanup.
  let browser = BrowserApp.addTab(TEST_URL).browser;
  let tab = BrowserApp.getTabForBrowser(browser);
  yield do_promiseTabChangeEvent(tab.id, TAB_STOP_EVENT);

  do_register_cleanup(function cleanup() {
    BrowserApp.closeTab(tab);
    Services.prefs.clearUserPref(ACCESSIBLECARET_PREF);
  });

  // References to test document elements.
  let doc = browser.contentDocument;
  let ce_LTR_elem = doc.getElementById("LTRcontenteditable");
  let tc_LTR_elem = doc.getElementById("LTRtextContent");
  let i_LTR_elem = doc.getElementById("LTRinput");
  let ta_LTR_elem = doc.getElementById("LTRtextarea");

  let ce_RTL_elem = doc.getElementById("RTLcontenteditable");
  let tc_RTL_elem = doc.getElementById("RTLtextContent");
  let i_RTL_elem = doc.getElementById("RTLinput");
  let ta_RTL_elem = doc.getElementById("RTLtextarea");

  // Locate longpress midpoints for test elements, ensure expactations.
  let ce_LTR_midPoint = getFirstCharPressPoint(doc, ce_LTR_elem, "F");
  let tc_LTR_midPoint = getFirstCharPressPoint(doc, tc_LTR_elem, "O");
  let i_LTR_midPoint = getFirstCharPressPoint(doc, i_LTR_elem, "T");
  let ta_LTR_midPoint = getFirstCharPressPoint(doc, ta_LTR_elem, "W");

  let ce_RTL_midPoint = getFirstCharPressPoint(doc, ce_RTL_elem, "א");
  let tc_RTL_midPoint = getFirstCharPressPoint(doc, tc_RTL_elem, "ת");
  let i_RTL_midPoint = getFirstCharPressPoint(doc, i_RTL_elem, "ל");
  let ta_RTL_midPoint = getFirstCharPressPoint(doc, ta_RTL_elem, "ה");


  // Longpress various LTR content elements. Test focused element against
  // expected, and selected text against expected.
  let result = getLongPressResult(browser, ce_LTR_midPoint);
  is(result.focusedElement, ce_LTR_elem, "Focused element should match expected.");
  is(result.text, "Find", "Selected text should match expected text.");

  result = getLongPressResult(browser, tc_LTR_midPoint);
  is(result.focusedElement, null, "No focused element is expected.");
  is(result.text, "Open", "Selected text should match expected text.");

  result = getLongPressResult(browser, i_LTR_midPoint);
  is(result.focusedElement, i_LTR_elem, "Focused element should match expected.");
  is(result.text, "Type", "Selected text should match expected text.");

  result = getLongPressResult(browser, ta_LTR_midPoint);
  is(result.focusedElement, ta_LTR_elem, "Focused element should match expected.");
  is(result.text, "Words", "Selected text should match expected text.");

  // Longpress various RTL content elements. Test focused element against
  // expected, and selected text against expected.
  result = getLongPressResult(browser, ce_RTL_midPoint);
  is(result.focusedElement, ce_RTL_elem, "Focused element should match expected.");
  is(result.text, "איפה", "Selected text should match expected text.");

  result = getLongPressResult(browser, tc_RTL_midPoint);
  is(result.focusedElement, null, "No focused element is expected.");
  is(result.text, "תן", "Selected text should match expected text.");

  result = getLongPressResult(browser, i_RTL_midPoint);
  is(result.focusedElement, i_RTL_elem, "Focused element should match expected.");
  is(result.text, "לרוץ", "Selected text should match expected text.");

  result = getLongPressResult(browser, ta_RTL_midPoint);
  is(result.focusedElement, ta_RTL_elem, "Focused element should match expected.");
  is(result.text, "הספר", "Selected text should match expected text.");

  ok(true, "Finished all tests.");
});

run_next_test();
