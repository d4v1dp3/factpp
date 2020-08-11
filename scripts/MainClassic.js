/**
 * @fileOverview This file has functions related to documenting JavaScript.
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
'use strict';

dim.log("Start: "+__FILE__+" ["+__DATE__+"]");

//dimctrl.defineState(37, "TimeOutBeforeTakingData", "MCP took more than 5minutes to start TakingData");

// ================================================================
//  Code related to the schedule
// ================================================================

//this is just the class implementation of 'Observation'
include('scripts/Observation_class.js');

// this file just contains the definition of
// the variable observations, which builds our nightly schedule, hence the filename
include('scripts/schedule.js');

// make Observation objects from user input and check if 'date' is increasing.
for (var i=0; i<observations.length; i++)
{
    observations[i] = new Observation(observations[i]);

    // check if the start date given by the user is increasing.
    if (i>0 && observations[i].start <= observations[i-1].start)
    {
        throw new Error("Start time '"+ observations[i].start.toUTCString()+
                        "' in row "+i+" exceeds start time in row "+(i-1));
    }
}

// Get the observation scheduled for 'now' from the table and
// return its index
function getObservation(now)
{
    if (now==undefined)
        now = new Date();

    if (isNaN(now.valueOf()))
        throw new Error("Date argument in getObservation invalid.");

    for (var i=0; i<observations.length; i++)
        if (now<observations[i].start)
            return i-1;

    return observations.length-1;
}

// ================================================================
//  Code to check whether observation is allowed
// ================================================================
/*
function currentEst(source)
{
    var moon = new Moon();
    if (!moon.isUp)
        return 7.7;

    var dist = Sky.dist(moon, source);

    var alt = 90-moon.toLocal().zd;

    var lc = dist*alt*pow(Moon.disk(), 6)/360/360;

    var cur = 7.7+4942*lc;

    return cur;
}

function thresholdEst(source) // relative threshold (ratio)
{
    // Assumption:
    // atmosphere is 70km, shower taks place after 60km, earth radius 6400km
    // just using the cosine law
    // This fits very well with MC results: See Roger Firpo, p.45
    // "Study of the MAGIC telescope sensitivity for Large Zenith Angle observations"

    var c = Math.cos(Math.Pi-source.zd);
    var ratio = (10*sqrt(409600*c*c+9009) + 6400*c - 60)/10;

    // assumption: Energy threshold increases linearily with current
    // assumption: Energy threshold increases linearily with distance

    return ratio*currentEst(source)/7.7;
}
*/

// ----------------------------------------------------------------

// ================================================================
//  Code related to monitoring the fad system
// ================================================================

var sub_incomplete = new Subscription("FAD_CONTROL/INCOMPLETE");

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

    incomplete = inc;

    console.out("Sending MCP/STOP");
    dim.send("MCP/STOP");
}

var sub_connections = new Subscription("FAD_CONTROL/CONNECTIONS");

/**
 * call-back function of FAD_CONTROL/CONNECTIONS
 * store IDs of problematic FADs 
 *
 */
/*
sub_connections.onchange = function(evt)
{
    // This happens, but why?
    if (!evt.obj['status'])
        return;

    this.reset = [ ];

    for (var x=0; x<40; x++)
        if (evt.obj['status'][x]!=66 && evt.obj['status'][x]!=67)
            this.reset.push(x);

    if (this.reset.length==0)
        return;

    //m.alarm("FAD board loss detected...");
    dim.send("MCP/RESET");
    dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
}
*/

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
sub_connections.reconnect = function()
{
    // this.reset is a list containing the IDs of FADs, 
    // which have neither CONNECTION==66 nor ==67, whatever this means :-)
    if (this.reset.length==0)
        return true;

    console.out("  Reconnect: start ["+this.reset.length+"]");

    for (var i=0; i<this.reset.length; i++)
        dim.send("FAD_CONTROL/DISCONNECT", this.reset[i]);

    v8.sleep(3000);

    while (this.reset.length)
        dim.send("FAD_CONTROL/CONNECT", this.reset.pop());

    v8.sleep(1000);
    dim.wait("FAD_CONTROL", "Connected", 3000);

    console.out("  Reconnect: end");

    return true;
}

// ================================================================
//  Code related to taking data
// ================================================================

var startrun = new Subscription("FAD_CONTROL/START_RUN");
startrun.get(5000);

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
    console.out("Trying automatic reconnect ["+txt+"]...");

    for (var i=0; i<list.length; i++)
    {
        console.out("   ...disconnect "+list[i]);
        dim.send("FAD_CONTROL/DISCONNECT", list[i]);
    }

    console.out("   ...waiting for 5s");
    v8.sleep(5000);

    for (var i=0; i<list.length; i++)
    {
        console.out("   ...reconnect "+list[i]);
        dim.send("FAD_CONTROL/CONNECT", list[i]);
    }

    console.out("   ...waiting for 1s");
    v8.sleep(1000);
    console.out("");
}

function takeRun(type, count, time)
{
    if (!count)
        count = -1;
    if (!time)
        time = -1;

    var nextrun = startrun.get().obj['next'];
    console.out("  Take run %3d".$(nextrun)+": N="+count+" T="+time+"s ["+type+"]");

    incomplete = 0;
    dim.send("MCP/START", time?time:-1, count?count:-1, type);

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

    // Here we could check and handle fad losses

    try
    {
        dim.wait("MCP", "TakingData", 15000);
    }
    catch (e)
    {
        console.out("");
        console.out("MCP:         "+dim.state("MCP").name);
        console.out("FAD_CONTROL: "+dim.state("FAD_CONTROL").name);
        console.out("FTM_CONTROL: "+dim.state("FTM_CONTROL").name);
        console.out("");

        if (dim.state("MCP").name!="Configuring3" ||
            dim.state("FAD_CONTROL").name!="Configuring2")
            throw e;

        console.out("");
        console.out("Waiting for fadctrl to get configured timed out... checking for in-run FAD loss.");

        var con  = sub_connections.get();
        var stat = con.obj['status'];

        console.out("Sending MCP/RESET");
        dim.send("MCP/RESET");

        dim.wait("FTM_CONTROL", "Idle",      3000);
        dim.wait("FAD_CONTROL", "Connected", 3000);
        dim.wait("MCP",         "Idle",      3000);

        /*** FOR REMOVE ***/
        var reset = [ ];

        for (var i=0; i<40; i++)
            if (stat[i]!=0x43)
            {
                console.out("  FAD %2d".$(i)+" not in Configured state.");
                reset.push(parseInt(i/10));
            }

        reset = reset.filter(function(elem,pos){return reset.indexOf(elem)==pos;});

        if (reset.length>0)
        {
            console.out("");
            console.out("  FADs belong to crate(s): "+reset);
            console.out("");
        }
        /**** FOR REMOVE ****/

        var list = [];
        for (var i=0; i<40; i++)
            if (stat[i]!=0x43)
                list.push(i);

        reconnect(list, "configuration");

        throw e;
    }

    dim.wait("MCP", "Idle", time>0 ? time*1250 : undefined); // run time plus 25%

    if (incomplete)
    {
        console.out("Incomplete: "+incomplete);

        console.out("");
        console.out("MCP:         "+dim.state("MCP").name);
        console.out("FAD_CONTROL: "+dim.state("FAD_CONTROL").name);
        console.out("FTM_CONTROL: "+dim.state("FTM_CONTROL").name);
        console.out("");

        dim.wait("MCP",         "Idle", 3000);
        dim.wait("FTM_CONTROL", "Idle", 3000);

        // Necessary to allow the disconnect, reconnect
        dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
        dim.wait("FAD_CONTROL", "Connected", 3000);

        var list = [];
        for (var i=0; i<40; i++)
            if (incomplete&(1<<i))
                list.push(i);

        reconnect(list, "data taking");

        throw new Error("In-run FAD loss detected.");
    }

    //console.out("  Take run: end");

    // DN: currently reconnect() never returns false 
    //     .. but it can fail of course.
    //if (!sub_connections.reconnect())
    //    exit();

    return true;//sub_connections.reconnect();
}

// ----------------------------------------------------------------

function doDrsCalibration(where)
{
    console.out("  Take DRS calibration ["+where+"]");

    service_feedback.voltageOff();

    var tm = new Date();

    while (1)
    {
        dim.send("FAD_CONTROL/START_DRS_CALIBRATION");
        if (!takeRun("drs-pedestal", 1000))     // 40 / 20s     (50Hz)
            continue;

        // Does that fix the runopen before runclose problem?
        //dim.wait("FAD_CONTROL", "Connected", 3000);
        //v8.sleep(1000);

        if (!takeRun("drs-gain",     1000))     // 40 / 20s     (50Hz)
            continue;

        // Does that fix the runopen before runclose problem?
        //dim.wait("FAD_CONTROL", "Connected", 3000);
        //v8.sleep(1000);

        if (!takeRun("drs-pedestal", 1000))     // 40 / 20s     (50Hz)
            continue;

        dim.send("FAD_CONTROL/SET_FILE_FORMAT", 2);
        if (!takeRun("drs-pedestal", 1000))     // 40 / 20s     (50Hz)
            continue;
        if (!takeRun("drs-time",     1000))     // 40 / 20s     (50Hz)
            continue;

        dim.send("FAD_CONTROL/RESET_SECONDARY_DRS_BASELINE");
        if (!takeRun("pedestal",     1000))     // 40 / 10s     (80Hz)
            continue;

        dim.send("FAD_CONTROL/SET_FILE_FORMAT", 2);
        if (!takeRun("pedestal",     1000))     // 40 / 10s     (80Hz)
            continue;
        //                                       -----------
        //                                       4'40 / 2'00

        break;
    }

    console.out("  DRS calibration done [%.1f]".$((new Date()-tm)/1000));
}

// ================================================================
//  Code related to the lid
// ================================================================

function OpenLid()
{
    /*
    while (Sun.horizon(-13).isUp)
    {
        var now = new Date();
        var minutes_until_sunset = (Sun.horizon(-13).set - now)/60000;
        console.out(now.toUTCString()+": Sun above FACT-horizon, lid cannot be opened: sleeping 1min, remaining %.1fmin".$(minutes_until_sunset));
        v8.sleep(60000);
    }*/

    var isClosed = dim.state("LID_CONTROL").name=="Closed";

    var tm = new Date();

    // Wait for lid to be open
    if (isClosed)
    {
        console.out("  Open lid: start");
        dim.send("LID_CONTROL/OPEN");
    }
    dim.wait("LID_CONTROL", "Open", 30000);

    if (isClosed)
        console.out("  Open lid: done [%.1fs]".$((new Date()-tm)/1000));
}

function CloseLid()
{
    var isOpen = dim.state("LID_CONTROL").name=="Open";

    var tm = new Date();

    // Wait for lid to be open
    if (isOpen)
    {
        console.out("  Close lid: start");
        dim.send("LID_CONTROL/CLOSE");
    }
    dim.wait("LID_CONTROL", "Closed", 30000);

    if (isOpen)
        console.out("  Close lid: end [%.1fs]".$((new Date()-tm)/1000));
}

// ================================================================
//  Code related to switching bias voltage on and off
// ================================================================

var service_feedback = new Subscription("FEEDBACK/DEVIATION");

service_feedback.onchange = function(evt)
{
    if (this.cnt && evt.counter>this.cnt+12)
        return;

    this.voltageStep = null;
    if (!evt.data)
        return;

    var delta = evt.obj['DeltaBias'];

    var avg = 0;
    for (var i=0; i<320; i++)
        avg += delta[i];
    avg /= 320;

    if (this.previous)
        this.voltageStep = Math.abs(avg-this.previous);

    this.previous = avg;

    console.out("  DeltaV="+this.voltageStep);
}

// DN:  Why is voltageOff() implemented as 
//      a method of a Subscription to a specific Service
//      I naively would think of voltageOff() as an unbound function.
//      I seems to me it has to be a method of a Subscription object, in order
//      to use the update counting method. But does it have to be
//      a Subscription to FEEDBACK/DEVIATION, or could it work with other services as well?
service_feedback.voltageOff = function()
{
    var state = dim.state("BIAS_CONTROL").name;

    // check of feedback has to be switched on
    var isOn = state=="VoltageOn" || state=="Ramping";
    if (isOn)
    {
        console.out("  Voltage off: start");

        // Supress the possibility that the bias control is
        // ramping and will reject the command to switch the
        // voltage off
        var isControl = dim.state("FEEDBACK").name=="CurrentControl";
        if (isControl)
        {
            console.out("  Suspending feedback.");
            dim.send("FEEDBACK/ENABLE_OUTPUT", false);
            dim.wait("FEEDBACK", "CurrentCtrlIdle", 3000);
        }

        // Switch voltage off
        console.out("  Voltage on: switch off");
        dim.send("BIAS_CONTROL/SET_ZERO_VOLTAGE");

        // If the feedback was enabled, re-enable it
        if (isControl)
        {
            console.out("  Resuming feedback.");
            dim.send("FEEDBACK/ENABLE_OUTPUT", true);
            dim.wait("FEEDBACK", "CurrentControl", 3000);
        }
    }

    dim.wait("BIAS_CONTROL", "VoltageOff", 5000);

    // FEEDBACK stays in CurrentCtrl when Voltage is off but output enabled
    // dim.wait("FEEDBACK", "CurrentCtrlIdle", 1000);

    if (isOn)
        console.out("  Voltage off: end");
}

// DN:  The name of the method voltageOn() in the context of the method
//      voltageOff() is a little bit misleading, since when voltageOff() returns
//      the caller can be sure the voltage is off, but when voltageOn() return
//      this is not the case, in the sense, that the caller can now take data.
//      instead the caller of voltageOn() *must* call waitForVoltageOn() afterwards
//      in order to safely take good-quality data.
//      This could lead to nasty bugs in the sense, that the second call might 
//      be forgotten by somebody
//      
//      so I suggest to rename voltageOn() --> prepareVoltageOn()
//      waitForVoltageOn() stays as it is
//      and one creates a third method called:voltageOn() like this
/*      service_feedback.voltageOn = function()
 *      {
 *          this.prepareVoltageOn();
 *          this.waitForVoltageOn();
 *      }
 * 
 * */
//      For convenience.

service_feedback.voltageOn = function()
{
    //if (Sun.horizon("FACT").isUp)
    //    throw new Error("Sun is above FACT-horizon, voltage cannot be switched on.");

    var isOff = dim.state("BIAS_CONTROL").name=="VoltageOff";
    if (isOff)
    {
        console.out("  Voltage on: switch on");
        //console.out(JSON.stringify(dim.state("BIAS_CONTROL")));

        dim.send("BIAS_CONTROL/SET_GLOBAL_DAC", 1);
    }

    // Wait until voltage on
    dim.wait("BIAS_CONTROL", "VoltageOn", 5000);

    // From now on the feedback waits for a valid report from the FSC
    // and than switchs to CurrentControl
    dim.wait("FEEDBACK", "CurrentControl", 60000);

    if (isOff)
    {
        console.out("  Voltage on: cnt="+this.cnt);

        this.previous = undefined;
        this.cnt = this.get().counter;
        this.voltageStep = undefined;
    }
}

service_feedback.waitForVoltageOn = function()
{
    // waiting 45sec for the current control to stabilize...
    // v8.sleep(45000);

    // ----- Wait for at least three updates -----
    // The feedback is started as if the camera where at 0deg
    // Then after the first temp update, the temperature will be set to the
    // correct value (this has already happened)
    // So we only have to wait for the current to get stable.
    // This should happen after three to five current updates.
    // So we want one recent temperature update
    //  and three recent current updates

    // Avoid output if condition is already fulfilled
    if (this.cnt && this.get().counter>this.cnt+10)
        return;

    // FIXME: timeout missing
    console.out("  Feedback wait: start");

    function func(service)
    {
        if ((service.cnt!=undefined && service.get().counter>service.cnt+10) ||
            (service.voltageStep && service.voltageStep<0.02))
            return true;
    }

    var now = new Date();
    //v8.timeout(5*60000, func, this);
    while ((this.cnt==undefined || this.get().counter<=this.cnt+10) && (!this.voltageStep || this.voltageStep>0.02))
        v8.sleep();

    console.out("  Feedback wait: end [dV=%.3f, cnt=%d, %.2fs]".$(this.voltageStep, this.get().counter, (new Date()-now)/1000));
}

// ================================================================
//  Function to shutdown the system
// ================================================================

function Shutdown()
{
    console.out("Shutdown: start");

    service_feedback.voltageOff();
    CloseLid(); 
    dim.send("DRIVE_CONTROL/PARK");

    console.out("Waiting for telescope to park. This may take a while.");

    // FIXME: This might not work is the drive is already close to park position
    dim.wait("DRIVE_CONTROL", "Locked", 3000);

    var sub = new Subscription("DRIVE_CONTROL/POINTING_POSITION");
    sub.get(5000);  // FIXME: Proper error message in case of failure

    function func()
    {
        var report = sub.get();

        var zd = report.obj['Zd'];
        var az = report.obj['Az'];

        if (zd>100 && Math.abs(az)<1)
            return true;

        return undefined;
    }

    var now = new Date();
    v8.timeout(150000, func);

    //dim.send("FEEDBACK/STOP");
    dim.send("FEEDBACK/ENABLE_OUTPUT", false);
    dim.send("FTM_CONTROL/STOP_TRIGGER");

    dim.wait("FEEDBACK", "CurrentCtrlIdle", 3000);
    dim.wait("FTM_CONTROL", "Idle", 3000);

    var report = sub.get();

    console.out("");
    console.out("Shutdown procedure seems to be finished...");
    console.out("  Telescope at Zd=%.1fdeg Az=%.1fdeg".$(report.obj['Zd'], report.obj['Az']));
    console.out("  Please make sure the park position was reached");
    console.out("  and the telescope is not moving anymore.");
    console.out("  Please check that the lid is closed and the voltage switched off.");
    console.out("");
    console.out("Shutdown: end ["+(new Date()-now)/1000+"s]");

    sub.close();
}

// ================================================================
// Check datalogger subscriptions
// ================================================================

var datalogger_subscriptions = new Subscription("DATA_LOGGER/SUBSCRIPTIONS");
datalogger_subscriptions.get(3000, false);

datalogger_subscriptions.check = function()
{
    var obj = this.get();
    if (!obj.data)
        throw new Error("DATA_LOGGER/SUBSCRIPTIONS not available.");

    var expected =
        [
         "BIAS_CONTROL/CURRENT",
         "BIAS_CONTROL/DAC",
         "BIAS_CONTROL/NOMINAL",
         "BIAS_CONTROL/VOLTAGE",
         "DRIVE_CONTROL/POINTING_POSITION",
         "DRIVE_CONTROL/SOURCE_POSITION",
         "DRIVE_CONTROL/STATUS",
         "DRIVE_CONTROL/TRACKING_POSITION",
         "FAD_CONTROL/CONNECTIONS",
         "FAD_CONTROL/DAC",
         "FAD_CONTROL/DNA",
         "FAD_CONTROL/DRS_RUNS",
         "FAD_CONTROL/EVENTS",
         "FAD_CONTROL/FEEDBACK_DATA",
         "FAD_CONTROL/FILE_FORMAT",
         "FAD_CONTROL/FIRMWARE_VERSION",
         "FAD_CONTROL/INCOMPLETE",
         "FAD_CONTROL/PRESCALER",
         "FAD_CONTROL/REFERENCE_CLOCK",
         "FAD_CONTROL/REGION_OF_INTEREST",
         "FAD_CONTROL/RUNS",
         "FAD_CONTROL/RUN_NUMBER",
         "FAD_CONTROL/START_RUN",
         "FAD_CONTROL/STATISTICS1",
         "FAD_CONTROL/STATISTICS2",
         "FAD_CONTROL/STATS",
         "FAD_CONTROL/STATUS",
         "FAD_CONTROL/TEMPERATURE",
         "FEEDBACK/CALIBRATED_CURRENTS",
         "FEEDBACK/CALIBRATION",
         "FEEDBACK/DEVIATION",
         "FEEDBACK/REFERENCE",
         "FSC_CONTROL/CURRENT",
         "FSC_CONTROL/HUMIDITY",
         "FSC_CONTROL/TEMPERATURE",
         "FSC_CONTROL/VOLTAGE",
         "FTM_CONTROL/COUNTER",
         "FTM_CONTROL/DYNAMIC_DATA",
         "FTM_CONTROL/ERROR",
         "FTM_CONTROL/FTU_LIST",
         "FTM_CONTROL/PASSPORT",
         "FTM_CONTROL/STATIC_DATA",
         "FTM_CONTROL/TRIGGER_RATES",
         "LID_CONTROL/DATA",
         "MAGIC_LIDAR/DATA",
         "MAGIC_WEATHER/DATA",
         "MCP/CONFIGURATION",
         "PWR_CONTROL/DATA",
         "RATE_CONTROL/THRESHOLD",
         "RATE_SCAN/DATA",
         "RATE_SCAN/PROCESS_DATA",
         "TEMPERATURE/DATA",
         "TIME_CHECK/OFFSET",
         "TNG_WEATHER/DATA",
         "TNG_WEATHER/DUST",
        ];

    function map(entry)
    {
        if (entry.length==0)
            return undefined;

        var rc = entry.split(',');
        if (rc.length!=2)
            throw new Error("Subscription list entry '"+entry+"' has wrong number of elements.");
        return rc;
    }

    var list = obj.data.split('\n').map(map);

    function check(name)
    {
        if (list.every(function(el){return el[0]!=name;}))
            throw new Error("Subscription to '"+name+"' not available.");
    }

    expected.forEach(check);
}



// ================================================================
// Crosscheck all states
// ================================================================

// ----------------------------------------------------------------
// Do a standard startup to bring the system in into a well
// defined state
// ----------------------------------------------------------------
include('scripts/Startup.js');

// ----------------------------------------------------------------
// Check that everything we need is availabel to receive commands
// (FIXME: Should that go to the general CheckState?)
// ----------------------------------------------------------------
console.out("Checking send.");
checkSend(["MCP", "DRIVE_CONTROL", "LID_CONTROL", "FAD_CONTROL", "FEEDBACK"]);
console.out("Checking send: done");

// ----------------------------------------------------------------
// Bring feedback into the correct operational state
// ----------------------------------------------------------------
console.out("Feedback init: start.");
service_feedback.get(5000);

dim.send("FEEDBACK/ENABLE_OUTPUT", true);
dim.send("FEEDBACK/START_CURRENT_CONTROL", 0.);

v8.timeout(3000, function() { var n = dim.state("FEEDBACK").name; if (n=="CurrentCtrlIdle" || n=="CurrentControl") return true; });

// ----------------------------------------------------------------
// Connect to the DRS_RUNS service
// ----------------------------------------------------------------
console.out("Drs runs init: start.");

var sub_drsruns = new Subscription("FAD_CONTROL/DRS_RUNS");
sub_drsruns.get(5000);
// FIXME: Check if the last DRS calibration was complete?

function getTimeSinceLastDrsCalib()
{
    // ----- Time since last DRS Calibration [min] ------
    var runs = sub_drsruns.get(0);
    var diff = (new Date()-runs.time)/60000;

    // Warning: 'roi=300' is a number which is not intrisically fixed
    //          but can change depending on the taste of the observers
    var valid = runs.obj['run'][2]>0 && runs.obj['roi']==300;

    if (valid)
        console.out("  Last DRS calib: %.1fmin ago".$(diff));
    else
        console.out("  No valid drs calibration available");

    return valid ? diff : null;
}

// ----------------------------------------------------------------
// Make sure we will write files
// ----------------------------------------------------------------
dim.send("FAD_CONTROL/SET_FILE_FORMAT", 2);

// ----------------------------------------------------------------
// Print some information for the user about the
// expected first oberservation
// ----------------------------------------------------------------
var test = getObservation();
if (test!=undefined)
{
    var n = new Date();
    if (test==-1)
        console.out(n.toUTCString()+": First observation scheduled for "+observations[0].start.toUTCString());
    if (test>=0 && test<observations.length)
        console.out(n.toUTCString()+": First observation should start immediately.");
    if (observations[0].start>n+12*3600*1000)
        console.out(n.toUTCString()+": No observations scheduled for the next 12 hours!");
}

// ----------------------------------------------------------------
// Start main loop
// ----------------------------------------------------------------
console.out("Start main loop.");

var run = -2; // getObservation never called
var sub;
var lastObs;
var sun = Sun.horizon(-13);
var system_on;  // undefined

while (1)
{
    // Check if observation position is still valid
    // If source position has changed, set run=0
    var idxObs = getObservation();
    if (idxObs===undefined)
        break;

    // we are still waiting for the first observation in the schedule
    if (idxObs==-1)
    {
        // flag that the first observation will be in the future
        run = -1; 
        v8.sleep(1000);
        continue;
    }

    // Check if we have to take action do to sun-rise
    var was_up = sun.isUp;
    sun = Sun.horizon(-13);
    if (!was_up && sun.isUp)
    {
        console.out("", "Sun rise detected.... automatic shutdown initiated!");
        // FIXME: State check?
        Shutdown();
        system_on = false;
        continue;
    }

    // Current and next observation target
    var obs     = observations[idxObs];
    var nextObs = observations[idxObs+1];

    // Check if observation target has changed
    if (lastObs!=idxObs) // !Object.isEqual(obs, nextObs)
    {
        console.out("--- "+idxObs+" ---");
        console.out("Current time:        "+new Date().toUTCString());
        console.out("Current observation: "+obs.start.toUTCString());
        if (nextObs!=undefined)
            console.out("Next    observation: "+nextObs.start.toUTCString());
        console.out("");

        // This is the first source, but we do not come from
        // a scheduled 'START', so we have to check if the
        // telescop is operational already
        sub = 0;
        if (run<0)
        {
            //Startup();   // -> Bias On/Off?, Lid open/closed?
            //CloseLid();
        }

        // The first observation had a start-time in the past...
        // In this particular case start with the last entry
        // in the list of measurements
        if (run==-2)
            sub = obs.length-1;

        run = 0;
    }
    lastObs = idxObs;

    if (nextObs==undefined && obs[obs.length-1].task!="SHUTDOWN")
        throw Error("Last scheduled measurement must be a shutdown.");

    // We are done with all measurement slots for this
    // observation... wait for next observation
    if (sub>=obs.length)
    {
        v8.sleep(1000);
        continue;
    }

    var task = obs[sub].task;

    if (system_on===false && task!="STARTUP")
    {
        v8.sleep(1000);
        continue;
    }

    // Check if sun is still up... only DATA and RATESCAN must be suppressed
    if ((task=="DATA" || task=="RATESCAN") && sun.isUp)
    {
        var now = new Date();
        var remaining = (sun.set - now)/60000;
        console.out(now.toUTCString()+" - "+obs[sub].task+": Sun above FACT-horizon: sleeping 1min, remaining %.1fmin".$(remaining));
        v8.sleep(60000);
        continue;
    }

    console.out("\n"+(new Date()).toUTCString()+": Current measurement: "+obs[sub]);

    var power_states = sun.isUp || system_on===false ? [ "DriveOff" ] : [ "SystemOn" ];
    var drive_states = sun.isUp || system_on===false ?   undefined    : [ "Armed", "Tracking", "OnTrack" ];

    // A scheduled task was found, lets check if all servers are
    // still only and in reasonable states. If this is not the case,
    // something unexpected must have happend and the script is aborted.
    //console.out("  Checking states [general]");
    var table =
        [
         [ "TNG_WEATHER"   ],
         [ "MAGIC_WEATHER" ],
         [ "CHAT"          ],
         [ "SMART_FACT"    ],
         [ "TEMPERATURE"   ],
         [ "DATA_LOGGER",     [ "NightlyFileOpen", "WaitForRun", "Logging" ] ],
         [ "FSC_CONTROL",     [ "Connected"                ] ],
         [ "MCP",             [ "Idle"                     ] ],
         [ "TIME_CHECK",      [ "Valid"                    ] ],
         [ "PWR_CONTROL",     power_states/*[ "SystemOn"                 ]*/ ],
//         [ "AGILENT_CONTROL", [ "VoltageOn"                ] ],
         [ "BIAS_CONTROL",    [ "VoltageOff", "VoltageOn", "Ramping" ] ],
         [ "FEEDBACK",        [ "CurrentControl", "CurrentCtrlIdle" ] ],
         [ "LID_CONTROL",     [ "Open", "Closed"           ] ],
         [ "DRIVE_CONTROL",   drive_states/*[ "Armed", "Tracking", "OnTrack" ]*/ ],
         [ "FTM_CONTROL",     [ "Idle", "TriggerOn"        ] ],
         [ "FAD_CONTROL",     [ "Connected", "WritingData" ] ],
         [ "RATE_SCAN",       [ "Connected"                ] ],
         [ "RATE_CONTROL",    [ "Connected", "GlobalThresholdSet", "InProgress"  ] ],
        ];

    if (!checkStates(table))
    {
        throw new Error("Something unexpected has happened. One of the servers"+
                        "is in a state in which it should not be. Please,"+
                        "try to find out what happened...");
    }

    datalogger_subscriptions.check();

    // Check if obs.task is one of the one-time-tasks
    switch (obs[sub].task)
    {
    case "STARTUP":
        console.out("  STARTUP", "");
        CloseLid();

        doDrsCalibration("startup");  // will switch the voltage off

        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        // Before we can switch to 3000 we have to make the right DRS calibration
        console.out("  Take single p.e. run.");
        while (!takeRun("pedestal", 5000));

        // It is unclear what comes next, so we better switch off the voltage
        service_feedback.voltageOff();
        system_on = true;
        break;

    case "SHUTDOWN":
        console.out("  SHUTDOWN", "");
        Shutdown();
        system_on = false;

        // FIXME: Avoid new observations after a shutdown until
        //        the next startup (set run back to -2?)
        console.out("  Waiting for next startup.", "");
        sub++;
        continue;

    case "IDLE":
        v8.sleep(1000);
        continue;

    case "DRSCALIB":
        console.out("  DRSCALIB", "");
        doDrsCalibration("drscalib");  // will switch the voltage off
        break;

    case "SINGLEPE":
        console.out("  SINGLE-PE", "");

        // The lid must be closes
        CloseLid();

        // Check if DRS calibration is necessary
        var diff = getTimeSinceLastDrsCalib();
        if (diff>30 || diff==null)
            doDrsCalibration("singlepe");  // will turn voltage off

        // The voltage must be on
        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        // Before we can switch to 3000 we have to make the right DRS calibration
        console.out("  Take single p.e. run.");
        while (!takeRun("pedestal", 5000));

        // It is unclear what comes next, so we better switch off the voltage
        service_feedback.voltageOff();
        break;

    case "RATESCAN":
        console.out("  RATESCAN", "");

        var tm1 = new Date();

        // This is a workaround to make sure that we really catch
        // the new state and not the old one
        dim.send("DRIVE_CONTROL/STOP");
        dim.wait("DRIVE_CONTROL", "Armed", 5000);

        // The lid must be open
        OpenLid();

        // The voltage must be switched on
        service_feedback.voltageOn();

        if (obs.source != undefined)
            dim.send("DRIVE_CONTROL/TRACK_ON", obs[sub].source);
        else
            dim.send("DRIVE_CONTROL/TRACK", obs[sub].ra, obs[sub].dec);

        dim.wait("DRIVE_CONTROL", "OnTrack", 150000); // 110s for turning and 30s for stabilizing

        service_feedback.waitForVoltageOn();

        var tm2 = new Date();

        // Start rate scan
        dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 50, 1000, -10);

        // Lets wait if the ratescan really starts... this might take a few
        // seconds because RATE_SCAN configures the ftm and is waiting for
        // it to be configured.
        dim.wait("RATE_SCAN", "InProgress", 10000);
        dim.wait("RATE_SCAN", "Connected", 2700000);

        // this line is actually some kind of hack. 
        // after the Ratescan, no data is written to disk. I don't know why, but it happens all the time
        // So I decided to put this line here as a kind of patchwork....
        //dim.send("FAD_CONTROL/SET_FILE_FORMAT", 2);

        console.out("  Ratescan done [%.1fs, %.1fs]".$((tm2-tm1)/1000, (new Date()-tm2)/1000));
        break; // case "RATESCAN"

    case "DATA":

        // ========================== case "DATA" ============================
    /*
        if (Sun.horizon("FACT").isUp)
        {
            console.out("  SHUTDOWN","");
            Shutdown();
            console.out("  Exit forced due to broken schedule", "");
            exit();
        }
    */
        // Calculate remaining time for this observation in minutes
        var remaining = nextObs==undefined ? 0 : (nextObs.start-new Date())/60000;

        // ------------------------------------------------------------

        console.out("  Run #"+run+"  (remaining "+parseInt(remaining)+"min)");

        // ----- Time since last DRS Calibration [min] ------
        var diff = getTimeSinceLastDrsCalib();

        // Changine pointing position and take calibration...
        //  ...every four runs (every ~20min)
        //  ...if at least ten minutes of observation time are left
        //  ...if this is the first run on the source
        var point  = (run%4==0 && remaining>10) || run==0;

        // Take DRS Calib...
        //  ...every four runs (every ~20min)
        //  ...at last  every two hours
        //  ...when DRS temperature has changed by more than 2deg (?)
        //  ...when more than 15min of observation are left
        //  ...no drs calibration was done yet
        var drscal = (run%4==0 && (remaining>15 && diff>70)) || diff==null;

        if (point)
        {
            // Change wobble position every four runs,
            // start with alternating wobble positions each day
            var wobble = (parseInt(run/4) + parseInt(new Date()/1000/3600/24-0.5))%2+1;

            //console.out("  Move telescope to '"+source+"' "+offset+" "+wobble);
            console.out("  Move telescope to '"+obs[sub].source+"' ["+wobble+"]");

            //var offset = observations[obs][2];
            //var wobble = observations[obs][3 + parseInt(run/4)%2];

            //dim.send("DRIVE_CONTROL/TRACK_SOURCE", offset, wobble, source);

            dim.send("DRIVE_CONTROL/TRACK_WOBBLE", wobble, obs[sub].source);

            // Do we have to check if the telescope is really moving?
            // We can cross-check the SOURCE service later
        }

        if (drscal)
            doDrsCalibration("data");  // will turn voltage off

        OpenLid();

        // voltage must be switched on after the lid is open for the
        // feedback to adapt the voltage properly to the night-sky
        // background light level.
        service_feedback.voltageOn();

        // This is now th right time to wait for th drive to be stable
        dim.wait("DRIVE_CONTROL", "OnTrack", 150000); // 110s for turning and 30s for stabilizing

        // Now we have to be prepared for data-taking:
        // make sure voltage is on
        service_feedback.waitForVoltageOn();

        // If pointing had changed, do calibration
        if (point)
        {
            console.out("  Calibration.");

            // Calibration (2% of 20')
            while (1)
            {
                if (!takeRun("pedestal",         1000))  // 80 Hz  -> 10s
                    continue;
                if (!takeRun("light-pulser-ext", 1000))  // 80 Hz  -> 10s
                    continue;
                break;
            }
        }

        console.out("  Taking data: start [5min]");

        var len = 300;
        while (len>0)
        {
            var time = new Date();
            if (takeRun("data", -1, len)) // Take data (5min)
                break;

            len -= parseInt((new Date()-time)/1000);
        }

        console.out("  Taking data: done");
        run++;

        continue; // case "DATA"
    }

    if (nextObs!=undefined && sub==obs.length-1)
        console.out("  Waiting for next observation scheduled for "+nextObs.start.toUTCString(),"");

    sub++;
}

sub_drsruns.close();

// ================================================================
// Comments and ToDo goes here
// ================================================================

// error handline : http://www.sitepoint.com/exceptional-exception-handling-in-javascript/
// classes: http://www.phpied.com/3-ways-to-define-a-javascript-class/
//
// Arguments: TakeFirstDrsCalib
// To be determined: How to stop the script without foreceful interruption?
