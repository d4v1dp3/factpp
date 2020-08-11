'use strict';

/**
 *
 *  Waits for the timeout (in ms) for the given servers to be online
 *  and in one of the provided states.
 *
 *  Returns true if MAGIC_WEATHER is online, FTM_CONTROL is in Idle
 *  and LID_CONTROL is either in Open or Closed.
 *
 *  Returns false if all given servers are online, but at least one is
 *  not in the expected state.
 *
 *  Throws an exception if not all provided servers are online after
 *  the given timeout.
 *
 *  If you want to wait infinitely: CheckStates(table, null)
 *  If the timeout is negative (e.g. -500 means 500ms) or zero,
 *     no exception is thrown but false is returned.
 *
 * @param table
 *
 * @param {Integer} [timeout=5000]
 *    timeout in milliseconds
 *
 * @returns
 *
 * @throws
 *
 * @example
 *    var table =
 *    [
 *        [ "MAGIC_WEATHER"  ],
 *        [ "FTM_CONTROL",  [ "Idle" ] ],
 *        [ "LID_CONTROL",  [ "Open", "Closed" ] ],
 *    ];
 *
 *    checkStates(table);
 *
 *
 */
function checkStates(table, timeout, wait)
{
    if (timeout===undefined)
        timeout = 5000;

    var states = [];

    var time = new Date();
    while (1)
    {
        // Get states of all servers in question
        states = [];
        for (var i=0; i<table.length; i++)
        {
            var state = dim.state(table[i][0]);
            states[i] = state ? state.name : undefined;
        }

        // Check if they are all valid
        if (states.indexOf(undefined)<0)
        {
            // If they are all valid, check them against the
            // state lists provided for each server
            var rc = true;
            for (var i=0; i<table.length; i++)
            {
                if (!table[i][1] || table[i][1].indexOf(states[i])>=0)
                    continue;

                if (!wait)
                    dim.log(table[i][0]+" in ["+states[i]+"] not as it ought to be ["+table[i][1]+"]");

                rc = false;
            }
            if (rc)
                return rc;

            if (!wait)
                return false;
        }

        if ((new Date())-time>=Math.abs(timeout))
            break;

        v8.sleep();
    }

    if (timeout<0)
        return false;

    // Create a list of all servers which do not have valid states yet
    var servers = [];
    for (var i=0; i<table.length; i++)
        if (!states[i])
            servers.push(table[i][0]);

    // If all servers do not yet have valid states, it is obsolete to
    // print all their names
    if (servers.length==table.length && servers.length>1)
        servers = [ "servers." ];

    throw new Error("Timeout waiting for access to named states of "+servers.join(", "));
}

function checkSend(servers, timeout)
{
    if (timeout===undefined)
        timeout = 5000;

    var states = [];

    var time = new Date();
    while (1)
    {
        // Get states of all servers in question
        states = [];
        for (var i=0; i<servers.length; i++)
            states[i] = dim.send(servers[i]);

        // Check if they are all valid
        if (states.indexOf(false)<0)
            return true;

        if ((new Date())-time>=Math.abs(timeout))
            break;

        v8.sleep();
    }

    if (timeout<0)
        return false;

    // Create a list of all servers which do not have valid states yet
    var missing = [];
    for (var i=0; i<servers.length; i++)
        if (!states[i])
            missing.push(servers[i]);

    throw new Error("Timeout waiting for send-ready of "+missing.join(", "));
}

function Wait(server,states,timeout1,timeout2)
{
    if (typeof(states)=="string")
        states = [ states ];

    // If timeout2 is defined and >0 wait first for the
    // server to come online. If it does not come online
    // in time, an exception is thrown.
    if (timout2>0 && CheckStates([ server ], timeout2))
        return true;

    var time = new Date();
    while (1)
    {
        // If a server disconnects now while checking for the
        // states, an exception will be thrown, too.
        if (CheckStates([ server, states ], 0))
            return true;

        if (Date()-time>=abs(timeout1))
            break;

        v8.sleep();
    }

    if (timeout1<0)
        throw new Error("Timeout waiting for Server "+server+" to be in ["+states+"]");

    return false;
}

// Wait 5s for the FAD_CONTROL to get to the states
//   return false if FAD_CONTROL is not online
//   return false if state is not
// Wait("FAD_CONTROL", [ "COnnected", "Disconnected" ], 5000);

function dimwait(server, state, timeout)
{
    if (!timeout)
        timeout = 5000;

    var time = new Date();
    while (1)
    {
        var s = dim.state(server);
        if (s.index===state || s.name===state)
            return true;

        //if (s.index==undefined)
        //    throw "Server "+server+" not connected waiting for "+state+".";

        if (Date()-time>=timeout)
            break;

        v8.sleep();
    }

    if (timeout>0)
        throw new Error("Timeout waiting for ["+states+"] of "+server+".");

    return false;


    /*
    if (!timeout)
        timeout = 5000;

    var time = new Date();
    while (timeout<0 || new Date()-time<timeout)
    {
        var s = dim.state(server);
        if (s.index===state || s.name===state)
            return true;

        if (s.index==undefined)
            throw "Server "+server+" not connected waiting for "+state+".";

        v8.sleep();
    }

    return false;
*/
}

function Sleep(timeout)
{
    if (!timeout)
        timeout = 5000;

    var time = new Date();
    while (Date()-time<timeout)
        v8.sleep();
}

function Timer()
{
    this.date = new Date();

    this.reset = function() { this.date = new Date(); }
    this.print = function(id)
    {
        var diff = Date()-this.date;
        if (id)
            console.out("Time["+id+"]: "+diff+"ms");
        else
            console.out("Time: "+diff+"ms");
    }
}

function WaitStates(server, states, timeout, func)
{
    var save = dim.onchange[server];

    function inner()
    {
        dim.onchange[server] = function(arg)
        {
            if (!this.states)
                this.states = states instanceof Array ? states : [ states ];

            var comp = this.states[0];
            if (arg.index===comp || arg.name===comp || comp=='*')
                this.states.shift();

            //console.out(JSON.stringify(arg), this.states, comp, arg.name, "");

            if (save instanceof Function)
                save();

            if (this.states.length==0)
                delete dim.onchange[server];

        }

        if (func instanceof Function)
            func();

        var time = new Date();
        while (1)
        {
            if (!dim.onchange[server])
                return true;

            if (new Date()-time>=timeout)
                break;

            v8.sleep();
        }

        delete dim.onchange[server];

        if (timeout>0)
            throw new Error("Timeout waiting for ["+states+"] of "+server+".");

        return false;
    }

    var rc = inner();
    dim.onchang
        e[server] = save;
    return rc;
}
