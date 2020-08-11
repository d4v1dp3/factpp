'use strict';

//
// this file contains just the implementation of the
// Observation class (I know there are no classes in javascript...)
//

function Observation(obj)
{
    if (typeof(obj)!='object')
        throw new Error("Observation object can only be constructed using an object.");

    if (!obj.date)
        throw new Error("Observation object must have a 'date' parameter");

    var ret = [];

    // FIXME: Check transisiton from summer- and winter-time!!
    var utc = obj.date.toString().toUpperCase()=="NOW" ? new Date() : new Date(obj.date);
    if (isNaN(utc.valueOf()))
        throw new Error('"'+obj.date+'" not a valid Date... try something like "2013-01-08 23:05 UTC".');

    ret.start = utc;
    ret.id    = obj.id;

    // If the given data is not an array, make it the first entry of an array
    // so that we can simply loop over all entries
    if (obj.measurements.length===undefined)
    {
        var cpy = obj.measurements;
        obj.measurements = [];
        obj.measurements[0] = cpy;
    }

    for (var i=0; i<obj.measurements.length; i++)
    {
        var obs = obj.measurements[i];

        ret[i] = { };
        ret[i].task   = obs.task ? obs.task.toUpperCase() : "DATA";
        ret[i].source = obs.source;
        ret[i].ra     = parseFloat(obs.ra);
        ret[i].dec    = parseFloat(obs.dec);
        ret[i].zd     = parseFloat(obs.zd);
        ret[i].az     = parseFloat(obs.az);
        ret[i].orbit  = parseFloat(obs.orbit);
        ret[i].angle  = parseFloat(obs.angle);
        ret[i].time   = parseInt(obs.time);
        ret[i].threshold = parseInt(obs.threshold);
        ret[i].lidclosed = obs.lidclosed;
        ret[i].biason = obs.biason;
        ret[i].rstype = obs.rstype ? obs.rstype : "default";
        ret[i].sub    = i;
        ret[i].start  = utc;
        ret[i].nodrs  = obs.nodrs;
        ret[i].grb    = obs.grb;


        ret[i].toString = function()
        {
            var rc = this.task;
            rc += "["+this.sub+"]";
            if (this.source)
                rc += ": " + this.source;
            //rc += " ["+this.start.toUTCString()+"]";
            return rc;
        }

        switch (ret[i].task)
        {
        case 'DATA':
            if (i!=obj.measurements.length-1)
                throw new Error("Measurement DATA [n="+i+", "+utc.toUTCString()+"] must be the last in the list of measurements [cnt="+obj.measurements.length+"]");
            if (ret[i].source == undefined)
                throw new Error("Measurement DATA must have a source defined");
            // This is obsolete. We cannot check everything which is not evaluated anyways
            //if (ret[i].lidclosed == true)
            //    throw new Error("Observation must not have 'lidclosed'==true " +
            //                    "if 'task'=='data'");
            break;

        case 'SUSPEND':
        case 'RESUME':
            if (obj.measurements.length!=1)
                throw new Error("Measurement "+ret[i].task+" [n="+i+", "+utc.toUTCString()+"] must be the only measurements [cnt="+obj.measurements.length+"] in an observation");
            break;

        case 'STARTUP':
        case 'SHUTDOWN':
            if (ret[i].source != undefined)
                console.out("WARNING - Measurement "+ret[i].task+" has a source defined");
            break;

        case 'RATESCAN':
            if (ret[i].source == undefined && (isNaN(ret[i].ra) || isNaN(ret[i].dec)))
                throw new Error("Measurement RATESCAN must have either a source or 'ra' & 'dec' defined");
            // This is obsolete. We cannot check everything which is not evaluated anyways
            //if (ret[i].lidclosed == true)
            //    throw new Error("Observation RATESCAN must not have 'lidclosed'==true");
            break;

        case 'RATESCAN2':
            if ((ret[i].lidclosed != true) && ret[i].source == undefined && (isNaN(ret[i].ra) || isNaN(ret[i].dec)))
                throw new Error("Measurement RATESCAN2 ('lidclosed'==false or undefined) must have either a source or 'ra' & 'dec' defined");
            if (ret[i].lidclosed == true && (isNaN(ret[i].az) || isNaN(ret[i].az)))
                throw new Error("Measurement RATESCAN2 ('lidclosed'==true) must have 'zd' & 'az' defined");
            break;

        case 'CUSTOM':
            if (isNaN(ret[i].az) || isNaN(ret[i].az) || isNaN(ret[i].time) || isNaN(ret[i].threshold))
                throw new Error("Measurement CUSTOM must have 'zd' & 'az', 'time' and 'threshold' defined.");
            break;

        case 'SINGLEPE':
        case 'OVTEST':
        case 'DRSCALIB':
        case 'IDLE':
        case 'SLEEP':
            break;

        default:
            throw new Error("The measurement type "+ret[i].task+" is unknown.");
        }
    }

    return ret;
}
