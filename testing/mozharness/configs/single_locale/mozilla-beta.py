config = {
    "nightly_build": True,
    "branch": "mozilla-beta",
    "en_us_binary_url": "http://ftp.mozilla.org/pub/mozilla.org/firefox/nightly/latest-mozilla-beta/",
    "update_channel": "beta",
    "latest_mar_dir": '/pub/mozilla.org/firefox/nightly/latest-mozilla-beta-l10n',

    # l10n
    "hg_l10n_base": "https://hg.mozilla.org/releases/l10n/mozilla-beta",

    # repositories
    "mozilla_dir": "mozilla-beta",
    "repos": [{
        "vcs": "hg",
        "repo": "https://hg.mozilla.org/build/tools",
        "revision": "default",
        "dest": "tools",
    }, {
        "vcs": "hgtool",
        "repo": "https://hg.mozilla.org/releases/mozilla-beta",
        "revision": "default",
        "dest": "mozilla-beta",
    }, {
        "vcs": "hgtool",
        "repo": "https://hg.mozilla.org/build/compare-locales",
        "revision": "RELEASE_AUTOMATION"
    }],
    # purge options
    'purge_minsize': 12,
    'is_automation': True,
    'default_actions': [
        "clobber",
        "pull",
        "list-locales",
        "setup",
        "repack",
        "taskcluster-upload",
        "summary",
    ],
}
