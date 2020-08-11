'use strict';

// Get FSC connected
function handleFscConnected(wait_state)
{
    var state = dim.state("FSC_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //console.out("FSC_CONTROL:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    // Do-nothing conditions
    case "Disconnected":
        console.out("Fscctrl in 'Disconnected'... sending RECONNECT... waiting for 'Connected'.");
        dim.send("FSC_CONTROL/RECONNECT");
        return "Connected";

    case "Connected":
        return "";
    }

    throw new Error("FSC_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
