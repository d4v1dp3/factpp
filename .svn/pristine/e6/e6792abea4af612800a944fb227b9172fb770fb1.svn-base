'use strict';

// ==========================================================================
// Reset Crate
// ==========================================================================

// call it with: .js doCrateReset.js crate0=true crate3=true
//           or: DIM_CONTROL/START doCrateReset.js crate0=true crate3=true

// -------------------------------------------------------------------------

include('scripts/CheckStates.js');

function crateReset(crate)
{
    var msg = "Starting crate reset:";

    var cnt = 0;
    for (var i=0; i<4; i++)
        if (crate[i])
        {
            cnt++;
            msg += " "+i;
        }

    if (cnt==0)
    {
        console.out("No crate to reset.");
        return;
    }

    dim.log(msg);

    console.out("Checking availability of servers...");

    var table =
        [
         [ "MCP" ],
         [ "FTM_CONTROL" ],
         [ "FAD_CONTROL" ],
        ];
    if (!checkStates(table, 3000))
        throw new Error("Either MCP, FTM_CONTROL or FAD_CONTROL not online.");

    // No data taking should be in progress
    // Trigger must be switched off
    checkSend(["MCP", "FAD_CONTROL", "FTM_CONTROL" ]);


    var mcp = dim.state("MCP");
    if (mcp.name=="TriggerOn" || mcp.state=="TakingData")
        dim.send("MCP/STOP");
    if (mcp.name.substr(0,7)=="kConfig")
        dim.send("MCP/RESET");
    if (dim.state("FTM_CONTROL").name=="TriggerOn")
        dim.send("FTM_CONTROL/STOP_TRIGGER");
    if (dim.state("FTM_CONTROL").name.indexOf("Config")==0)
        dim.send("FTM_CONTROL/RESET_CONFIGURE");

    console.out("Checking status of servers...");

    var table =
        [
         [ "MCP", [ "Idle", "Connected" ]],
         [ "FTM_CONTROL", [ "Valid" ] ],
         [ "FAD_CONTROL", [ "Disengaged", "Disconnected", "Connecting", "Connected" ] ],
        ];
    if (!checkStates(table, 3000, true))
        throw new Error("Either MCP, FTM_CONTROL or FAD_CONTROL not in a state in which it ought to be.");

    // FTUs must be switched off

    console.out("Disable FTUs...");

    //checkSend(["FTM_CONTROL"]);
    dim.send("FTM_CONTROL/ENABLE_FTU", -1, false);
    v8.sleep(1000);

    // Boards in the crates must be disconnected

    //checkSend(["FAD_CONTROL"]);

    dim.log("Disconnecting crates.");

    if (dim.state("FAD_CONTROL").name=="Connecting" || dim.state("FAD_CONTROL").name=="Connected")
        for (var i=0; i<10; i++)
        {
            for (var j=0; j<4; j++)
                if (crate[j])
                {
                    console.out("Sending DISCONNECT "+(j*10+i));
                    dim.send("FAD_CONTROL/DISCONNECT", j*10+i);
                }
        }

    v8.sleep(2000);
    if (!checkStates([[ "FAD_CONTROL", [ "Disengaged", "Disconnected", "Connected" ] ]]))
        throw new Error("FAD_CONTROL neither Disengaged, Disconnected not Connected.");


    // Reset crates

    dim.log("Sending reset.");

    if (cnt==4)
        dim.send("FTM_CONTROL/RESET_CAMERA");
    else
    {
        for (var i=0; i<4; i++)
            if (crate[i])
            {
                console.out("Sending RESET_CRATE "+i);
                dim.send("FTM_CONTROL/RESET_CRATE", i);
            }
    }

    // We have to wait a bit

    v8.sleep(3200);

    // Reconnect all boards

    dim.log("Waiting for connection.");

    if (dim.state("FAD_CONTROL").name=="Disengaged")
    {
        dim.send("Waiting 38s for crates to finish reset.");
        v8.sleep(38000);
        dim.send("FAD_CONTROL", "START");
    }
    else
        for (var i=0; i<10; i++)
        {
            v8.sleep(3200);
            for (var j=0; j<4; j++)
                if (crate[j])
                {
                    console.out("Sending CONNECT "+(j*10+i));
                    dim.send("FAD_CONTROL/CONNECT", j*10+i);
                }
        }


    // Reconnect all FTUs

    console.out("Enable FTUs...");

    v8.sleep(1000);
    dim.send("FTM_CONTROL/ENABLE_FTU", -1, true);
    v8.sleep(3000);
    dim.send("FTM_CONTROL/PING");
    v8.sleep(1000);

    dim.wait("FAD_CONTROL", "Connected", 3000);

    // Done

    dim.log("Crate reset finished.");
}
