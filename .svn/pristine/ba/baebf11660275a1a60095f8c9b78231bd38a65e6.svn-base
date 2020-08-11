'use strict';

// Get bias control connected and voltage off
function handleBiasVoltageOff(wait_state)
{
    var state = dim.state("BIAS_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("BIAS_CONTROL: "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    // Do-nothing conditions
    case "Connecting":
    case "Initializing":
    case "Connected":
    case "Ramping":
        return wait_state;

    case "Locked":
        console.out("WARNING - Bias is LOCKED. Please report, this is serious, unlock manually and go on.");
        return "";

    // Do-something conditions
    case "Disconnected":
        console.out("Bias in Disconnected... connect.");
        dim.send("BIAS_CONTROL/RECONNECT");
        return "VoltageOff";

    case "NotReferenced":
    case "VoltageOn":
        console.out("Bias in "+state.name+"... switch voltage off.");
        dim.send("BIAS_CONTROL/SET_ZERO_VOLTAGE");
        return "VoltageOff";

    // Final state reached condition
    case "VoltageOff":
        return "";

    // Conditions which cannot be handled
    case "OverCurrent": throw "BIAS_CONTROL in OverCurrent";
    case "ExpertMode":  throw "BIAS_CONTROL in expert mode";
    }

    throw new Error("BIAS_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
