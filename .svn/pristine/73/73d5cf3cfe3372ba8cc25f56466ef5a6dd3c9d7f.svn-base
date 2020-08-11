'use strict';

function handleRatectrlConnected(wait_state)
{
    var state = dim.state("RATE_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("RATE_CONTROL: "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "DimNetworkNotAvailable":
    case "Disconnected":
        return undefined;

    case "Calibrating":
    case "GlobalThresholdSet":
    case "InProgress":
        console.out("Ratectrl in '"+state.name+"'... sending STOP... waiting for 'Connected'.");
        dim.send("RATE_CONTROL/STOP");
        return "Connected";

    case "Connected":
        return "";
    }

    throw new Error("RATE_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}

