console.out("Sending lid close...");

include("scripts/CheckStates.js");

checkSend(["LID_CONTROL"]);
dim.send("LID_CONTROL/CLOSE");
