'use strict';

function handleFeedbackConnected(wait_state)
{
    var state = dim.state("FEEDBACK");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("FEEDBACK:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "Disconnected":
    case "Connecting":
        return undefined;

    case "Connected":
    case "Calibrated":
        return "";

    case "WaitingForData":
    case "OnStandby":
    case "InProgress":
    case "Warning":
    case "Critical":
        console.out("Feedback in '"+state.name+"'... sending STOP... waiting for 'Calibrated'.");
        dim.send("FEEDBACK/STOP");
        return "Calibrated";

    case "Calibrating":
        console.out("Feedback in '"+state.name+"'... sending STOP... waiting for 'Connected'.");
        dim.send("FEEDBACK/STOP");
        return "Connected";
    }

    throw new Error("FEEDBACK:"+state.name+"["+state.index+"] unknown or not handled.");
}

