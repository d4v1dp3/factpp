var wobble = $['wobble']=='true' ? 2 : 1;
var source = $['source'];

console.out("Start tracking wobble position "+wobble+" of "+source);

include("scripts/CheckStates.js");

checkSend(["DRIVE_CONTROL"]);
dim.send("DRIVE_CONTROL/TRACK_WOBBLE", wobble, source);
