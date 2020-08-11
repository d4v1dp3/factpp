var offset = parseFloat($['offset']);
var angle  = parseFloat($['wobble']);
var source = $['source'];

if (isNaN(offset) || offset<0)
    throw new Error("Invalid wobble offset!");

if (isNaN(angle) || angle<0 || angle>360)
    throw new Error("Invalid wobble angle!");

console.out("Start tracking source "+source+" at wobble angle "+angle+"deg and offset "+offset+"deg");

include("scripts/CheckStates.js");

checkSend(["DRIVE_CONTROL"]);
dim.send("DRIVE_CONTROL/TRACK_SOURCE", offset, angle, source);
