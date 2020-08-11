'use strict';

var Func = function() { };
Func.sum = function(a, b) { return a+b; }
Func.sq  = function(a, b) { return Math.sqrt(a*a + b*b); }
Func.min = function(a, b) { return Math.min(a, b); }
Func.max = function(a, b) { return Math.max(a, b); }
Func.avg = function(arr)  { return arr.reduce(Func.Sum, 0)/arr.length; }
Func.stat = function(arr, func)
{
    if (arr.length==0)
        return undefined;

    var sum = 0;
    var sq  = 0;
    var cnt = 0;
    var min = arr[0];
    var max = arr[0];
    arr.forEach(function(val, idx) { sum+=val; sq+=val*val; if (val>max) max=val; if (val<min) min=val; if (func && func(val, idx)) cnt++ });
    sum /= arr.length;
    sq  /= arr.length;

    return { avg:sum, rms:Math.sqrt(sq-sum*sum), min:min, max:max, count:cnt };
}

// ===================================================================

console.out(("\n%78s".$("")).replace(/ /g, "="));

if (dim.state("FTM_CONTROL").name=="TriggerOn")
{
    dim.send("FTM_CONTROL/STOP_TRIGGER");
    dim.wait("FTM_CONTROL", "Valid");
}


include('scripts/CheckStates.js');

var table =
[
 [ "MCP",                 [ "Idle"      ] ],
 [ "AGILENT_CONTROL_24V", [ "VoltageOn" ] ],
 [ "AGILENT_CONTROL_50V", [ "VoltageOn" ] ],
// [ "AGILENT_CONTROL_80V", [ "VoltageOn" ] ],
 [ "AGILENT_CONTROL_80V", [ "VoltageOn", "Disconnected"       ] ], //hack to allow for data taking while agilent not available
 [ "FTM_CONTROL",         [ "Valid"     ] ],
 [ "FAD_CONTROL",         [ "Connected",    "RunInProgress"   ] ],
 [ "BIAS_CONTROL",        [ "Disconnected", "VoltageOff"      ] ],
 [ "DATA_LOGGER",         [ "WaitForRun",   "NightlyFileOpen", "Logging" ] ],
];

console.out("Checking states.");
if (!checkStates(table))
{
    throw new Error("Something unexpected has happened. One of the servers",
            "is in a state in which it should not be. Please,",
            "try to find out what happened...");
}

// ===================================================================

include('scripts/Hist1D.js');
include('scripts/Hist2D.js');

console.out("Checking power on time");

var service_drs = new Subscription("FAD_CONTROL/DRS_RUNS");

var runs = service_drs.get(5000, false);
//if (!runs)
//    throw new Error("Could not connect to FAD_CONTROL/DRS_RUNS");

var power = dim.state("AGILENT_CONTROL_50V").time;
var now   = new Date();

var diff = (now-runs.time)/3600000;

console.out(" * Now:                "+now);
console.out(" * Last power cycle:   "+power);
console.out(" * Last DRS calib set: "+(runs.data?runs.time:"none"));


if (1)//diff>8 && now.getHours()>16 || runs.time<power)
{
    console.out("Checking send.");
    checkSend(["FAD_CONTROL", "MCP", "RATE_CONTROL"]);
    console.out("Checking send: done");

    //console.out("Most probablay the camera has not been checked for underflows yet.");

    var service_event = new Subscription("FAD_CONTROL/EVENT_DATA");

    dim.send("FAD_CONTROL/START_DRS_CALIBRATION");
    dim.send("FAD_CONTROL/SET_FILE_FORMAT", 0);

    var sub_runs = new Subscription("FAD_CONTROL/RUNS");
    var sruns = sub_runs.get(5000, false);

    if (dim.state("FAD_CONTROL").name=="RunInProgress" || sruns.qos==1)
    {
        dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
        dim.wait("FAD_CONTROL", "Connected", 3000);

        console.out("Waiting for open files to be closed...");
        v8.timeout(60000, function() { if (sub_runs.get(0, false).qos==0) return true; });

        // Although the file should be closed now, the processing might still be on-going
        // and delayed events might be received. The only fix for that issue is to
        // add the run number to the data we are waiting for
        v8.sleep(5000);
    }

    sub_runs.close();

    console.out("Starting drs-gain... waiting for new event");

    var sub_startrun = new Subscription("FAD_CONTROL/START_RUN");
    var sub_incomplete = new Subscription("FAD_CONTROL/INCOMPLETE");
    var sub_connections = new Subscription("FAD_CONTROL/CONNECTIONS");
    sub_connections.get(5000);
    sub_startrun.get(5000);

    include('scripts/takeRun.js');

    while (1)
    {
        var event_counter = service_event.get(10000, false).counter;

        var stop = function ()
        {
            while (1)
            {
                if (dim.state("MCP").name=="TakingData" && service_event.get(0, false).counter>event_counter)
                {
                    dim.send("MCP/STOP");
                    console.out("Sent MCP/STOP.");
                    return;
                }
                v8.sleep(100);
            }
        }

        var thread = new Thread(250, stop);

        var rc = takeRun("drs-gain");

        thread.kill();

        if (rc)
            break;
    }

    console.out("Event received.");

    sub_incomplete.close();
    sub_connections.close();
    sub_startrun.close();


    // FIXME: Restore DRS calibration in case of failure!!
    //        FAD Re-connect in case of failure?
    //        MCP/RESET in case of failure?
    //        Proper error reporting!

    var event = service_event.get(3000);//, false);
    service_event.close();

    console.out("Run stopped.");

    dim.send("RATE_CONTROL/STOP"); // GlobalThresholdSet -> Connected
    dim.wait("MCP", "Idle", 3000);

    var nn = runs.data && runs.data.length>0 && runs.obj['roi']>0 ? runs.obj['run'].reduce(Func.max) : -1;
    if (nn>0)
    {
        var night = runs.obj['night'];

        var yy =  night/10000;
        var mm = (night/100)%100;
        var dd =  night%100;

        var filefmt = "/loc_data/raw/%d/%02d/%02d/%8d_%03d.drs.fits";

        dim.log("Trying to restore last DRS calibration #"+nn+"  ["+runs.time+"; "+night+"]");

        // FIXME: Timeout
        var drs_counter = service_drs.get(0, false).counter;
        dim.send("FAD_CONTROL/LOAD_DRS_CALIBRATION", filefmt.$(yy, mm, dd, night, nn));

        try
        {
            var now = new Date();
            v8.timeout(3000, function() { if (service_drs.get(0, false).counter>drs_counter) return true; });
            dim.log("Last DRS calibration restored ["+(new Date()-now)/1000+"s]");
        }
        catch (e)
        {
            console.warn("Restoring last DRS calibration failed.");
        }
    }

    var hist = Hist2D(16, -2048.5, 2048.5, 11, -10, 100);

    var data = event.obj;

    for (var i=0; i<1440; i++)
        hist.fill(data.avg[i], isNaN(data.rms[i])?-1:data.rms[i]);

    hist.print();

    var stat0 = Func.stat(data.avg, function(val, idx) { if (val<600) console.out(" PIX[hw="+idx+"]="+val); return val<600; });
    var stat1 = Func.stat(data.rms);

    console.out("Avg[min]=%.1f".$(stat0.min));
    console.out("Avg[avg]=%.1f +- %.1f".$(stat0.avg, stat0.rms));
    console.out("Avg[max]=%.1f".$(+stat0.max));
    console.out("Avg[cnt]="+stat0.count);
    console.out("");
    console.out("Rms[min]=%.1f".$(stat1.min));
    console.out("Rms[avg]=%.1f +- %.1f".$(stat1.avg, stat1.rms));
    console.out("Rms[max]=%.1f".$(stat1.max));
    console.out(("%78s\n".$("")).replace(/ /g, "="));

    //      OK                            UNDERFLOW
    // ------------------------------------------------------
    // Avg[min]=722.0                Avg[min]=-380.0
    // Avg[avg]=815.9 +- 45.9        Avg[avg]= 808.0 +- 102.0
    // Avg[max]=930.5                Avg[max]= 931.1
    // Avg[cnt]=0                    Avg[cnt]= 9

    // Rms[min]=14.0                 Rms[min]=13.9
    // Rms[avg]=16.5 +- 1.6          Rms[avg]=18.8 +- 26.8
    // Rms[max]=44.0                 Rms[max]=382.1

    if (stat0.count>0)
    {
        if (stat0.count>8)
            throw new Error("Underflow condition detected in about "+parseInt(stat0.count/9+.5)+" DRS.");

        console.warn("There is probably an underflow condition in one DRS... please check manually.");
    }
}

service_drs.close();
