'use strict';

function handleScheduler(wait_state)
{
    var state = dim.state("SCHEDULER");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    switch (state.name)
    {
    // Do-nothing conditions
    case "Disconnected":
        throw new Error("SCHEDULER: No connection to SQL database!");
        return "";

    case "Armed":
        return "";

    case "Connected":
        dim.send("SCHEDULER/START");
        return "Armed";

    }

    throw new Error("SCHEDULER:"+state.name+"["+state.index+"] unknown or not handled.");
}
