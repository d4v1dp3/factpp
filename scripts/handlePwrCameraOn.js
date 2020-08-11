'use strict';

// Switch interlock camera power on
function handlePwrCameraOn(wait_state)
{
    var state = dim.state("PWR_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("PWR_CONTROL:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    // Do-nothing conditions
    case "Disconnected":
    case "Connected":
    case "NoConnection":
        return undefined;

    // Drive off
    case "PowerOff":
        console.out("Pwrctrl in 'PowerOff'... sending CAMERA_POWER ON... waiting for 'DriveOff'.");
        dim.send("PWR_CONTROL/CAMERA_POWER", true);
        return "DriveOff";

    // Drive on
    case "DriveOn":
        console.out("Pwrctrl in 'DriveOn'... sending CAMERA_POWER ON... waiting for 'SystemOn'.");
        dim.send("PWR_CONTROL/CAMERA_POWER", true);
        return "SystemOn";

    // Intermediate states?
    case "CameraOn":
    case "BiasOn":
    case "CameraOff":
    case "BiasOff":
        return wait_state;

    case "DriveOff":
    case "SystemOn":
        // Now the agilent control need to be switched on!
        return "";

    case "CoolingFailure":
        throw new Error("Cooling unit reports failure... please check.");
    }

    throw new Error("PWR_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
