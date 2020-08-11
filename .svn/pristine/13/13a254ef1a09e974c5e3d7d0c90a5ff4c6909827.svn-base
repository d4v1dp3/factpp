'use strict';

function handleFtuCheck(wait_state)
{
    var service = this.ftuList.get();
    if (!service.obj || service.obj.length==0)
        return undefined;

    if (!wait_state)
    {
        dim.send("FTM_CONTOL/PING");
        return toString(service.counter);
    }

    if (toInt(wait_state)==service.counter)
        return wait_state;

    var ping = service.data['Ping'];
    for (var i=0; i<40; i++)
    {
        if (ping[i]==1)
            continue;

        dim.log("Problems in the FTU communication found.");
        dim.log("Send command to disable all FTUs.");
        dim.log(" => Power cycle needed.");
        dim.send("FTM_CONTOL/ENABLE_FTU", -1, false);
        throw new Error("CrateReset[FTU]");
    }

    return "";
}
