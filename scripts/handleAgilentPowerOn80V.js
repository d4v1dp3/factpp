'use strict';

// switch agilent control output on
function handleAgilentPowerOn(wait_state)
{
    var state = dim.state("AGILENT_CONTROL_80V");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("AGILENT_CONTROL:  "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "Disconnected":
    case "Connected":
        return undefined;

    case "VoltageLow":
        return wait_state;

    case "VoltageOff":
        console.out("Agilent in 'VoltageOff'... sending SET_POWER ON... waiting for 'VoltageOn'.");
        dim.send("AGILENT_CONTROL_80V/SET_POWER", true);
        return "VoltageOn";

    case "VoltageOn":
        return "";

    case "VoltageHigh":
        throw new Error("Agilent reports voltage above limit ('VoltageHigh')... please check.");
    }

    throw new Error("AGILENT_CONTROL_80V:"+state.name+"["+state.index+"] unknown or not handled.");
}
