'use strict';

// Get fad control connected
function handleFadConnected(wait_state)
{
    var state = dim.state("FAD_CONTROL");
    if (state===undefined)
        return undefined;

    if (wait_state && wait_state.length>0 && state.name!=wait_state)
        return wait_state;

    //dim.log("BIAS_CONTROL: "+state.name+"["+state.index+"]");

    switch (state.name)
    {
    case "Offline":
        return undefined;

    case "Disconnected":
        console.out("Fadctrl in 'Disconnected'... sending START... waiting for 'Connected'.");
        dim.send("FAD_CONTROL/START");
        return "Connected";

    // Do-nothing conditions
    case "Connecting":
        return wait_state;

    // Do-something conditions
    case "Configuring1":
    case "Configuring2":
    case "Configuring3":
    case "Configured":
        console.out("Fadctrl in Configure state... sending RESET_CONFIGURE... waiting for 'Connected'.");
        dim.send("FAD_CONTROL/RESET_CONFIGURE");
        return "Connected";

    case "Disengaged":
        console.out("Fadctrl in 'Disengaged'... sending START... waiting for 'Connected'.");
        dim.send("FAD_CONTROL/START");
        return "Connected";

    case "RunInProgress":
        console.out("Fadctrl in 'RunInProgress'... sending CLOSE_OPEN_FILES... waiting for 'Connected'.");
        dim.send("FAD_CONTROL/CLOSE_OPEN_FILES");
        return "Connected";

    // Final state reached condition
    case "Connected":
        var sub_con = new Subscription("FAD_CONTROL/CONNECTIONS");
        var con = sub_con.get(5000);
        var all = true;
        for (var i=0; i<40; i++)
            if (con.obj['status'][i]&66!=66)
            {
                console.out("Board "+i+" not connected... sending CONNECT... waiting for 'Connected'.");
                dim.send("FAD_CONTROL/CONNECT", i);
                all = false;
            }
        sub_con.close();
        return all ? "" : "Connected";
    }

    throw new Error("FAD_CONTROL:"+state.name+"["+state.index+"] unknown or not handled.");
}
