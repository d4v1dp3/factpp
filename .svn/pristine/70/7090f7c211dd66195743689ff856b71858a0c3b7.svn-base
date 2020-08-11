voltageOff = function()
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

waitForVoltageOn = function()
{
    // Avoid output if condition is already fulfilled
    dim.log("Waiting for voltage to be stable.");

    function func()
    {
        if (this.ok==true)
            return true;
    }

    var now = new Date();

    this.last = undefined;
    this.ok = false;
    v8.timeout(4*60000, func, this); // FIMXE: Remove 4!
    this.ok = undefined;

    dim.log("Voltage On(?)");

    //if (irq)
        //dim.log("Waiting for stable voltage interrupted.");
    //else
        //dim.log("Voltage stable within limits");
}


voltageOn = function(ov)
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

 dim.log("STARTING SCRIPT closed_lid_ratescan");

// dim.log("Sending Voltage On");
// voltageOn();
 v8.sleep(10000);

     var tm = new Date();

     dim.log("Starting ratescan.");

     // Start rate scan
     //dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 200, 900, 20);
     dim.send("RATE_SCAN/START_THRESHOLD_SCAN", 100, 900, 10);

      // PAUSE
     dim.log("PAUSE 10s");
     v8.sleep(10000);
     dim.log("Start Ratescan");

     // Lets wait if the ratescan really starts... this might take a few
     dim.wait("RATE_SCAN", "InProgress", 10000);
     //dim.wait("RATE_SCAN", "Connected", 2700000);
     dim.wait("RATE_SCAN", "Connected", 3500000);

     dim.log("Ratescan done");


// voltageOff();

 dim.log("Task finished [RATESCAN]");
 console.out("");

