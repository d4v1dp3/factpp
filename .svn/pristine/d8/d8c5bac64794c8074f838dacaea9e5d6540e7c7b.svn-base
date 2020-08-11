var zd = $['zd'];
var az = $['az'];

if (isNaN(zd) || zd<-100 || zd>100)
    throw new Error("Invalid zenith distance!");

if (isNaN(az) || az<-290 || az>80)
    throw new Error("Invalid azimuth!");

console.out("Moving telescope to zd="+zd+"deg, az="+az+"deg");

include("scripts/CheckStates.js");

checkSend(["DRIVE_CONTROL"]);
dim.send("DRIVE_CONTROL/MOVE_TO", zd, az);
