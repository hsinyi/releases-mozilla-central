function test_fragment_play(v, start, end, is, ok, finish) {

var completed = false;
var loadedMetadataRaised = false;
var seekedRaised = false;
var pausedRaised = false;

function onLoadedMetadata() {
  var s = start == null ? 0 : start;
  var e = end == null ? v.duration : end;
  ok(v.currentTime == s, "loadedmetadata currentTime is " + v.currentTime + " != " + s);
  ok(v.initialTime == s, "initialTime (" + v.initialTime + ") == start Time (" + s + ")");
  ok(v.mozFragmentEnd == e, "mozFragmentEnd (" + v.mozFragmentEnd + ") == end Time (" + e + ")");
  loadedMetadataRaised = true; 
  v.play();
}

function onSeeked() {
  if (completed)
    return;

  var s = start == null ? 0 : start;
  ok(v.currentTime == s, "seeked currentTime is " + v.currentTime + " != " + s);

  seekedRaised = true;
}

function onTimeUpdate() {
  if (completed)
    return;

  v._lastTimeUpdate = v.currentTime;
}

function onPause() {
  if (completed)
    return;

  var e = end == null ? v.duration : end;
  var a = e - 0.05;
  var b = e + 0.05;
  ok(v.currentTime >= a && v.currentTime <= b, "paused currentTime is " + a + " < " + v.currentTime + " < " + b + " ? " + v._lastTimeUpdate);
  pausedRaised = true;
  v.play();
}


function onEnded() {
  if (completed)
    return;

  completed = true;
  ok(loadedMetadataRaised, "loadedmetadata event");
  if (start) {
    ok(seekedRaised, "seeked event");
  }
  if (end) {
    ok(pausedRaised, "paused event: " + end + " " + v.duration);
  }
  finish();
}

v.addEventListener("ended", onEnded, false);
v.addEventListener("loadedmetadata", onLoadedMetadata, false);
v.addEventListener("seeked", onSeeked, false);
v.addEventListener("pause", onPause, false);
v.addEventListener("timeupdate", onTimeUpdate, false);
}
