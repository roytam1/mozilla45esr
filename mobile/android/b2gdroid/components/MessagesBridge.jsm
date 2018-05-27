/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

this.EXPORTED_SYMBOLS = ["MessagesBridge"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SystemAppProxy.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "settings",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

// This module receives messages from Launcher.java as observer notifications.
// It also listens for settings changes to relay them back to Android.

function debug() {
  dump("-*- MessagesBridge " + Array.slice(arguments) + "\n");
}

function getWindow() {
  return SystemAppProxy.getFrame().contentWindow ||
         Services.wm.getMostRecentWindow("navigator:browser");
}

// To prevent roundtrips like android -> gecko -> android we keep track of
// in flight setting changes.
var _blockedSettings = new Set();

this.MessagesBridge = {
  init: function() {
    Services.obs.addObserver(this.onAndroidMessage, "Android:Launcher", false);
    Services.obs.addObserver(this.onAndroidSetting, "Android:Setting", false);
    Services.obs.addObserver(this.onSettingChange, "mozsettings-changed", false);
    Services.obs.addObserver(this.onAndroidNotification, "Android:Notification", false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);

    SystemAppProxy.addEventListener("mozContentNotificationEvent", this);

    // Send a request to get the device's IMEI.
    Messaging.sendRequestForResult({ type: "Android:GetIMEI" })
    .then(aData => {
      debug("Got IMEI: " + aData.imei);
      let lock = settings.createLock();
      lock.set("deviceinfo.imei", aData.imei, null);
    });
  },

  handleEvent: function(evt) {
    let detail = evt.detail;

    switch(detail.type) {
      case "desktop-notification-click":
        debug("Sending Android:NotificationOpened");
        Messaging.sendRequest({ type: "Android:NotificationOpened", value: { id: detail.id }});
        break;
      case "desktop-notification-close":
        // On receipt of a notification close, send the id to the Android layer
        // so it can be removed from the Android NotificationManager.
        debug("Sending Android:NotificationClosed");
        Messaging.sendRequest({ type: "Android:NotificationClosed", value: { id: detail.id }});
        break;
    }
  },

  onAndroidNotification: function(aSubject, aTopic, aData) {
    let data = JSON.parse(aData);
    debug("Got android notification: " + data._action);
    switch(data._action) {
      case "post":
        debug("showNotification(id=" + data.id + ")");
        showNotification(data);
        break;
      case "remove":
        debug("removeNotification(id=" + data.id + ")");
        removeNotification(data);
        break;
    }
  },

  onAndroidMessage: function(aSubject, aTopic, aData) {
    let data = JSON.parse(aData);
    debug(`Got Android:Launcher message ${data.action}`);

    let window = getWindow();
    switch (data.action) {
      case "screen_on":
      case "screen_off":
        // In both cases, make it look like pressing the power button
        // by dispatching keydown & keyup on the system app window.
        window.dispatchEvent(new window.KeyboardEvent("keydown", { key: "Power" }));
        window.dispatchEvent(new window.KeyboardEvent("keyup", { key: "Power" }));
        break;
      case "view":
        let a = new window.MozActivity({ name: "view",
                                         data: { type: "url",
                                                 url: data.url } });
        break;
      case "send_sms":
        new window.MozActivity({
          name: "new",
          data: {
            type: "websms/sms",
            number: data.number,
            body: data.body
          }
        });
        break;
      case "home-key":
        window.dispatchEvent(new window.KeyboardEvent("keydown", { key: "Home" }));
        window.dispatchEvent(new window.KeyboardEvent("keyup", { key: "Home" }));
        break;
      case "task-switcher":
        window.dispatchEvent(new window.CustomEvent("taskmanagershow", {}));
        break;
      case "back-key":
        Services.obs.notifyObservers(null, "back-docommand", null);
        break;
    }
  },

  onAndroidSetting: function(aSubject, aTopic, aData) {
    let data = JSON.parse(aData);
    let lock = settings.createLock();
    let key = Object.keys(data)[0];
    debug(`Got Android:Setting message ${key} -> ${data[key]}`);
    // Don't relay back to android the same setting change.
    _blockedSettings.add(key);
    lock.set(key, data[key], null);
  },

  onSettingChange: function(aSubject, aTopic, aData) {
    if ("wrappedJSObject" in aSubject) {
      aSubject = aSubject.wrappedJSObject;
    }
    if (aSubject) {
      debug("Got setting change: " + aSubject.key + " -> " + aSubject.value);

      if (_blockedSettings.has(aSubject.key)) {
        _blockedSettings.delete(aSubject.key);
        debug("Rejecting blocked setting change for " + aSubject.key);
        return;
      }

      let window = getWindow();

      if (aSubject.value instanceof window.Blob) {
        debug(aSubject.key + " is a Blob");
        let reader = new window.FileReader();
        reader.readAsDataURL(aSubject.value);
        reader.onloadend = function() {
          Messaging.sendRequest({ type: "Settings:Change",
                                  setting: aSubject.key,
                                  value: reader.result });
        }
      } else {
        Messaging.sendRequest({ type: "Settings:Change",
                                setting: aSubject.key,
                                value: aSubject.value });
      }
    }
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      Services.obs.removeObserver(this.onAndroidMessage, "Android:Launcher");
      Services.obs.removeObserver(this.onAndroidSetting, "Android:Setting");
      Services.obs.removeObserver(this.onSettingChange, "mozsettings-changed");
      Services.obs.removeObserver(this.onAndroidNotification, "Android:Notification");
      Services.obs.removeObserver(this, "xpcom-shutdown");
    }
  }
}

function removeNotification(aDetail) {
  // Remove the notification
  SystemAppProxy._sendCustomEvent("mozChromeNotificationEvent", {
    type: "desktop-notification-close",
    id: aDetail.id
  });
}

function showNotification(aDetail) {
  const manifestURL = aDetail.manifestURL;

  function send(appName) {
    aDetail.type = "desktop-notification";
    aDetail.appName = appName;

    SystemAppProxy._sendCustomEvent("mozChromeNotificationEvent", aDetail);
  }

  if (!manifestURL|| !manifestURL.length) {
    send(null);
    return;
  }

  // If we have a manifest URL, get the app title from the manifest
  // to prevent spoofing.
  appsService.getManifestFor(manifestURL).then((manifest) => {
    let app = appsService.getAppByManifestURL(manifestURL);

    // Sometimes an android notification may be created by a service rather
    // than an app. In this case, don't use the appName.
    if (!app) {
      send(null);
      return;
    }

    let helper = new ManifestHelper(manifest, app.origin, manifestURL);
    send(helper.name);
  });
}

this.MessagesBridge.init();
