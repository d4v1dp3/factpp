'use strict';

var service_ftm = new Subscription("FTM_CONTROL/FTU_LIST");

// Make sure that we receive a 'Yes, we are connected and names are available' event
service_ftm.get(5000);

// Check for all FTUs to be connected when the next event arrives
service_ftm.onchange = function(event)
{
    var ping = event.obj['Ping'];
    for (var i=0; i<40; i++)
    {
        if (ping[i]==1)
            continue;

        var str = "";
        for (var h=0; h<4; h++)
        {
            for (var w=0; w<10; w++)
                str += ping[h*10+w];
            if (h!=3)
                str += '|';
        }

        console.out(str)

        console.out("Problems in the FTU communication found.");
        console.out("Send command to disable all FTUs.");
        console.out(" => Crate reset needed.");

        dim.send("FTM_CONTROL/ENABLE_FTU", -1, false);
        throw new Error("CrateReset[FTU]");
    }

    // Signal success by closing the connection
    service_ftm.close();
}

// Send ping (request FTU status)
dim.send("FTM_CONTROL/PING");

// Wait for 1 second for the answer
var timeout = new Thread(3000, function(){ if (service_ftm.isOpen) throw new Error("Could not check that all FTUs are ok within 3s."); });
while (service_ftm.isOpen)
    v8.sleep();

