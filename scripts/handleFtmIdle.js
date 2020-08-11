'use strict';

// Get ftm connected, idle and with working FTUs
function handleFtmIdle(wait_state)
{
    var state = dim.state("FTM_CONTROL");
    if (state===undefined)
        return undefined;

    // Only try to open the service if the server is already in the list
    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("FTM_CONTROL:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "Disconnected":
        console.out("Ftmctrl in 'Disconnected'... sending RECONNECT... waiting for 'Valid'.");
        dim.send("FTM_CONTROL/RECONNECT");
        return "Valid";

    case "Idle":
        console.out("Ftmctrl in 'Idle'... sending DISCONNECT... waiting for 'Disconnected'.");
        dim.send("FTM_CONTROL/DISCONNECT");
        v8.sleep(3000);
        return "Disconnected";

    case "Valid":
        return "";

    case "TriggerOn":
        console.out("Ftmctrl in 'TriggerOn'... sending STOP_TRIGGER... waiting for 'Valid'.");
        dim.send("FTM_CONTROL/STOP_TRIGGER");
        return "Valid";
 
    case "Configuring1":
    case "Configuring2":
    case "Configured1":
        console.out("Ftmctrl in '"+state.name+"'... sending RESET_CONFIGURE... waiting for 'Valid'.");
        dim.send("FTM_CONTROL/RESET_CONFIGURE");
        return "Valid";

    case "Configured2":
        return "TriggerOn";

    case "ConfigError1":
    case "ConfigError2":
    case "ConfigError3":
        throw new Error("FTM_CONTROL:"+state.name+"["+state.index+"] in error state.");
    }

    throw new Error("FTM_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
