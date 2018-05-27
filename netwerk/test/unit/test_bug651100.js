function gen_1MiB()
{
  var i;
  var data="x";
  for (i=0 ; i < 20 ; i++)
    data+=data;
  return data;
}

function write_and_check(str, data, len)
{
  var written = str.write(data, len);
  if (written != len) {
    do_throw("str.write has not written all data!\n" +
             "  Expected: " + len  + "\n" +
             "  Actual: " + written + "\n");
  }
}

function write_big_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  // write 65MiB
  var i;
  for (i=0 ; i<65 ; i++)
    write_and_check(os, data, data.length);

  // another write should fail and the entry will be doomed
  try {
    write_and_check(os, data, data.length);
    do_throw("write should fail");
  } catch (e) {}

  os.close();
  entry.close();

  // DoomEntry() is called while writing to the entry, but the data is really
  // deleted (and the cache size updated) on the background thread when
  // the entry is deactivated. We need to sync with the cache IO thread before
  // we continue with the test.
  syncWithCacheIOThread(run_test_2);
}

function write_big_metafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = gen_1MiB();

  // > 64MiB
  var i;
  for (i=0 ; i<65 ; i++)
    entry.setMetaDataElement("metadata_"+i, data);

  entry.metaDataReady();

  os.close();
  entry.close();

  // We don't check whether the cache is full while writing metadata. Also we
  // write the metadata when closing the entry, so we need to write some data
  // after closing this entry to invoke the cache cleanup.
  asyncOpenCacheEntry("http://smalldata/",
                      "disk", Ci.nsICacheStorage.OPEN_TRUNCATE, null,
                      write_and_doom_small_datafile);
}

function write_and_doom_small_datafile(status, entry)
{
  do_check_eq(status, Cr.NS_OK);
  var os = entry.openOutputStream(0);
  var data = "0123456789";

  write_and_check(os, data, data.length);

  os.close();
  entry.asyncDoom(null);
  entry.close();
  syncWithCacheIOThread(run_test_3);
}

function check_cache_size(cont) {
  get_device_entry_count("disk", null, function(count, consumption) {
    // Because the last entry we store is doomed using AsyncDoom and not Doom, it is still active
    // during the visit processing, hence consumption is larger then 0 (one block is allocated).
    // ...I really like all these small old-cache bugs, that will finally go away... :)
    do_check_true(consumption <= 1024)
    cont();
  });
}

function run_test() {
  if (newCacheBackEndUsed()) {
    // browser.cache.disk.* (limits mainly) tests
    do_check_true(true, "This test doesn't run with the new cache backend, the test or the cache needs to be fixed");
    return;
  }

  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

  // set max entry size bigger than 64MiB
  prefBranch.setIntPref("browser.cache.disk.max_entry_size", 65*1024);
  // disk cache capacity must be at least 8 times bigger
  prefBranch.setIntPref("browser.cache.disk.capacity", 8*65*1024);
  // disable smart size
  prefBranch.setBoolPref("browser.cache.disk.smart_size.enabled", false);

  do_get_profile();

  // clear the cache
  evict_cache_entries();

  // write an entry with data > 64MiB
  asyncOpenCacheEntry("http://bigdata/",
                      "disk", Ci.nsICacheStorage.OPEN_TRUNCATE, null,
                      write_big_datafile);

  do_test_pending();
}

function run_test_2()
{
  check_cache_size(run_test_2a);
}

function run_test_2a()
{
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

  // set cache capacity lower than max entry size (see comment in
  // write_big_metafile)
  prefBranch.setIntPref("browser.cache.disk.capacity", 64*1024);

  // write an entry with metadata > 64MiB
  asyncOpenCacheEntry("http://bigmetadata/",
                      "disk", Ci.nsICacheStorage.OPEN_TRUNCATE, null,
                      write_big_metafile);
}

function run_test_3()
{
  check_cache_size(do_test_finished);
}
