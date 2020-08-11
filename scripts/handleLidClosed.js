'use strict';

// Get Lids closed
function handleLidClosed(wait_state)
{
    var state = dim.state("LID_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("LID_CONTROL:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    // Do-nothing conditions
    case "NotReady":
    case "Ready":
    case "NoConnection":
    case "Connected":
    case "Moving":
    case "UpperClosing":
        return wait_state;

    case "Unknown":
    case "Inconsistent":
    case "PowerProblem":
    case "Open":
    case "Opening":
        console.out("Lidctrl in '"+state.name+"'... sending CLOSE... waiting for 'Closed'.");
        dim.send("LID_CONTROL/CLOSE");
        return "Closed";

    case "Closed":
        return "";
    }

    throw new Error("LID_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
