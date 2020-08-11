console.out("Sending lid open...");

include("scripts/CheckStates.js");

checkSend(["LID_CONTROL"]);
dim.send("LID_CONTROL/OPEN");
