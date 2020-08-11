var ra = $['ra'];
var dec = $['dec'];

console.out("Start tracking at ra="+ra+"h, dec="+dec+"deg");

include("scripts/CheckStates.js");

checkSend(["DRIVE_CONTROL"]);
dim.send("DRIVE_CONTROL/TRACK", ra, dec);
