/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 */
"use strict";

Components.utils.import("resource:///modules/NewTabURL.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AppConstants.jsm");

add_task(function* () {
  Assert.equal(NewTabURL.get(), "about:newtab", "Default newtab URL should be about:newtab");
  let url = "http://example.com/";
  let notificationPromise = promiseNewtabURLNotification(url);
  NewTabURL.override(url);
  yield notificationPromise;
  Assert.ok(NewTabURL.overridden, "Newtab URL should be overridden");
  Assert.equal(NewTabURL.get(), url, "Newtab URL should be the custom URL");

  notificationPromise = promiseNewtabURLNotification("about:newtab");
  NewTabURL.reset();
  yield notificationPromise;
  Assert.ok(!NewTabURL.overridden, "Newtab URL should not be overridden");
  Assert.equal(NewTabURL.get(), "about:newtab", "Newtab URL should be the about:newtab");

  // change newtab page to remote
  Services.prefs.setBoolPref("browser.newtabpage.remote", true);
  let expected = AppConstants.RELEASE_BUILD ? "about:newtab" : "about:remote-newtab";
  Assert.equal(NewTabURL.get(), expected, "Newtab URL should be the " + expected);
  Assert.ok(!NewTabURL.overridden, "Newtab URL should not be overridden");
});

function promiseNewtabURLNotification(aNewURL) {
  return new Promise(resolve => {
    Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
      Services.obs.removeObserver(observer, aTopic);
      Assert.equal(aData, aNewURL, "Data for newtab-url-changed notification should be new URL.");
      resolve();
    }, "newtab-url-changed", false);
  });
}
