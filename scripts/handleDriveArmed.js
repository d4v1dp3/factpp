'use strict';

// Get Drive control armed
function handleDriveArmed(wait_state)
{
    var state = dim.state("DRIVE_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("DRIVE_CONTROL: "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "Disconnected":
        console.out("Drivectrl in 'Disconnected'... sending RECONNECT... waiting for 'Initialized'.");
        dim.send("DRIVE_CONTROL/RECONNECT");
        return "Initialized";

    case "Connected":
    case "Parking":
    case "Stopping":
    case "Armed":
    case "Blocked":
        v8.sleep(1000);
        return undefined;

    case "Locked":
        console.out("WARNING - Drive is LOCKED. Please unlock manually.");
        // Do NOT unlock the drive here... it is a safety feature which should
        // never be used in an auotmatic process otherwise it is not realiable!
        return "";

    case "Moving":
    case "Tracking":
    case "OnTrack":
        console.out("Drive in '"+state.name+"'... sending STOP... waiting for 'Initialized'.");
        dim.send("DRIVE_CONTROL/STOP");
        return "Initialized";

    case "ERROR":
        console.out("Drive in '"+state.name+"'... sending STOP... waiting for 'Initialized'");
        dim.send("DRIVE_CONTROL/STOP");
        return "Initialized";  // Process that again if necessary

    case "Initialized":
        return "";
    }

    throw new Error("DRIVE_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
