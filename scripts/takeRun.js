'use strict';

// ================================================================
//  Code related to monitoring the fad system
// ================================================================

var incomplete = 0;

sub_incomplete.onchange = function(evt)
{
    if (!evt.data)
        return;

    var inc = evt.obj['incomplete'];
    if (!inc || inc>0xffffffffff)
        return;

    if (incomplete>0)
        return;

    if (dim.state("MCP").name!="TakingData")
        return;

    console.out("");
    dim.log("Incomplete event ["+inc+","+incomplete+"] detected, sending MCP/STOP");

    incomplete = inc;
    dim.send("MCP/STOP");
}

// ================================================================
//  Code related to taking data
// ================================================================

/**
 * reconnect to problematic FADs
 *
 * Dis- and Reconnects to FADs, found to be problematic by call-back function
 * onchange() to have a different CONNECTION value than 66 or 67. 
 * 
 * @returns
 *      a boolean is returned. 
 *      reconnect returns true if:
 *          * nothing needed to be reset --> no problems found by onchange()
 *          * the reconnection went fine.
 *      
 *      reconnect *never returns false* so far.
 *
 * @example
 *      if (!sub_connections.reconnect())
 *          exit();
 */
function reconnect(list, txt)
{ /*
    var reset = [ ];

    for (var i=0; i<list.length; i++)
        {
            console.out("  FAD %2d".$(list[i])+" lost during "+txt);
            reset.push(parseInt(list[i]/10));
        }

    reset = reset.filter(function(elem,pos){return reset.indexOf(elem)==pos;});

    console.out("");
    console.out("  FADs belong to crate(s): "+reset);
    console.out("");
*/
    console.out("");
    dim.log("Trying automatic reconnect ["+txt+",n="+list.length+"]...");

    if (list.length>3)
        throw new Error("Too many boards to be reconnected. Please check what happened.");

    for (var i=0; i<list.length; i++)
    {
        console.out("   ...disconnect "+list[i]);
        dim.send("FAD_CONTROL/DISCONNECT", list[i]);
    }

    console.out("   ...waiting for 3s");
    v8.sleep(3000);

    for (var i=0; i<list.length; i++)
    {
        console.out("   ...reconnect "+list[i]);
        dim.send("FAD_CONTROL/CONNECT", list[i]);
    }

    console.out("   ...waiting for 1s");

    // Wait for one second to bridge possible pending connects
    v8.sleep(1000);

    console.out("   ...checking connection");

    // Wait for FAD_CONTROL to realize that all boards are connected
    // FIXME: Wait for '40' boards being connected instead
    try
    {
        dim.wait("FAD_CONTROL", "Connected", 3000);
    }
    catch (e)
    {
        if (dim.state("FAD_CONTROL").name!="Connecting")
        {
            console.out("");
            console.out(" + FAD_CONTROL: "+dim.state("FAD_CONTROL").name);
            console.out("");
            throw e;
        }

        var crates = [];
        for (var i=0; i<list.length; i++)
            crates[list[i]/10] = true;

        include('scripts/crateReset.js');
        crateReset(crates);
    }

    // Wait also for MCP to have all boards connected again
    dim.wait("MCP", "Idle", 3000);

    dim.log("Automatic reconnect successfull.");
    console.out("");
}

function takeRun(type, count, time, func)
{
    if (!count)
        count = -1;
    if (!time)
        time = -1;

    var nextrun = sub_startrun.get().obj['next'];
    dim.log("Take run %3d".$(nextrun)+": N="+count+" T="+time+"s ["+type+"]");

    // FIXME: Replace by callback?
    //
    // DN: I believe instead of waiting for 'TakingData' one could split this
    // up into two checks with an extra condition:
    //  if type == 'data':
    //      wait until ThresholdCalibration starts:
    //          --> this time should be pretty identical for each run
    //      if this takes longer than say 3s:
    //          there might be a problem with one/more FADs
    //    
    //      wait until "TakingData":
    //          --> this seems to take even some minutes sometimes... 
    //              (might be optimized rather soon, but still in the moment...)
    //      if this takes way too long: 
    //          there might be something broken, 
    //          so maybe a very high time limit is ok here.
    //          I think there is not much that can go wrong, 
    //          when the Thr-Calib has already started. Still it might be nice 
    //          If in the future RateControl is written so to find out that 
    //          in case the threshold finding algorithm does 
    //          *not converge as usual*
    //          it can complain, and in this way give a hint, that the weather
    //          might be a little bit too bad.
    //  else:
    //      wait until "TakingData":
    //          --> in a non-data run this time should be pretty short again
    //      if this takes longer than say 3s:
    //          there might be a problem with one/more FADs
    //  

    // Use this if you use the rate control to calibrate by rates
    //if (!dim.wait("MCP", "TakingData", -300000) )
    //{
    //    throw new Error("MCP took longer than 5 minutes to start TakingData"+
    //                    "maybe this idicates a problem with one of the FADs?");
    //}

    // ================================================================
    //  Function for Critical voltage
    // ================================================================

    // INSTALL a watchdog... send FAD_CONTROL/CLOSE_OPEN_FILES
    // could send MCP/RESET as well but would result in a timeout
    var callback = dim.onchange['FEEDBACK'];
    dim.onchange['FEEDBACK'] = function(state)
    {
        if (callback)
            callback.call(this, state);

        if ((state.name=="Critical" || state.name=="OnStandby") &&
            (this.last!="Critical"  && this.last!="OnStandby"))
        {
            console.out("Feedback state changed from "+this.last+" to "+state.name+" [takeRun.js]");

            // Includes FAD_CONTROL/CLOSE_ALL_OPEN_FILES
            dim.send("MCP/STOP");
        }

        this.last=state.name;
    }

    // Here we could check and handle fad losses

    incomplete = 0;

    var start = true;

    for (var n=0; n<3; n++)
    {
        if (start)
        {
            dim.send("MCP/START", time, count, type);
            if (typeof(func)=="function")
                func();
        }

        try
        {
            dim.wait("MCP", "TakingData", 15000);
            break;
        }
        catch (e)
        {
            if (dim.state("MCP").name=="TriggerOn" &&
                dim.state("FAD_CONTROL").name=="Connected" &&
                dim.state("FTM_CONTROL").name=="TriggerOn")
            {
                console.out("");
                console.out("Waiting for TakingData timed out. Everything looks ok, but file not yet open... waiting once more.");
                start = false;
                continue;
            }

            start = true;

            console.out("");
            console.out(" + MCP:         "+dim.state("MCP").name);
            console.out(" + FAD_CONTROL: "+dim.state("FAD_CONTROL").name);
            console.out(" + FTM_CONTROL: "+dim.state("FTM_CONTROL").name);
            console.out("");

            if (dim.state("MCP").name!="Configuring3" ||
                (dim.state("FAD_CONTROL").name!="Configuring1" &&
                 dim.state("FAD_CONTROL").name!="Configuring2"))
                throw e;

            console.out("");
            console.out("Waiting for fadctrl to get configured timed out... checking for in-run FAD loss.");

            var con  = sub_connections.get();
            var stat = con.obj['status'];

            console.out("Sending MCP/RESET");
            dim.send("MCP/RESET");

            dim.wait("FTM_CONTROL", "Valid",     3000);
            dim.wait("FAD_CONTROL", "Connected", 3000);
            dim.wait("MCP",         "Idle",      3000);

            var list = [];
            for (var i=0; i<40; i++)
                if (stat[i]!=0x43)
                    list.push(i);

            reconnect(list, "configuration");

            if (n==2)
                throw e;

            //dim.wait("MCP", "Idle", 3000);
        }
    }

    // This is to check if we have missed the event. This can happen as
    // a race condition when the MCP/STOP is sent by the event handler
    // but the run was not yet fully configured.
    var statefb = dim.state("FEEDBACK").name;
    if (statefb=="Critical" || statefb=="OnStandby")
    {
        console.out("Run started by FEEDBACK in state "+statefb);
        dim.send("MCP/STOP"); // Includes FAD_CONTROL/CLOSE_ALL_OPEN_FILES

        dim.onchange['FEEDBACK'] = callback;

        return true;
    }

    dim.wait("MCP", "Idle", time>0 ? time*1250 : undefined); // run time plus 25%

    // REMOVE watchdog
    dim.onchange['FEEDBACK'] = callback;

    if (incomplete)
    {
        console.out("");
        console.out(" - MCP:         "+dim.state("MCP").name);
        console.out(" - FAD_CONTROL: "+dim.state("FAD_CONTROL").name);
        console.out(" - FTM_CONTROL: "+dim.state("FTM_CONTROL").name);

        dim.wait("FTM_CONTROL", "Valid",     3000);
        dim.wait("FAD_CONTROL", "Connected", 3000);
        dim.wait("MCP",         "Idle",      3000);

        var str = incomplete.toString(2);
        var len = str.length;

        var list = [];
        for (var i=0; i<str.length; i++)
            if (str[str.length-i-1]=='1')
                list.push(i);

        reconnect(list, "data taking");

        return false;
    }

    // FIXME: What if the ext1 is not enabled in the configuration?
    if (type=="data")
    {
        var dim_trg = new Subscription("FAD_CONTROL/TRIGGER_COUNTER");
        var counter = dim_trg.get(3000);

        // The check on physics and pedestal triggers is to ensure that
        // there was at least a chance to receive any event (e.g. in case
        // of an interrupt this might not be the case)
        if (counter.qos!=111 &&
            (counter.data['N_trg']>1000 || counter.data['N_ped']>5) &&
            counter.data['N_ext1']==0) // 'o' for open
            throw new Error("No ext1 triggers received during data taking... please check the reason and report in the logbook.");
        dim_trg.close();
    }

    return true;
}
