/**
 * @fileOverview This file has functions related to documenting JavaScript.
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
'use strict';

dim.log("Start: "+__FILE__+" ["+__DATE__+"]");

// This should be set in dimctrl.rc as JavaScript.schedule-database.
// It is sent together with the script to the dimserver.
// If started directly, it has to be set after the command:
//
//   .js scripts/Main.js schedule-database=...
//
if (!$['schedule-database'])
    throw new Error("Environment 'schedule-database' not set!");

//dimctrl.defineState(37, "TimeOutBeforeTakingData", "MCP took more than 5minutes to start TakingData");

// ================================================================
//  Code related to the schedule
// ================================================================

//this is just the class implementation of 'Observation'
include('scripts/Observation_class.js');
include('scripts/getSchedule.js');

var observations = [ ];

// Get the observation scheduled for 'now' from the table and
// return its index
function getObservation(now)
{
    if (now==undefined)
        now = new Date();

    if (isNaN(now.valueOf()))
        throw new Error("Date argument in getObservation invalid.");

    observations = getSchedule();
    if (observations.length==0)
        return -1;

    var suspended = -1;

    var rc = observations.length-1;

    for (var i=0; i<observations.length; i++)
    {
        // Find the first observation which is in the future
        if (observations[i].start>now)
        {
            rc = i-1;
            break;
        }

        // If the loop just passed a suspend, i.e. it is in the past, set the suspend flag
        if (observations[i][0].task=="SUSPEND")
            suspended = i;

        // If the loop just passed a resume, i.e. it is in the past, remove the suspend flag
        if (observations[i][0].task=="RESUME")
            suspended = -1;
    }

    // Now rc contains the index of the first observation in the past

    // If the system is in suspend mode, the suspend observation is
    // returned as recent measurement. For convenience (to avoid that
    // the next observation is announced), all future observations
    // are removed.
    if (suspended>=0)
    {
        observations.splice(suspended+1);
        return suspended;
    }

    // Observations have already been resumed and the last scheduled
    // observation is the resume itself: remove the resume and
    // all leading suspend/resume pairs until the last observation
    // is found
    while (rc>=0 && observations[rc][0].task=="RESUME")
    {
        observations.splice(rc--, 1);

        // Find previous suspend
        if (rc>=0 && observations[rc][0].task=="SUSPEND")
            observations.splice(rc--, 1);
    }

    return rc;
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

// ================================================================
//  Code to perform the DRS calib sequence
// ================================================================

var irq;

function doDrsCalibration(where)
{
    dim.log("Starting DRS calibration ["+where+"]");

    service_feedback.voltageOff();

    var tm = new Date();

    while (!irq)
    {
        dim.send("FAD_CONTROL/START_DRS_CALIBRATION");
        if (irq || !takeRun("drs-pedestal", 1000))     // 40 / 20s     (50Hz)
            continue;

        if (irq || !takeRun("drs-gain",     1000))     // 40 / 20s     (50Hz)
            continue;

        if (where!="data")
        {
            if (irq || !takeRun("drs-pedestal", 1000))     // 40 / 20s     (50Hz)
                continue;
        }

        break;
    }

    if (where!="data")
    {
        dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

        while (!irq && !takeRun("drs-pedestal", 1000));     // 40 / 20s     (50Hz)
        while (!irq && !takeRun("drs-time",     1000));     // 40 / 20s     (50Hz)
    }

    while (!irq)
    {
        dim.send("FAD_CONTROL/RESET_SECONDARY_DRS_BASELINE");
        if (takeRun("pedestal",     1000))              // 40 / 10s     (80Hz)
            break;
    }

    dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

    while (!irq && !takeRun("pedestal",     1000));     // 40 / 10s     (80Hz)
    //                                                   -----------
    //                                                   4'40 / 2'00

    if (irq)
        dim.log("DRS calibration interrupted [%.1fs]".$((new Date()-tm)/1000));
    else
        dim.log("DRS calibration done [%.1fs]".$((new Date()-tm)/1000));
}

// ================================================================
//  Code related to the lid
// ================================================================

function OpenLid(fast)
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
    var isInconsistent = dim.state("LID_CONTROL").name=="Inconsistent";

    var tm = new Date();

    // Wait for lid to be open
    if (isClosed || isInconsistent)
    {
        dim.log("Opening lid");
        dim.send("LID_CONTROL/OPEN");

        dim.log("Turning off IR camera LEDs...");

        var cam = new Curl("fact@cam/cgi-bin/user/Config.cgi");
        cam.data.push("action=set");
        cam.data.push("Camera.System.Title=Camera1");
        cam.data.push("Camera.General.IRControl.Value=2");
        cam.data.push("Camera.System.Display=ALL");
        cam.data.push("Camera.Environment=OUTDOOR");
        var ret = cam.send();
        dim.log("Camera response: "+ret.data.replace(/\n/g,"/")+" ["+ret.rc+"]");
    }

    if (!fast)
        return;

    dim.wait("LID_CONTROL", "Open", 30000);

    if (isClosed || isInconsistent)
        dim.log("Lid open [%.1fs]".$((new Date()-tm)/1000));
}

function CloseLid()
{
    var isOpen = dim.state("LID_CONTROL").name=="Open";

    var tm = new Date();

    // Wait for lid to be open
    if (isOpen)
    {
        if (dim.state("FTM_CONTROL").name=="TriggerOn")
        {
            dim.send("FTM_CONTROL/STOP_TRIGGER");
            dim.wait("FTM_CONTROL", "Valid", 3000);
        }

        dim.log("Closing lid.");
        dim.send("LID_CONTROL/CLOSE");
    }
    v8.timeout(30000, function() { if (dim.state("LID_CONTROL").name=="Closed" || dim.state("LID_CONTROL").name=="Inconsistent") return true; });
    //dim.wait("LID_CONTROL", "Closed", 30000);
    //dim.wait("LID_CONTROL", "Inconsistent", 30000);

    if (isOpen)
        dim.log("Lid closed [%.1fs]".$((new Date()-tm)/1000));
}

// ================================================================
//  Interrupt data taking in case of high currents
// ================================================================
dim.onchange['FEEDBACK'] = function(state)
{
    if ((state.name=="Critical" || state.name=="OnStandby") &&
        (this.prev!="Critical"  && this.prev!="OnStandby"))
    {
        console.out("Feedback state changed from "+this.prev+" to "+state.name+" [Main.js]");
        irq = "RESCHEDULE";
    }
    this.prev=state.name;
}

// ================================================================
//  Code related to switching bias voltage on and off
// ================================================================

var service_feedback = new Subscription("FEEDBACK/CALIBRATED_CURRENTS");

service_feedback.onchange = function(evt)
{
    if (!evt.data)
        return;

    if (this.ok==undefined)
        return;

    var Unom = evt.obj['U_nom'];
    var Uov  = evt.obj['U_ov'];
    if (!Uov)
        return;

    var cnt = 0;
    var avg = 0;
    for (var i=0; i<320; i++)
    {
        // This is a fix for the channel with a shortcut
        if (i==272)
            continue;

        var dU = Uov[i]-Unom;

        // 0.022 corresponds to 1 DAC count (90V/4096)
        if (Math.abs(dU)>0.033)
            cnt++;

        avg += dU;
    }
    avg /= 320;

    console.out("  DeltaUov=%.3f (%.3f) [N(>0.033V)=%d]".$(avg, avg-this.last, cnt));

    this.ok = cnt<3;// || (this.last!=undefined && Math.abs(this.last-avg)<0.002);
    this.last = avg;
}

service_feedback.voltageOff = function()
{
    var state = dim.state("BIAS_CONTROL").name;

    if (state=="Disconnected")
    {
        console.out("  Voltage off: bias crate disconnected!");
        return;
    }

    // check of feedback has to be switched on
    var isOn = state=="VoltageOn" || state=="Ramping";
    if (isOn)
    {
        dim.log("Switching voltage off.");

        if (dim.state("FTM_CONTROL").name=="TriggerOn")
        {
            dim.send("FTM_CONTROL/STOP_TRIGGER");
            dim.wait("FTM_CONTROL", "Valid", 3000);
        }

        // Supress the possibility that the bias control is
        // ramping and will reject the command to switch the
        // voltage off
        //dim.send("FEEDBACK/STOP");
        //dim.wait("FEEDBACK", "Calibrated", 3000);

        // Make sure we are not in Ramping anymore
        //dim.wait("BIAS_CONTROL", "VoltageOn", 3000);

        // Switch voltage off
        dim.send("BIAS_CONTROL/SET_ZERO_VOLTAGE");
    }

    dim.wait("BIAS_CONTROL", "VoltageOff", 60000); // FIXME: 30000?
    dim.wait("FEEDBACK",     "Calibrated",  3000);

    // FEEDBACK stays in CurrentCtrl when Voltage is off but output enabled
    // dim.wait("FEEDBACK", "CurrentCtrlIdle", 1000);

    if (isOn)
        dim.log("Voltage off.");
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

service_feedback.voltageOn = function(ov)
{
    if (isNaN(ov))
        ov = 1.1;

    if (this.ov!=ov && dim.state("FEEDBACK").name=="InProgress") // FIXME: Warning, OnStandby, Critical if (ov<this.ov)
    {
        dim.log("Stoping feedback.");
        if (dim.state("FTM_CONTROL").name=="TriggerOn")
        {
            dim.send("FTM_CONTROL/STOP_TRIGGER");
            dim.wait("FTM_CONTROL", "Valid", 3000);
        }

        dim.send("FEEDBACK/STOP");
        dim.wait("FEEDBACK", "Calibrated", 3000);

        // Make sure we are not in Ramping anymore
        dim.wait("BIAS_CONTROL", "VoltageOn", 3000);
    }

    var isOff = dim.state("FEEDBACK").name=="Calibrated";
    if (isOff)
    {
        dim.log("Switching voltage to Uov="+ov+"V.");

        dim.send("FEEDBACK/START", ov);

        // FIXME: We could miss "InProgress" if it immediately changes to "Warning"
        //        Maybe a dim.timeout state>8 ?
        dim.wait("FEEDBACK", "InProgress", 45000);

        this.ov = ov;
    }

    // Wait until voltage on
    dim.wait("BIAS_CONTROL", "VoltageOn", 60000); // FIXME: 30000?
}

service_feedback.waitForVoltageOn = function()
{
    // Avoid output if condition is already fulfilled
    dim.log("Waiting for voltage to be stable.");

    function func()
    {
        if (irq || this.ok==true)
            return true;
    }

    var now = new Date();

    this.last = undefined;
    this.ok = false;
    v8.timeout(4*60000, func, this); // FIMXE: Remove 4!
    this.ok = undefined;

    if (irq)
        dim.log("Waiting for stable voltage interrupted.");
    else
        dim.log("Voltage stable within limits");
}

// ================================================================
//  Function to shutdown the system
// ================================================================

function Shutdown(type)
{
    if (!type)
        type = "default";

    dim.log("Starting shutdown ["+type+"].");

    var now1 = new Date();

    var bias = dim.state("BIAS_CONTROL").name;
    if (bias=="VoltageOn" || bias=="Ramping")
        service_feedback.voltageOn(0);

    CloseLid();

    var now2 = new Date();

    dim.send("DRIVE_CONTROL/PARK");

    console.out("","Waiting for telescope to park. This may take a while.");

    // FIXME: This might not work is the drive is already close to park position
    //dim.wait("DRIVE_CONTROL", "Parking", 3000);

    /*
    // Check if DRS calibration is necessary
    var diff = getTimeSinceLastDrsCalib();
    if (diff>30 || diff==null)
    {
        doDrsCalibration("singlepe");  // will turn voltage off
        if (irq)
            break;
    }*/

    //take single pe run if required
    if (type=="singlepe")
    {
        dim.log("Taking single-pe run.");

        // The voltage must be on
        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        // Before we can switch to 3000 we have to make the right DRS calibration
        dim.log("Taking single p.e. run.");
        while (!irq && !takeRun("single-pe", 10000));

        /*
         Maybe we need to send a trigger... but data runs contain pedestal triggers... so it should work in any case...
        var customRun = function()
            {
                v8.sleep(500);//wait that configuration is set
                dim.wait("FTM_CONTROL", "TriggerOn", 15000);
                dim.send("FAD_CONTROL/SEND_SINGLE_TRIGGER");
                dim.send("RATE_CONTROL/STOP");
                dim.send("FTM_CONTROL/STOP_TRIGGER");
                dim.wait("FTM_CONTROL", "Valid", 3000);
                dim.send("FTM_CONTROL/ENABLE_TRIGGER", true);
                dim.send("FTM_CONTROL/SET_TIME_MARKER_DELAY", 123);
                dim.send("FTM_CONTROL/SET_THRESHOLD", -1, obs[sub].threshold);
                v8.sleep(500);//wait that configuration is set
                dim.send("FTM_CONTROL/START_TRIGGER");
                dim.wait("FTM_CONTROL", "TriggerOn", 15000);
            }*/
    }

    //wait until drive is in locked (after it reached park position)
    dim.wait("DRIVE_CONTROL", "Locked", 150000);

    //unlock drive if task was sleep
    if (type=="unlock")
        dim.send("DRIVE_CONTROL/UNLOCK");


    // It is unclear what comes next, so we better switch off the voltage
    service_feedback.voltageOff();

    dim.log("Finishing shutdown.");

    var now3 = new Date();

    dim.send("FTM_CONTROL/STOP_TRIGGER");
    dim.wait("FTM_CONTROL",  "Valid",        3000);

    if (bias!="Disconnected")
        dim.wait("FEEDBACK", "Calibrated",   3000);

    if (type!="unlock")
    {
        dim.send("BIAS_CONTROL/DISCONNECT");

        var pwrctrl_state = dim.state("PWR_CONTROL").name;
        if (pwrctrl_state=="SystemOn" ||
            pwrctrl_state=="BiasOff"  ||
            pwrctrl_state=="DriveOn")
            dim.send("PWR_CONTROL/TOGGLE_DRIVE");

        dim.wait("BIAS_CONTROL", "Disconnected", 3000);
        dim.wait("PWR_CONTROL",  "DriveOff",     6000);
    }

    var sub = new Subscription("DRIVE_CONTROL/POINTING_POSITION");
    sub.get(5000);  // FIXME: Proper error message in case of failure

    var report = sub.get();

    console.out("");
    console.out("Shutdown procedure ["+type+"] seems to be finished...");
    console.out("  "+new Date().toUTCString());
    console.out("  Telescope at Zd=%.1fdeg Az=%.1fdeg".$(report.obj['Zd'], report.obj['Az']));
    console.out("  Please check on the web cam that the park position was reached");
    console.out("  and the telescope is not moving anymore.");
    console.out("  Please check visually that the lid is really closed and");
    console.out("  that the biasctrl really switched the voltage off.", "");
    console.out("    DRIVE_CONTROL: "+dim.state("DRIVE_CONTROL").name);
    console.out("    FEEDBACK:      "+dim.state("FEEDBACK").name);
    console.out("    FTM_CONTROL:   "+dim.state("FTM_CONTROL").name);
    console.out("    BIAS_CONTROL:  "+dim.state("BIAS_CONTROL").name);
    console.out("    PWR_CONTROL:   "+dim.state("PWR_CONTROL").name);
    console.out("");
    dim.log("Shutdown: end ["+(now2-now1)/1000+"s, "+(now3-now2)/1000+"s, "+(new Date()-now3)/1000+"s]");
    console.out("");

    sub.close();
}


// ================================================================
//  Function to set the system to sleep-mode
// ================================================================
// FIXME: do not repeat code from shutdown-function
/*
function GoToSleep()
{
    CloseLid();

    var isArmed = dim.state("DRIVE_CONTROL").name=="Armed";
    if (!isArmed)
    {
        dim.log("Drive not ready to move. -> send STOP");
        dim.send("DRIVE_CONTROL/STOP");
        dim.wait("DRIVE_CONTROL", "Armed", 5000);
    }

    dim.send("DRIVE_CONTROL/MOVE_TO 101 0");//park position
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

    try { v8.timeout(150000, func); }
    catch (e)
    {
        var p = sub.get();
        dim.log('Park position not reached? Telescope at Zd='+p.obj['Zd']+' Az='+p.obj['Az']);
    }
    var p2 = sub.get();
    dim.log('Telescope at Zd=%.1fdeg Az=%.1fdeg'.$(p2.obj['Zd'], p2.obj['Az']));
    sub.close();
}
*/

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
         "AGILENT_CONTROL_24V/DATA",
         "AGILENT_CONTROL_50V/DATA",
         "AGILENT_CONTROL_80V/DATA",
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
         "FAD_CONTROL/STATS",
         "FAD_CONTROL/STATUS",
         "FAD_CONTROL/TEMPERATURE",
         "FEEDBACK/CALIBRATED_CURRENTS",
         "FEEDBACK/CALIBRATION",
         "FEEDBACK/CALIBRATION_R8",
         "FEEDBACK/CALIBRATION_STEPS",
/*         "FEEDBACK/REFERENCE",*/
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
         "GPS_CONTROL/NEMA",
         "SQM_CONTROL/DATA",
         //"LID_CONTROL/DATA",
         "LID_CONTROL/MOTORS",
         "MAGIC_LIDAR/DATA",
         "MAGIC_WEATHER/DATA",
         "RAIN_SENSOR/DATA",
         "MCP/CONFIGURATION",
         "PWR_CONTROL/DATA",
         "RATE_CONTROL/THRESHOLD",
         "RATE_SCAN/DATA",
         "RATE_SCAN/PROCESS_DATA",
         "TEMPERATURE/DATA",
         "TIME_CHECK/OFFSET",
         "TNG_WEATHER/DATA",
         "TNG_WEATHER/DUST",
         "GTC_DUST/DATA",
         "PFMINI_CONTROL/DATA",
         "BIAS_TEMP/DATA",
         "GUDE_CONTROL/DATA",
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
        if (list.every(function(el){return el==undefined || el[0]!=name;}))
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

// ================================================================
//  Code to monitor clock conditioner
// ================================================================

var sub_counter = new Subscription("FTM_CONTROL/COUNTER");
sub_counter.onchange = function(evt)
{
    if (evt.qos>0 && evt.qos!=2 && (evt.qos&0x100)==0)
        throw new Error("FTM reports: clock conditioner not locked.");
}

// ================================================================
//  Code related to monitoring the fad system
// ================================================================

// This code is here, because scripts/Startup.js needs the
// same subscriptions... to be revised.
var sub_incomplete = new Subscription("FAD_CONTROL/INCOMPLETE");
var sub_connections = new Subscription("FAD_CONTROL/CONNECTIONS");
var sub_startrun = new Subscription("FAD_CONTROL/START_RUN");
sub_startrun.get(5000);

include('scripts/takeRun.js');

// ----------------------------------------------------------------
// Check that everything we need is availabel to receive commands
// (FIXME: Should that go to the general CheckState?)
// ----------------------------------------------------------------
//console.out("Checking send.");
checkSend(["MCP", "DRIVE_CONTROL", "LID_CONTROL", "FAD_CONTROL", "FEEDBACK"]);
//console.out("Checking send: done");

// ----------------------------------------------------------------
// Bring feedback into the correct operational state
// ----------------------------------------------------------------
//console.out("Feedback init: start.");
service_feedback.get(5000);

// ----------------------------------------------------------------
// Connect to the DRS_RUNS service
// ----------------------------------------------------------------
//console.out("Drs runs init: start.");

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
        dim.log("Last DRS calibration was %.1fmin ago".$(diff));
    else
        dim.log("No valid DRS calibration available.");

    return valid ? diff : null;
}

// ----------------------------------------------------------------
// Install interrupt handler
// ----------------------------------------------------------------
var triggerReloadSources = false;

function handleIrq(cmd, args, time, user)
{
    console.out("Interrupt received:");
    console.out("  IRQ:  "+cmd);
    console.out("  Time: "+time);
    if (user)
        console.out("  User: "+user);

    if (cmd.toUpperCase()=="RELOADSOURCES")
    {
        dim.log("Reloading source table scheduled before next pointing operation.");
        triggerReloadSources = true;
        return;
    }

    irq = cmd ? cmd : "stop";

    dim.send("MCP/STOP");

    if (irq.toUpperCase()=="PREPARE")
    {
        //triggerReloadSources = true;
        dim.send("DRIVE_CONTROL/PREPARE_GRB"); // STOP + RELOAD_SOURCES
        irq = undefined;
    }

    /*
	if (fDimFTM.state()==FTM::State::kTriggerOn)
	{
            Message("Stopping FTM");
	    Dim::SendCommandNB("FTM_CONTROL/STOP_TRIGGER");
	}

        // FIXME: Do step 2 only when FTM is stopped
        if (fDimFAD.state()==FAD::State::kConnected || fDimFAD.state()==FAD::State::kRunInProgress)
        {
            if (fDimFAD.state()==FAD::State::kRunInProgress)
                Dim::SendCommandNB("FAD_CONTROL/CLOSE_OPEN_FILES");
        }

        return GetCurrentState();
     */

    // This will stop a rate scan in progress
    if (dim.state("RATE_SCAN").name=="InProgress")
        dim.send("RATE_SCAN/STOP");
}

dimctrl.setInterruptHandler(handleIrq);

// ----------------------------------------------------------------
// Make sure we will write files
// ----------------------------------------------------------------
dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

// ----------------------------------------------------------------
// Print some information for the user about the
// expected first oberservation
// ----------------------------------------------------------------
var test = getObservation();
if (test!=undefined)
{
    var n = new Date();
    if (observations.length>0 && test==-1)
        dim.log("First observation scheduled for "+observations[0].start.toUTCString()+" [id="+observations[0].id+"]");
    if (test>=0 && test<observations.length)
        dim.log("First observation should start immediately ["+observations[test].start.toUTCString()+", id="+observations[test].id+"]");
    if (observations.length>0 && observations[0].start>n+12*3600*1000)
        dim.log("No observations scheduled for the next 12 hours!");
    if (observations.length==0)
        dim.log("No observations scheduled!");
}

// ----------------------------------------------------------------
// Start main loop
// ----------------------------------------------------------------
dim.log("Entering main loop.");
console.out("");

var run = -2; // getObservation never called
var sub;
var lastId;
var nextId;
var sun = Sun.horizon(-12);
var system_on;  // undefined

if (sun.observatory!="ORM")
    throw new Error("Default observatory set to '"+sun.observatory+"' not as it ought to be [ORM]!");

function processIrq()
{
    if (!irq)
        return false;

    while (irq.toUpperCase()=="PREPARE")
        v8.sleep(10);

    if (irq.toUpperCase()=="RESCHEDULE")
    {
        irq = undefined;
        return false;
    }

    if (irq.toUpperCase()=="OFF")
    {
        service_feedback.voltageOff();
        dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
        return true;
    }

    /*
    if (irq.toUpperCase()=="STOP")
    {
        dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
        dim.send("MCP/STOP");
        return true;
    }*/

    if (irq.toUpperCase()=="SHUTDOWN")
    {
        Shutdown();
        return true;
    }

    dim.log("IRQ "+irq+" unhandled... stopping script.");
    return true;
}

while (!processIrq())
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
    sun = Sun.horizon(-12);
    if (!was_up && sun.isUp)
    {
        console.out("");
        dim.log("Sun rise detected.... automatic shutdown initiated!");
        // FIXME: State check?
        Shutdown();
        system_on = false;
        continue;
    }

    // Current and next observation target
    var obs     = observations[idxObs];
    var nextObs = observations[idxObs+1];

    // Check if observation target has changed
    if (lastId!=obs.id) // !Object.isEqual(obs, nextObs)
    {
        dim.log("Starting new observation ["+obs.start.toUTCString()+", id="+obs.id+"]");

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
        lastId = obs.id;
    }

    //dim.log("DEBUG: Next observation scheduled for "+nextObs.start.toUTCString()+" [id="+nextObs.id+"]");
    if (nextObs && nextId!=nextObs.id)
    {
        dim.log("Next observation scheduled for "+nextObs.start.toUTCString()+" [id="+nextObs.id+"]");
        console.out("");
        nextId = nextObs.id;
    }

    if (!nextObs && nextId)
    {
        if (obs[sub].task=="SUSPEND")
            dim.log("Further observation suspended.");
        else
            dim.log("No further observation scheduled.");
        console.out("");
        nextId = undefined;
    }

    //if (nextObs==undefined && obs[obs.length-1].task!="SHUTDOWN")
    //    throw Error("Last scheduled measurement must be a shutdown.");

    // We are done with all measurement slots for this
    // observation... wait for next observation
    if (sub>=obs.length)
    {
        v8.sleep(1000);
        continue;
    }

    if (system_on===false && obs[sub].task!="STARTUP")
    {
        v8.sleep(1000);
        continue;
    }

    // Check if sun is still up... only DATA and */
    if ((obs[sub].task=="DATA" || obs[sub].task=="RATESCAN" || obs[sub].task=="RATESCAN2" ) && sun.isUp)
    {
        var now = new Date();
        var remaining = (sun.set - now)/60000;
        console.out(now.toUTCString()+" - "+obs[sub].task+": Sun above FACT-horizon: sleeping 1min, remaining %.1fmin".$(remaining));
        v8.sleep(60000);
        continue;
    }


    if (obs[sub].task!="IDLE" && (obs[sub].task!="DATA" && run>0))
        dim.log("New task ["+obs[sub]+"]");

    // FIXME: Maybe print a warning if Drive is on during day time!

    // It is not ideal that we allow the drive to be on during day time, but
    // otherwise it is difficult to allow e.g. the STARTUP at the beginning of the night
    var power_states = sun.isUp || !system_on ? [ "DriveOff", "SystemOn" ] : [ "SystemOn" ];
    var drive_states = sun.isUp || !system_on ? undefined : [ "Initialized", "Tracking", "OnTrack" ];

    if (drive_states && obs[sub].grb)
        drive_states.push("Stopping");

    // A scheduled task was found, lets check if all servers are
    // still only and in reasonable states. If this is not the case,
    // something unexpected must have happend and the script is aborted.
    //console.out("  Checking states [general]");
    var table =
        [
         [ "GTC_DUST"      ],
         [ "TNG_WEATHER"   ],
         [ "MAGIC_WEATHER" ],
         [ "RAIN_SENSOR"   ],
         [ "CHAT"          ],
         [ "SMART_FACT"    ],
         [ "TEMPERATURE"   ],
         [ "DATA_LOGGER",         [ "NightlyFileOpen", "WaitForRun", "Logging" ] ],
         [ "FSC_CONTROL",         [ "Connected"                ] ],
         [ "MCP",                 [ "Idle"                     ] ],
         [ "TIME_CHECK",          [ "Valid"                    ] ],
         [ "PWR_CONTROL",         power_states/*[ "SystemOn"                 ]*/ ],
         [ "AGILENT_CONTROL_24V", [ "VoltageOn"                ] ],
         [ "AGILENT_CONTROL_50V", [ "VoltageOn"                ] ],
         //[ "AGILENT_CONTROL_80V", [ "VoltageOn"                ] ],
         [ "AGILENT_CONTROL_80V", [ "VoltageOn", "Disconnected"       ] ], //hack to allow for data taking while agilent not available
         [ "BIAS_CONTROL",        [ "VoltageOff", "VoltageOn", "Ramping" ] ],
         [ "FEEDBACK",            [ "Calibrated", "InProgress", "OnStandby", "Warning", "Critical" ] ],
         [ "LID_CONTROL",         [ "Open", "Closed"           ] ],
         [ "DRIVE_CONTROL",       drive_states/*[ "Armed", "Tracking", "OnTrack" ]*/ ],
         [ "FTM_CONTROL",         [ "Valid", "TriggerOn"       ] ],
         [ "FAD_CONTROL",         [ "Connected", "RunInProgress" ] ],
         [ "RATE_SCAN",           [ "Connected"                ] ],
         [ "RATE_CONTROL",        [ "Connected", "GlobalThresholdSet", "InProgress"  ] ],
         [ "GPS_CONTROL",         [ "Locked"  ] ],
         [ "SQM_CONTROL",         [ "Disconnected", "Connected", "Valid" ] ],
         [ "PFMINI_CONTROL",      [ "Disconnected", "Connected", "Receiving" ] ],
         [ "BIAS_TEMP",           [ "Valid" ] ],
         [ "GUDE_CONTROL",        [ "Valid" ] ],
         [ "SCHEDULER",           [ /*"Connected",*/ "Armed" ] ],
        ];


    if (!checkStates(table))
    {
        throw new Error("Something unexpected has happened. One of the servers "+
                        "is in a state in which it should not be. Please,"+ 
                        "try to find out what happened...");
    }

    datalogger_subscriptions.check();

    // If this is an observation which needs the voltage to be swicthed on
    // skip that if the voltage is not stable
    /*
    if (obs[sub].task=="DATA" || obs[sub].task=="RATESCAN")
    {
        var state = dim.state("FEEDBACK").name;
        if (state=="Warning" || state=="Critical" || state=="OnStandby")
        {
            v8.sleep(1000);
            continue;
        }
    }*/


    // Check if obs.task is one of the one-time-tasks
    switch (obs[sub].task)
    {
    case "IDLE":
        v8.sleep(5000);
        continue;

    case "SUSPEND":
    case "SLEEP":
        Shutdown("unlock"); //GoToSleep();

        dim.log("Task finished ["+obs[sub].task+"].");
        console.out("");
        sub++;
        continue;

    case "STARTUP":
        CloseLid();

        doDrsCalibration("startup");  // will switch the voltage off

        if (irq)
            break;

        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        // Before we can switch to 3000 we have to make the right DRS calibration
        dim.log("Taking single p.e. run.");
        while (!irq && !takeRun("single-pe", 10000));

        // It is unclear what comes next, so we better switch off the voltage
        service_feedback.voltageOff();

        system_on = true;
        dim.log("Task finished [STARTUP]");
        console.out("");
        break;

    case "SHUTDOWN":
        Shutdown("singlepe");
        system_on = false;

        // FIXME: Avoid new observations after a shutdown until
        //        the next startup (set run back to -2?)
        sub++;
        dim.log("Task finished [SHUTDOWN]");
        console.out("");
        //console.out("  Waiting for next startup.", "");
        continue;

    case "DRSCALIB":
        doDrsCalibration("drscalib");  // will switch the voltage off
        dim.log("Task finished [DRSCALIB]");
        console.out("");
        break;

    case "SINGLEPE":
        // The lid must be closes
        CloseLid();

        // Check if DRS calibration is necessary
        var diff = getTimeSinceLastDrsCalib();
        if (diff>30 || diff==null)
        {
            doDrsCalibration("singlepe");  // will turn voltage off
            if (irq)
                break;
        }

        // The voltage must be on
        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        // Before we can switch to 3000 we have to make the right DRS calibration
        dim.log("Taking single p.e. run.");
        while (!irq && !takeRun("single-pe", 10000));

        // It is unclear what comes next, so we better switch off the voltage
        service_feedback.voltageOff();
        dim.log("Task finished [SINGLE-PE]");
        console.out("");
        break;

    case "OVTEST":
        var locked = dim.state("DRIVE_CONTROL").name=="Locked";
        if (!locked)
            dim.send("DRIVE_CONTROL/PARK");

        dim.send("FEEDBACK/STOP");

        // The lid must be closed
        CloseLid();

        if (!locked)
        {
            //console.out("Waiting for telescope to park. This may take a while.");
            dim.wait("DRIVE_CONTROL", "Locked", 3000);
            dim.send("DRIVE_CONTROL/UNLOCK");
        }

        // Check if DRS calibration is necessary
        var diff = getTimeSinceLastDrsCalib();
        if (diff>30 || diff==null)
        {
            doDrsCalibration("ovtest");  // will turn voltage off
            if (irq)
                break;
        }

        // The voltage must be on
        service_feedback.voltageOn(0.4);
        service_feedback.waitForVoltageOn();

        dim.log("Taking single p.e. run (0.4V)");
        while (!irq && !takeRun("single-pe", 10000));

        for (var i=5; i<18 && !irq; i++)
        {
            dim.send("FEEDBACK/STOP");
            dim.wait("FEEDBACK", "Calibrated", 3000);
            dim.wait("BIAS_CONTROL", "VoltageOn", 3000);
            dim.send("FEEDBACK/START", i*0.1);
            dim.wait("FEEDBACK", "InProgress", 45000);
            dim.wait("BIAS_CONTROL", "VoltageOn", 60000); // FIXME: 30000?
            service_feedback.waitForVoltageOn();
            dim.log("Taking single p.e. run ("+(i*0.1)+"V)");
            while (!irq && !takeRun("single-pe", 10000));
        }

        // It is unclear what comes next, so we better switch off the voltage
        service_feedback.voltageOff();
        dim.log("Task finished [OVTEST]");
        console.out("");
        break;

    case "RATESCAN":
        var tm1 = new Date();

        // This is a workaround to make sure that we really catch
        // the new OnTrack state later and not the old one
        dim.send("DRIVE_CONTROL/STOP");
        dim.wait("DRIVE_CONTROL", "Initialized", 15000);

        // The lid must be open
        OpenLid();

        // Switch the voltage to a reduced level (Ubd)
        service_feedback.voltageOn(0);

        if (obs[sub].source != null) // undefined != null -> false
        {
            dim.log("Pointing telescope to '"+obs[sub].source+"'.");
            dim.send("DRIVE_CONTROL/TRACK_ON", obs[sub].source);
        }
        else
        {
            dim.log("Pointing telescope to ra="+obs[sub].ra+" dec="+obs[sub].dec);
            dim.send("DRIVE_CONTROL/TRACK", obs[sub].ra, obs[sub].dec);
        }

        dim.wait("DRIVE_CONTROL", "OnTrack", 150000); // 110s for turning and 30s for stabilizing

        // Now tracking stable, switch voltage to nominal level and wait
        // for stability.
        service_feedback.voltageOn();
        service_feedback.waitForVoltageOn();

        if (!irq)
        {
            dim.log("Starting calibration.");

            // Calibration (2% of 20')
            while (!irq)
            {
                if (irq || !takeRun("pedestal",         1000))  // 80 Hz  -> 10s
                    continue;
                //if (irq || !takeRun("light-pulser-ext", 1000))  // 80 Hz  -> 10s
                //    continue;
                break;
            }

            var tm2 = new Date();

            dim.log("Starting ratescan.");

            //set reference to whole camera (in case it was changed)
            dim.send("RATE_SCAN/SET_REFERENCE_CAMERA");
            // Start rate scan
            dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 50, 1000, -10, "default");

            // Lets wait if the ratescan really starts... this might take a few
            // seconds because RATE_SCAN configures the ftm and is waiting for
            // it to be configured.
            dim.wait("RATE_SCAN", "InProgress", 10000);
            dim.wait("RATE_SCAN", "Connected", 2700000);

            // Here one could implement a watchdog for the feedback as well, but what is the difference
            // whether finally one has to find out if the feedback was in the correct state
            // or the ratescan was interrupted?

            // this line is actually some kind of hack.
            // after the Ratescan, no data is written to disk. I don't know why, but it happens all the time
            // So I decided to put this line here as a kind of patchwork....
            //dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

            dim.log("Ratescan done [%.1fs, %.1fs]".$((tm2-tm1)/1000, (new Date()-tm2)/1000));
        }

        dim.log("Task finished [RATESCAN]");
        console.out("");
        break; // case "RATESCAN"

    case "RATESCAN2":
        var tm1 = new Date();

        // This is a workaround to make sure that we really catch
        // the new OnTrack state later and not the old one
        dim.send("DRIVE_CONTROL/STOP");
        dim.wait("DRIVE_CONTROL", "Initialized", 15000);

        if (obs[sub].rstype=="dark-bias-off")
            service_feedback.voltageOff();
        else
        {
            // Switch the voltage to a reduced level (Ubd)
            var bias = dim.state("BIAS_CONTROL").name;
            if (bias=="VoltageOn" || bias=="Ramping")
                service_feedback.voltageOn(0);
        }

        // Open the lid if required
        if (!obs[sub].lidclosed)
            OpenLid();
        else
            CloseLid();

        // track source/position or move to position
        if (obs[sub].lidclosed)
        {
            dim.log("Moving telescope to zd="+obs[sub].zd+" az="+obs[sub].az);
            dim.send("DRIVE_CONTROL/MOVE_TO", obs[sub].zd, obs[sub].az);
            v8.sleep(3000);
            dim.wait("DRIVE_CONTROL", "Initialized", 150000); // 110s for turning and 30s for stabilizing
        }
        else
        {
            if (obs[sub].source != null)  // undefined != null -> false
            {
                dim.log("Pointing telescope to '"+obs[sub].source+"'.");
                dim.send("DRIVE_CONTROL/TRACK_ON", obs[sub].source);
            }
            else
            {
                dim.log("Pointing telescope to ra="+obs[sub].ra+" dec="+obs[sub].dec);
                dim.send("DRIVE_CONTROL/TRACK", obs[sub].ra, obs[sub].dec);
            }

            dim.wait("DRIVE_CONTROL", "OnTrack", 150000); // 110s for turning and 30s for stabilizing
        }

        // Now tracking stable, switch voltage to nominal level and wait
        // for stability.
        if (obs[sub].rstype!="dark-bias-off")
        {
            service_feedback.voltageOn();
            service_feedback.waitForVoltageOn();
        }

        if (!irq)
        {
            var tm2 = new Date();

            dim.log("Starting ratescan 2/1 ["+obs[sub].rstype+"]");

            //set reference to whole camera (in case it was changed)
            dim.send("RATE_SCAN/SET_REFERENCE_CAMERA");
            // Start rate scan
            dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 50, 300, 20, obs[sub].rstype);

            // Lets wait if the ratescan really starts... this might take a few
            // seconds because RATE_SCAN configures the ftm and is waiting for
            // it to be configured.
            dim.wait("RATE_SCAN", "InProgress", 10000);
            //FIXME: discuss what best value is here
            dim.wait("RATE_SCAN", "Connected", 2700000);//45min
            //dim.wait("RATE_SCAN", "Connected", 1200000);//3.3h

            // Here one could implement a watchdog for the feedback as well, but what is the difference
            // whether finally one has to find out if the feedback was in the correct state
            // or the ratescan was interrupted?

            // this line is actually some kind of hack.
            // after the Ratescan, no data is written to disk. I don't know why, but it happens all the time
            // So I decided to put this line here as a kind of patchwork....
            //dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

            dim.log("Ratescan 2/1 done [%.1fs, %.1fs]".$((tm2-tm1)/1000, (new Date()-tm2)/1000));
        }

        if (!irq)
        {
            var tm2 = new Date();

            dim.log("Starting ratescan 2/2 ["+obs[sub].rstype+"]");

            // Start rate scan
            dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 300, 1000, 100, obs[sub].rstype);

            // Lets wait if the ratescan really starts... this might take a few
            // seconds because RATE_SCAN configures the ftm and is waiting for
            // it to be configured.
            dim.wait("RATE_SCAN", "InProgress", 10000);
            dim.wait("RATE_SCAN", "Connected", 2700000);

            // Here one could implement a watchdog for the feedback as well, but what is the difference
            // whether finally one has to find out if the feedback was in the correct state
            // or the ratescan was interrupted?

            // this line is actually some kind of hack.
            // after the Ratescan, no data is written to disk. I don't know why, but it happens all the time
            // So I decided to put this line here as a kind of patchwork....
            //dim.send("FAD_CONTROL/SET_FILE_FORMAT", 6);

            dim.log("Ratescan 2/2 done [%.1fs, %.1fs]".$((tm2-tm1)/1000, (new Date()-tm2)/1000));
        }

        dim.log("Task finished [RATESCAN2]");
        console.out("");
        break; // case "RATESCAN2"

    case "CUSTOM":

        // This is a workaround to make sure that we really catch
        // the new OnTrack state later and not the old one
        dim.send("DRIVE_CONTROL/STOP");
        dim.wait("DRIVE_CONTROL", "Initialized", 15000);

        // Ramp bias if needed
        if (!obs[sub].biason)
            service_feedback.voltageOff();
        else
        {
            // Switch the voltage to a reduced level (Ubd)
            var bias = dim.state("BIAS_CONTROL").name;
            if (bias=="VoltageOn" || bias=="Ramping")
                service_feedback.voltageOn(0);
        }
        // Close lid
        CloseLid();

        // Move to position (zd/az)
        dim.log("Moving telescope to zd="+obs[sub].zd+" az="+obs[sub].az);
        dim.send("DRIVE_CONTROL/MOVE_TO", obs[sub].zd, obs[sub].az);
        v8.sleep(3000);
        dim.wait("DRIVE_CONTROL", "Initialized", 150000); // 110s for turning and 30s for stabilizing

        // Now tracking stable, switch voltage to nominal level and wait
        // for stability.
        if (obs[sub].biason)
        {
            service_feedback.voltageOn();
            service_feedback.waitForVoltageOn();
        }

        if (!irq)
        {
            dim.log("Taking custom run with time "+obs[sub].time+"s, threshold="+obs[sub].threshold+", biason="+obs[sub].biason);

            var customRun = function()
            {
                v8.sleep(500);//wait that configuration is set
                dim.wait("FTM_CONTROL", "TriggerOn", 15000);
                dim.send("FAD_CONTROL/SEND_SINGLE_TRIGGER");
                dim.send("RATE_CONTROL/STOP");
                dim.send("FTM_CONTROL/STOP_TRIGGER");
                dim.wait("FTM_CONTROL", "Valid", 3000);
                dim.send("FTM_CONTROL/ENABLE_TRIGGER", true);
                dim.send("FTM_CONTROL/SET_TIME_MARKER_DELAY", 123);
                dim.send("FTM_CONTROL/SET_THRESHOLD", -1, obs[sub].threshold);
                v8.sleep(500);//wait that configuration is set
                dim.send("FTM_CONTROL/START_TRIGGER");
                dim.wait("FTM_CONTROL", "TriggerOn", 15000);
            }

            takeRun("custom", -1, obs[sub].time, customRun);
        }
        dim.log("Task finished [CUSTOM].");
        dim.log("");
        break; // case "CUSTOM"

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
        //dim.log("DEBUG: remaining: "+remaining+" nextObs="+nextObs+" start="+nextObs.start);

        // ------------------------------------------------------------

        dim.log("Run count "+run+" [remaining "+parseInt(remaining)+"min]");

        // ----- Time since last DRS Calibration [min] ------
        var diff = getTimeSinceLastDrsCalib();

        // Changine pointing position and take calibration...
        //  ...every four runs (every ~20min)
        //  ...if at least ten minutes of observation time are left
        //  ...if this is the first run on the source
        var point  = (run%4==0 && remaining>10 && !obs[sub].orbit) || run==0; // undefined==null -> true!

        // Take DRS Calib...
        //  ...every four runs (every ~20min)
        //  ...at last  every two hours
        //  ...when DRS temperature has changed by more than 2deg (?)
        //  ...when more than 15min of observation are left
        //  ...no drs calibration was done yet
        var drscal = (run%4==0 && (remaining>15 && diff>70)) || diff==null;

        if (point)
        {
            // Switch the voltage to a reduced voltage level
            if (!obs[sub].grb)
                service_feedback.voltageOn(0);

            // Change wobble position every four runs,
            // start with alternating wobble positions each day
            var wobble = (parseInt(run/4) + parseInt(new Date()/1000/3600/24-0.5))%2+1;
            var angle  = obs[sub].angle == null ? Math.random()*360 : obs[sub].angle;

            if (obs[sub].orbit) // != undefined, != null, != 0
                dim.log("Pointing telescope to '"+obs[sub].source+"' [orbit="+obs[sub].orbit+"min, angle="+angle+"]");
            else
                dim.log("Pointing telescope to '"+obs[sub].source+"' [wobble="+wobble+"]");

            // This is a workaround to make sure that we really catch
            // the new OnTrack state later and not the old one
            dim.send("DRIVE_CONTROL/STOP");
            dim.wait("DRIVE_CONTROL", "Initialized", 15000);

            if (triggerReloadSources) // It's all synchronous - no need to wait here
            {
                dim.send("DRIVE_CONTROL/RELOAD_SOURCES");
                triggerReloadSources = false;
                dim.log("Reloading source table triggered in drivectrl.");
            }

            if (obs[sub].orbit) // != undefined, != null, != 0
                dim.send("DRIVE_CONTROL/TRACK_ORBIT", angle, obs[sub].orbit, obs[sub].source);
            else
                dim.send("DRIVE_CONTROL/TRACK_WOBBLE", wobble, obs[sub].source);

            // Do we have to check if the telescope is really moving?
            // We can cross-check the SOURCE service later
        }

        if (drscal && !obs[sub].nodrs)
        {
            doDrsCalibration("data");  // will turn voltage off
        }

        // Now we switch on the voltage and a significant amount of
        // time has been passed, so do the check again.
        sun = Sun.horizon(-12);
        if (!was_up && sun.isUp)
        {
            dim.log("Sun rise detected....");
            continue;
        }

        if (irq)
            continue;

        OpenLid(obs[sub].grb);

        // This is now the right time to wait for the drive to be stable
        if (!obs[sub].grb)
            dim.wait("DRIVE_CONTROL", "OnTrack", 150000); // 110s for turning and 30s for stabilizing
        else
            v8.timeout(150000, function() { if (dim.state("DRIVE_CONTROL").name=="Approaching" || dim.state("DRIVE_CONTROL").name=="Tracking" || dim.state("DRIVE_CONTROL").name=="OnTrack") return true; });

        // Now check the voltage... (do not start a lot of stuff just to do nothing)
        var state = dim.state("FEEDBACK").name;
        if (state=="Warning" || state=="Critical" || state=="OnStandby")
        {
            v8.sleep(60000);
            continue;
        }

        // Now we are 'OnTrack', so we can ramp to nominal voltage
        // and wait for the feedback to get stable
        //if (!obs[sub].grb || dim.state("FEEDBACK").name!="InProgress")
            service_feedback.voltageOn();
        if (!obs[sub].grb)
            service_feedback.waitForVoltageOn();

        // For the first run to be taken (ratecontrol)
        // Lid must be != Closed
        // Drive must be >= Moving
        // Setting the threshold can take 2-3 seconds

        if (obs[sub].grb && run==0)
        {
            // Instead of a pedestal run, we take a very short grb mode run
            // which is a classical data run but does not change the thresholds
            // This gives the voltages (typ. 5s) and currents (typ. 3s) time to
            // stabilize and threshold setting for the next run will be
            // reliable and fast but we have data already
            dim.log("Starting GRB mode.");

            // Whatever we do... it does not make sense to start with a closed lid
            // If the lid is not yet open, wait for the lid to be open and
            // go on as usual.
            if (dim.state("LID_CONTROL").name!="Open")
            {
                dim.log("Waiting for an open lid.");
                dim.wait("LID_CONTROL", "Open", 30000);
                service_feedback.waitForVoltageOn();
            }

            // Doing this manually makes it more flexible but turns off
            // automatic Critical mode checking of the feedback and
            // automatic reconnect
            var nextrun = sub_startrun.get().obj['next'];
            dim.log("Take run %3d".$(nextrun)+" [grb-mode]");

            dim.send("MCP/START", -1, -1, "grb"); // Use previous threshold settings if ratecontrol "InProgress"
            dim.wait("MCP", "TakingData", 15000);

            dim.wait("FEEDBACK",      "InProgress",  45000); // This is most likely the first to happen
            dim.wait("DRIVE_CONTROL", "OnTrack",    150000); // 110s for turning and 30s for stabilizing

            v8.sleep(1000); // By now we should have collected the required three current events in FEEDBACK
            // Current are averaged over 10s. So by now we should have a reasonable idea of the brightness of the sky
            // Individual pixels might still suffer wrong settings
            // Starting a new run takes less than 2s (for 60s runs this is 3%)
        }

        // If pointing had changed, do calibration
        if (!irq && point && !obs[sub].grb)
        {
            dim.log("Starting calibration.");

            // Calibration (2% of 20')
            while (!irq)
            {
                if (irq || !takeRun("pedestal",         1000))  // 80 Hz  -> 10s
                    continue;
//                if (irq || !takeRun("light-pulser-ext", 1000))  // 80 Hz  -> 10s
//                    continue;
                break;
            }
        }

        //console.out("  Taking data: start [5min]");

        // FIXME: What do we do if during calibration something has happened
        // e.g. drive went to ERROR? Maybe we have to check all states again?

        var twilight = Sun.horizon(-16).isUp;

        if (twilight || (obs[sub].grb && run<4))
        {
            for (var i=0; i<5 && !irq; i++)
                takeRun("data", -1, 60); // Take data (1min)
        }
        else
        {
            var len = 300;
            while (!irq && len>15)
            {
                var time = new Date();
                if (takeRun("data", -1, len)) // Take data (5min)
                    break;

                len -= parseInt((new Date()-time)/1000);
            }
        }

        //console.out("  Taking data: done");
        run++;

        continue; // case "DATA"
    }

    if (nextObs!=undefined && sub==obs.length-1)
        dim.log("Next observation will start at "+nextObs.start.toUTCString()+" [id="+nextObs.id+"]");

    sub++;
}

sub_drsruns.close();

dim.log("Left main loop [irq="+irq+"]");

// ================================================================
// Comments and ToDo goes here
// ================================================================

// error handline : http://www.sitepoint.com/exceptional-exception-handling-in-javascript/
// classes: http://www.phpied.com/3-ways-to-define-a-javascript-class/
