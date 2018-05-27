/* -*- Mode: Java; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.restrictions;

import org.mozilla.gecko.AboutPages;
import org.mozilla.gecko.util.ThreadUtils;

import android.annotation.TargetApi;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.StrictMode;
import android.os.UserManager;

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR2)
public class RestrictedProfileConfiguration implements RestrictionConfiguration {
    // Mapping from restrictable feature to default state (on/off)
    private static Map<Restrictable, Boolean> configuration = new LinkedHashMap<>();
    static {
        configuration.put(Restrictable.INSTALL_EXTENSION, false);
        configuration.put(Restrictable.PRIVATE_BROWSING, false);
        configuration.put(Restrictable.CLEAR_HISTORY, false);
        configuration.put(Restrictable.MASTER_PASSWORD, false);
        configuration.put(Restrictable.GUEST_BROWSING, false);
        configuration.put(Restrictable.ADVANCED_SETTINGS, false);
        configuration.put(Restrictable.CAMERA_MICROPHONE, false);
        configuration.put(Restrictable.DATA_CHOICES, false);
        configuration.put(Restrictable.TELEMETRY, false);
        configuration.put(Restrictable.HEALTH_REPORT, true);
        configuration.put(Restrictable.DEFAULT_THEME, true);
    }

    /**
     * These restrictions are hidden from the admin configuration UI.
     */
    private static List<Restrictable> hiddenRestrictions = Arrays.asList(
            Restrictable.MASTER_PASSWORD,
            Restrictable.GUEST_BROWSING,
            Restrictable.DATA_CHOICES,
            Restrictable.DEFAULT_THEME
    );

    /* package-private */ static boolean shouldHide(Restrictable restrictable) {
        return hiddenRestrictions.contains(restrictable);
    }

    /* package-private */ static Map<Restrictable, Boolean> getConfiguration() {
        return configuration;
    }

    private Context context;
    private Bundle cachedAppRestrictions;
    private Bundle cachedUserRestrictions;
    private boolean isCacheInvalid = true;

    public RestrictedProfileConfiguration(Context context) {
        this.context = context.getApplicationContext();
    }

    @Override
    public synchronized boolean isAllowed(Restrictable restrictable) {
        if (isCacheInvalid || !ThreadUtils.isOnUiThread()) {
            readRestrictions();
            isCacheInvalid = false;
        }

        // Special casing system/user restrictions
        if (restrictable == Restrictable.INSTALL_APPS || restrictable == Restrictable.MODIFY_ACCOUNTS) {
            return !cachedUserRestrictions.getBoolean(restrictable.name);
        }

        if (!cachedAppRestrictions.containsKey(restrictable.name) && !configuration.containsKey(restrictable)) {
            // Always allow features that are not in the configuration
            return true;
        }

        return cachedAppRestrictions.getBoolean(restrictable.name, configuration.get(restrictable));
    }

    private void readRestrictions() {
        final UserManager mgr = (UserManager) context.getSystemService(Context.USER_SERVICE);

        StrictMode.ThreadPolicy policy = StrictMode.allowThreadDiskReads();

        try {
            Bundle appRestrictions = mgr.getApplicationRestrictions(context.getPackageName());
            migrateRestrictionsIfNeeded(appRestrictions);

            cachedAppRestrictions = appRestrictions;
            cachedUserRestrictions = mgr.getUserRestrictions();
        } finally {
            StrictMode.setThreadPolicy(policy);
        }
    }

    @Override
    public boolean canLoadUrl(String url) {
        if (!isAllowed(Restrictable.INSTALL_EXTENSION) && AboutPages.isAboutAddons(url)) {
            return false;
        }

        if (!isAllowed(Restrictable.PRIVATE_BROWSING) && AboutPages.isAboutPrivateBrowsing(url)) {
            return false;
        }

        if (AboutPages.isAboutConfig(url)) {
            // Always block access to about:config to prevent circumventing restrictions (Bug 1189233)
            return false;
        }

        return true;
    }

    @Override
    public boolean isRestricted() {
        return true;
    }

    @Override
    public synchronized void update() {
        isCacheInvalid = true;
    }

    /**
     * This method migrates the old set of DISALLOW_ restrictions to the new restrictable feature ones (Bug 1189336).
     */
    public static void migrateRestrictionsIfNeeded(Bundle bundle) {
        if (!bundle.containsKey(Restrictable.INSTALL_EXTENSION.name) && bundle.containsKey("no_install_extensions")) {
            bundle.putBoolean(Restrictable.INSTALL_EXTENSION.name, !bundle.getBoolean("no_install_extensions"));
        }

        if (!bundle.containsKey(Restrictable.PRIVATE_BROWSING.name) && bundle.containsKey("no_private_browsing")) {
            bundle.putBoolean(Restrictable.PRIVATE_BROWSING.name, !bundle.getBoolean("no_private_browsing"));
        }

        if (!bundle.containsKey(Restrictable.CLEAR_HISTORY.name) && bundle.containsKey("no_clear_history")) {
            bundle.putBoolean(Restrictable.CLEAR_HISTORY.name, !bundle.getBoolean("no_clear_history"));
        }

        if (!bundle.containsKey(Restrictable.ADVANCED_SETTINGS.name) && bundle.containsKey("no_advanced_settings")) {
            bundle.putBoolean(Restrictable.ADVANCED_SETTINGS.name, !bundle.getBoolean("no_advanced_settings"));
        }
    }
}
