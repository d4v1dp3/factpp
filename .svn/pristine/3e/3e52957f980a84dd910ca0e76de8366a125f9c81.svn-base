'use strict';

// To de done:
//  - CheckLID status (should be open or closed)
//  - Is it necessary to switch the bias-voltage off?
//  - Get reasonable timeouts for all steps (wait, get, run)
//  - Improve order to accelerate execution
//
// =================================================================

/*
var table =
[
 [ "AGILENT_CONTROL" ],
 [ "BIAS_CONTROL"    ],
 [ "CHAT"            ],
 [ "DATA_LOGGER"     ],
 [ "DRIVE_CONTROL"   ],
 [ "FEEDBACK"        ],
 [ "FAD_CONTROL"     ],
 [ "FSC_CONTROL"     ],
 [ "FTM_CONTROL"     ],
 [ "LID_CONTROL"     ],
 [ "MAGIC_WEATHER"   ],
 [ "MCP"             ],
 [ "PWR_CONTROL"     ],
 [ "RATE_CONTROL"    ],
 [ "RATE_SCAN"       ],
 [ "SMART_FACT"      ],
 [ "TIME_CHECK"      ],
 [ "TNG_WEATHER"     ],
];

if (dim.state("DRIVE_CONTROL").name=="Locked")
{
    throw new Error("Drivectrl still locked... needs UNLOCK first.");
    //while (!dim.send("DRIVE_CONTROL"))
    //    v8.sleep();
    //dim.send("DRIVE_CONTROL/UNLOCK");
    //dim.wait("DRIVE_CONTROL", "Armed", 1000);
}

*/

console.out("");
dim.alarm();

var loop;
include("scripts/Handler.js");
include("scripts/CheckStates.js");

// -----------------------------------------------------------------
// Make sure camera electronics is switched on and has power
// -----------------------------------------------------------------

include("scripts/handleAgilentPowerOn24V.js");
include("scripts/handleAgilentPowerOn50V.js");
include("scripts/handleAgilentPowerOn80V.js");
include("scripts/handlePwrCameraOn.js");

checkSend(["AGILENT_CONTROL_24V","AGILENT_CONTROL_50V","AGILENT_CONTROL_80V","PWR_CONTROL"]);

loop = new Handler("PowerOn");
//loop.add(handleAgilentPowerOn24V);
//loop.add(handleAgilentPowerOn50V);
//loop.add(handleAgilentPowerOn80V);
loop.add(handlePwrCameraOn);
loop.run(30000);
console.out("");

// If power was switched on: wait for a few seconds

// -----------------------------------------------------------------
// Now take care that the bias control, the ftm and the fsc are
// properly connected and are in a reasonable state (e.g. the
// trigger is switched off)
// -----------------------------------------------------------------

include("scripts/handleBiasVoltageOff.js");
include("scripts/handleFtmIdle.js");
include("scripts/handleFscConnected.js");
include("scripts/handleFeedbackConnected.js");
include("scripts/handleRatectrlConnected.js");
include("scripts/handleLidClosed.js");
include("scripts/handleFadConnected.js");
include("scripts/handleScheduler.js");

checkSend(["BIAS_CONTROL","FAD_CONTROL","FTM_CONTROL", "FSC_CONTROL", "FEEDBACK", "RATE_CONTROL", "MCP", "SCHEDULER"]);

dim.send("MCP/RESET");

loop = new Handler("SystemSetup");
loop.add(handleBiasVoltageOff);
loop.add(handleFtmIdle);
loop.add(handleFscConnected);
loop.add(handleFadConnected);
loop.add(handleFeedbackConnected); // Feedback needs FAD to be Connected
loop.add(handleRatectrlConnected);
loop.add(handleLidClosed);
loop.add(handleScheduler);
loop.run(60000);

console.out("biasctrl:    "+dim.state("BIAS_CONTROL").name);
console.out("ftmctrl:     "+dim.state("FTM_CONTROL").name);
console.out("fscctrl:     "+dim.state("FSC_CONTROL").name);
console.out("feedback:    "+dim.state("FEEDBACK").name);
console.out("ratecontrol: "+dim.state("RATE_CONTROL").name);
console.out("fadctrl:     "+dim.state("FAD_CONTROL").name);
console.out("mcp:         "+dim.state("MCP").name);
console.out("scheduler:   "+dim.state("SCHEDULER").name);
console.out("");

console.out("Enable all FTU");
dim.send("FTM_CONTROL/ENABLE_FTU", -1, true);

// -----------------------------------------------------------------
// Now we check the FTU connection
// -----------------------------------------------------------------

/*
include("scripts/handleFtuCheck.js");

loop = new Handler("FtuCheck");
loop.ftuList = new Subscription("FTM_CONTROL/FTU_LIST");
loop.add(handleFtuCheck);
loop.run();
loop.ftuList.close();

dim.log("All FTUs are enabled and without error.");
*/

console.out("Checking FTU: start");
include("scripts/CheckFTU.js");
console.out("Checking FTU: done");
console.out("");

// -----------------------------------------------------------------
// Now we check the clock conditioner
// -----------------------------------------------------------------

var sub_counter = new Subscription("FTM_CONTROL/COUNTER");
var counter = sub_counter.get(3000, false).counter;
dim.send("FTM_CONTROL/REQUEST_STATIC_DATA");
v8.timeout(3000, function() { if (sub_counter.get(0, false).counter>counter) return true; });
if (sub_counter.get(0, false).qos&0x100==0)
    throw new Error("Clock conditioner not locked.");
sub_counter.close();

// -----------------------------------------------------------------
// Now we can safely try to connect the FAD boards.
// -----------------------------------------------------------------
/*
 include("scripts/handleFadConnected.js");

// If FADs already connected

checkSend(["FAD_CONTROL"]);

loop = new Handler("ConnectFad");
loop.add(handleFadConnected);
loop.run();

var failed = false;
dim.onchange["FAD_CONTROL"] = function(arg)
{
    if (this.rc && arg.name!="Connected")
        failed = true;
}

console.out("FADs connected.");
console.out("");

console.out(dim.state("FAD_CONTROL").name);
console.out(dim.state("MCP").name);
*/

// ================================================================
// Underflow check
// ================================================================
// Is it necessary to check for the so called 'underflow-problem'?
// (This is necessary after each power cycle)
// ----------------------------------------------------------------

include('scripts/CheckUnderflow.js');

// Now it is time to check the connection of the FADs
// it might hav thrown an exception already anyway


// ================================================================
// Power on drive system if power is off (do it hre to make sure not
// everything is switchd on at the same time)
// ================================================================

//console.out("PWR: "+(dim.state("PWR_CONTROL").index&16));

if ((dim.state("PWR_CONTROL").index&16)==0)
{
    console.out("Drive cabinet not powered... Switching on.");
    dim.send("PWR_CONTROL/TOGGLE_DRIVE");
    v8.timeout(5000, function() { if (dim.state("PWR_CONTROL").index&16) return true; });
}

include("scripts/handleDriveArmed.js");

checkSend(["DRIVE_CONTROL"]);

loop = new Handler("ArmDrive");
loop.add(handleDriveArmed);
loop.run(30000);


// ================================================================
// Bias crate calibration
// ================================================================
// Bias crate calibration if necessary (it is aftr 4pm (local tome)
// and the last calibration was more than eight hours ago.
// -----------------------------------------------------------------

// At this point we know that:
//  1) The lid is closed
//  2) The feedback is stopped
//  3) The voltage is off
function makeCurrentCalibration()
{
    dim.send("BIAS_CONTROL/SET_ZERO_VOLTAGE");
    dim.wait("BIAS_CONTROL", "VoltageOff", 30000); // waS: 15000

    var now = new Date();
    dim.send("FEEDBACK/CALIBRATE");

    console.out("Wait for calibration to start");
    dim.wait("FEEDBACK", "Calibrating", 5000);

    console.out("Wait for calibration to end");
    dim.wait("FEEDBACK", "Calibrated", 90000);

    console.out("Calibration finished ["+(new Date()-now)+"ms]");

    console.out("Wait for voltage to be off");
    dim.wait("BIAS_CONTROL", "VoltageOff", 30000); // was: 15000
}

// Check age of calibration
var service_calibration = new Subscription("FEEDBACK/CALIBRATION");

var data_calibration = service_calibration.get(3000, false);

var age = data_calibration.time;
var now = new Date();

var diff = (now-age)/3600000;

var fb_state = dim.state("FEEDBACK").index;

// !data_calibration.data: FEEDBACK might just be freshly
// started and will not yet serve this service.
if (fb_state<5 || (diff>8 && now.getHours()>16))
{
    if (fb_state<5)
        console.out("No BIAS crate calibration available: New calibration needed.");
    else
        console.out("Last BIAS crate calibration taken at "+age.toUTCString()+": New calibration needed.");

    makeCurrentCalibration();
}

service_calibration.close();

// ================================================================
// Setup GPS control and wait for the satellites to be locked
// ================================================================

checkSend(["GPS_CONTROL"]);

if (dim.state("GPS_CONTROL").name=="Disconnected")
    dim.send("GPS_CONTROL/RECONNECT");

// Wait for being connectes
v8.timeout(5000, function() { if (dim.state("GPS_CONTROL").name!="Disconnected") return true; });

// Wait for status available
v8.timeout(5000, function() { if (dim.state("GPS_CONTROL").name!="Connected") return true; });

if (dim.state("GPS_CONTROL").name=="Disabled")
    dim.send("GPS_CONTROL/ENABLE");

// Wait for gps to be enabled and locked
dim.wait("GPS_CONTROL", "Locked", 15000);

// ================================================================
// Crosscheck all states
// ================================================================

// FIXME: Check if there is a startup scheduled, if not do not force
// drive to be switched on

var table =
[
 [ "GTC_DUST"      ],
 [ "TNG_WEATHER"   ],
 [ "MAGIC_WEATHER" ],
 [ "RAIN_SENSOR"   ],
 [ "CHAT"          ],
 [ "SMART_FACT"    ],
 [ "TEMPERATURE"   ],
 [ "SCHEDULER",           [ "Connected", "Armed" ] ],
 [ "EVENT_SERVER",        [ "Running", "Standby" ] ],
 [ "DATA_LOGGER",         [ "NightlyFileOpen", "WaitForRun", "Logging" ] ],
 [ "FSC_CONTROL",         [ "Connected"                       ] ],
 [ "MCP",                 [ "Idle"                            ] ],
 [ "TIME_CHECK",          [ "Valid"                           ] ],
 [ "PWR_CONTROL",         [ "SystemOn"                        ] ],
 [ "AGILENT_CONTROL_24V", [ "VoltageOn"                       ] ],
 [ "AGILENT_CONTROL_50V", [ "VoltageOn"                       ] ],
 [ "AGILENT_CONTROL_80V", [ "VoltageOn", "Disconnected"       ] ], //hack to allow for data taking while agilent not available
 [ "BIAS_CONTROL",        [ "VoltageOff"                      ] ],
 [ "FEEDBACK",            [ "Calibrated"                      ] ],
 [ "RATE_SCAN",           [ "Connected"                       ] ],
 [ "RATE_CONTROL",        [ "Connected"                       ] ],
 [ "DRIVE_CONTROL",       [ "Initialized", "Tracking", "OnTrack", "Locked" ] ],
 [ "LID_CONTROL",         [ "Open", "Closed"                  ] ],
 [ "FTM_CONTROL",         [ "Valid", "TriggerOn"              ] ],
 [ "FAD_CONTROL",         [ "Connected", "WritingData"        ] ],
 [ "GPS_CONTROL",         [ "Locked" ] ],
// [ "SQM_CONTROL",         [ "Valid" ] ],
 [ "PFMINI_CONTROL",      [ "Receiving" ] ],
 [ "BIAS_TEMP",           [ "Valid" ] ],
 [ "GUDE_CONTROL",        [ "Valid" ] ],
];



if (!checkStates(table))
{
    throw new Error("Something unexpected has happened. Although the startup-"+
                    "procedure has finished, not all servers are in the state "+
                    "in which they ought to be. Please, try to find out what "+
                    "happened...");
}
