/* -*- Mode: Java; c-basic-offset: 4; tab-width: 20; indent-tabs-mode: nil; -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.mozilla.gecko.dlc.catalog;

import org.mozilla.gecko.AppConstants;
import org.mozilla.gecko.dlc.catalog.DownloadContent;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/* package-private */ class DownloadContentBootstrap {
    public static List<DownloadContent> createInitialDownloadContentList() {
        if (!AppConstants.MOZ_ANDROID_EXCLUDE_FONTS) {
            // We are packaging fonts. There's nothing we want to download;
            return Collections.emptyList();
        }

        return Arrays.asList(
                new DownloadContent.Builder()
                        .setId("bff50e08-7bbc-4d77-a907-bb0a54434bee")
                        .setLocation("fonts/CharisSILCompact-B.ttf.gz")
                        .setFilename("CharisSILCompact-B.ttf")
                        .setChecksum("699d958b492eda0cc2823535f8567d0393090e3842f6df3c36dbe7239cb80b6d")
                        .setDownloadChecksum("ff7ecae7669a51d5fa6a5f8e703278ebda3a68f51bc49c4321bde4438020d639")
                        .setSize(1676072)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("68c6472d-94a6-4fb2-8525-78e427b022fe")
                        .setLocation("fonts/CharisSILCompact-BI.ttf.gz")
                        .setFilename("CharisSILCompact-BI.ttf")
                        .setChecksum("82465e747b4f41471dbfd942842b2ee810749217d44b55dbc43623b89f9c7d9b")
                        .setDownloadChecksum("dfb6d583edd27d5e6d91d479e6c8a5706275662c940c65b70911493bb279904a")
                        .setSize(1667812)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("33d0ce0d-9c48-4a37-8b74-81cce872061b")
                        .setLocation("fonts/CharisSILCompact-I.ttf.gz")
                        .setFilename("CharisSILCompact-I.ttf")
                        .setChecksum("ab3ed6f2a4d4c2095b78227bd33155d7ccd05a879c107a291912640d4d64f767")
                        .setDownloadChecksum("5a257ec3c5226e7be0be65e463f5b22eff108da853b9ff7bc47f1733b1ddacf2")
                        .setSize(1693988)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("7e274cdc-4216-4dc0-b7a5-8ec333f0c287")
                        .setLocation("fonts/CharisSILCompact-R.ttf.gz")
                        .setFilename("CharisSILCompact-R.ttf")
                        .setChecksum("4ed509317f1bb441b185ea13bf1c9d19d1a0b396962efa3b5dc3190ad88f2067")
                        .setDownloadChecksum("cab284228b8dfe8ef46c3f1af70b5b6f9e92878f05e741ecc611e5e750a4a3b3")
                        .setSize(1727656)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("b144002f-d5de-448c-8952-da1e405e022f")
                        .setLocation("fonts/ClearSans-Bold.ttf.gz")
                        .setFilename("ClearSans-Bold.ttf")
                        .setChecksum("385d0a293c1714770e198f7c762ab32f7905a0ed9d2993f69d640bd7232b4b70")
                        .setDownloadChecksum("d95168996dc932e6504cb5448fcb759e0ee6e66c5c8603293b046d28ab589cce")
                        .setSize(140136)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("f07502f5-e4c5-41a8-8788-89717397a98d")
                        .setLocation("fonts/ClearSans-BoldItalic.ttf.gz")
                        .setFilename("ClearSans-BoldItalic.ttf")
                        .setChecksum("7bce66864e38eecd7c94b6657b9b38c35ebfacf8046bfb1247e08f07fe933198")
                        .setDownloadChecksum("f5e18f4acc4ceaeca9e081b1be79cd6034e0dc7ad683fa240195fd6c838452e0")
                        .setSize(156124)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("afafc7ef-f516-42da-88d4-d8435f65541b")
                        .setLocation("fonts/ClearSans-Italic.ttf.gz")
                        .setFilename("ClearSans-Italic.ttf")
                        .setChecksum("87c13c5fbae832e4f85c3bd46fcbc175978234f39be5fe51c4937be4cbff3b68")
                        .setDownloadChecksum("56d12114ac15d913d7d9876c698889cd25f26e14966a8bd7424aeb0f61ffaf87")
                        .setSize(155672)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("28521d9b-ac2e-45d0-89b6-a4c9076dbf6d")
                        .setLocation("fonts/ClearSans-Light.ttf.gz")
                        .setFilename("ClearSans-Light.ttf")
                        .setChecksum("e4885f6188e7a8587f5621c077c6c1f5e8d3739dffc8f4d055c2ba87568c750a")
                        .setDownloadChecksum("1fc716662866b9c01e32dda3fc9c54ca3e57de8c6ac523f46305d8ae6c0a9cf4")
                        .setSize(145976)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("13f01bf4-da71-4673-9c60-ec0e9a45c38c")
                        .setLocation("fonts/ClearSans-Medium.ttf.gz")
                        .setFilename("ClearSans-Medium.ttf")
                        .setChecksum("5d0e0115f3a3ed4be3eda6d7eabb899bb9a361292802e763d53c72e00f629da1")
                        .setDownloadChecksum("a29184ec6621dbd3bc6ae1e30bba70c479d1001bca647ea4a205ecb64d5a00a0")
                        .setSize(148892)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("73104370-c7ee-4b5b-bb37-392a4e66f65a")
                        .setLocation("fonts/ClearSans-MediumItalic.ttf.gz")
                        .setFilename("ClearSans-MediumItalic.ttf")
                        .setChecksum("937dda88b26469306258527d38e42c95e27e7ebb9f05bd1d7c5d706a3c9541d7")
                        .setDownloadChecksum("a381a3d4060e993af440a7b72fed29fa3a488536cc451d7c435d5fae1256318b")
                        .setSize(155228)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("274f3718-f6e0-40b4-b52a-44812ec3ea9e")
                        .setLocation("fonts/ClearSans-Regular.ttf.gz")
                        .setFilename("ClearSans-Regular.ttf")
                        .setChecksum("9b91bbdb95ffa6663da24fdaa8ee06060cd0a4d2dceaf1ffbdda00e04915ee5b")
                        .setDownloadChecksum("87dec7f0331e19b293fc510f2764b9bd1b94595ac279cf9414f8d03c5bf34dca")
                        .setSize(142572)
                        .setKind("font")
                        .setType("asset-archive")
                        .build(),

                new DownloadContent.Builder()
                        .setId("77803858-3cfb-4a0d-a1d3-fa1bf8a6c604")
                        .setLocation("fonts/ClearSans-Thin.ttf.gz")
                        .setFilename("ClearSans-Thin.ttf")
                        .setChecksum("07b0db85a3ad99afeb803f0f35631436a7b4c67ac66d0c7f77d26a47357c592a")
                        .setDownloadChecksum("64300b48b2867e5642212690f0ff9ea3988f47790311c444a81d25213b4102aa")
                        .setSize(147004)
                        .setKind("font")
                        .setType("asset-archive")
                        .build()
        );
    }
}
