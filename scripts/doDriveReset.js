console.out("Sending drive reset.");

include("scripts/CheckStates.js");

checkSend(["DRIVE_CONTROL"]);
dim.send("DRIVE_CONTROL/RESET");
