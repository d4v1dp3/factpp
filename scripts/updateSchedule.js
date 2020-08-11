'use strict';

//this is just the class implementation of 'Observation'
include('scripts/Observation_class.js');

var file = $['file'] ? $['file'] : 'scripts/schedule.js';

console.out('Reading schedule from '+file);

// this file just contains the definition of
// the variable observations, which builds our nightly schedule, hence the filename
include(file);

// -------------------------------------------------------------------------------------

// Get current time
var now = new Date(); //new Date("2013-04-07 19:00:00 UTC");

// Because Main.js could start a new observations just in the moment between 'now'
// and entering the new data in the database, we have to use the unique id
// in Main.js to check if the current observation should be changed (and sub resetted)
now = new Date(now.getTime());

//console.out("","NOW: "+now.toUTCString());

// -------------------------------------------------------------------------------------

console.out("Processing "+observations.length+" entries.");

var has_now = false;

// make Observation objects from user input and check if 'date' is increasing.
for (var i=0; i<observations.length; i++)
{
    if (observations[i].date.toUpperCase()=="NOW")
    {
        if (has_now)
            throw new Error("Only one entry 'now' allowed");

        has_now = true;
    }

    observations[i] = new Observation(observations[i]);

    // check if the start date given by the user is increasing.
    if (i>0 && observations[i].start <= observations[i-1].start)
    {
        throw new Error("Start time '"+ observations[i].start.toUTCString()+
                        "' in row "+i+" exceeds start time in row "+(i-1)+" "+observations[i-1].start.toUTCString() );
    }
}

// Remove all past entries from the schedule
while (observations.length>0 && observations[0].start<now)
    observations.shift();

// Output for debugging
/*
console.out("");
for (var i=0; i<observations.length; i++)
{
    for (var j=0; j<observations[i].length; j++)
        console.out("%3d.%3d: ".$(i,j)+JSON.stringify(observations[i][j]));
    console.out("");
}*/

// -------------------------------------------------------------------------------------

// Connect to database
var db = new Database("scheduler:5ch3du13r@www.fact-project.org/factdata");

// get all sources from database
var sources = db.query("SELECT * from Source");

// Convert SourceName to SourceKey
function getSourceKey(src)
{
    var arr = sources.filter(function(e) { return e['fSourceName']==src; });
    if (arr.length==0)
        throw new Error("Source '"+src+"' unknown.");
    if (arr.length>1)
        throw new Error("More than one source '"+src+"' found.");
    return arr[0]['fSourceKEY'];
}

// -------------------------------------------------------------------------------------

// List of all available measurement types
var measurementType = [ "STARTUP", "IDLE", "DRSCALIB", "SINGLEPE", "DATA", "RATESCAN", "SHUTDOWN", "OVTEST", "RATESCAN2", "SLEEP", "CUSTOM" ];

// Convert measurement type to index
function getMeasurementTypeKey(task)
{
    var idx = measurementType.indexOf(task);
    if (idx>=0)
        return idx;

    throw new Error("Task "+task+" not supported!");
}

// -------------------------------------------------------------------------------------

var queries = [ ];

var system_on = false;

// Now create the schedule which should be entered into the databse
for (var i=0; i<observations.length; i++)
{
    var obs = observations[i];

    for (var j=0; j<obs.length; j++)
    {
        console.out(i+" "+j+" "+obs[j].start.toUTCString());
        var isUp = Sun.horizon(-12, obs[j].start).isUp;

        if (obs[j].task=="STARTUP" && j>0)
            throw new Error("STARTUP must be the first measurement in a list of measurements.");
        if (obs[j].task=="DATA" && j!=obs.length-1)
            throw new Error("DATA must be the last task in a list of measurements");
        if (obs[j].task=="SHUTDOWN" && j!=obs.length-1)
            throw new Error("SHUTDOWN must be the last task in a list of measurements");
        if (system_on && obs[j].task=="SHUTDOWN" && isUp)
            throw new Error("SHUTDOWN must not be scheduled after -12deg (~10min before nautical sun-rise)");
        // FIXME: Check also end-time!
        if ((obs[j].task=="DATA" || obs[j].task=="RATESCAN" ||  obs[j].task=="RATESCAN2" ) && isUp)
            throw new Error("Data or Ratescan must not be scheduled when the sun is up (-12deg, earlist/lastest during astronomical twilight)");

        if (obs[j].task=="STARTUP" || obs[j].task=="DATA")
            system_on = true;

        var str = "INSERT INTO Schedule SET";
        str += " fStart='"+obs[j].start.toISOString()+"'";
        str += ",fMeasurementID="+obs[j].sub;
        str += ",fMeasurementTypeKey="+getMeasurementTypeKey(obs[j].task);

        // Currently only data in the case when a source is given or ra/dec is given can be provided
        if (obs[j].source)
            str += ",fSourceKey="+getSourceKey(obs[j].source);

        //lidclosed only  needs to be inserted to DB if 'false'
        if (obs[j].lidclosed)
        {
            if (obs[j].task=="RATESCAN2" && obs[j].rstype!="default")
                str += ",fData='\"lidclosed\":"+obs[j].lidclosed+",\"rstype\":\""+obs[j].rstype+"\",\"zd\":"+obs[j].zd+",\"az\":"+obs[j].az+"'";
            else
                str += ",fData='\"lidclosed\":"+obs[j].lidclosed+",\"zd\":"+obs[j].zd+",\"az\":"+obs[j].az+"'";
        }
        else
            if (obs[j].ra)
                str += ",fData='\"ra\":"+obs[j].ra+",\"dec\":"+obs[j].dec+"'";

        if (obs[j].task=="CUSTOM")
            str += ",fData='\"biason\":"+obs[j].biason+",\"time\":\""+obs[j].time+"\",\"threshold\":\""+obs[j].threshold+"\",\"zd\":"+obs[j].zd+",\"az\":"+obs[j].az+"'";

        queries.push(str);
    }
}

if (queries.length==0)
{
    console.out("","Nothing to do... no observation past "+now.toUTCString(), "");
    exit();
}

// Output and send all queries, update the databse
//console.out("");

db.query("LOCK TABLES Schedule WRITE");
db.query("DELETE FROM Schedule WHERE fStart>='"+now.toISOString()+"'");
for (var i=0; i<queries.length; i++)
    db.query(queries[i]);
db.query("UNLOCK TABLES");

//console.out("");

// ======================================================================================

// Because Main.js could start a new observations just in the moment between 'now'
// and entering the new data in the database, we have to use the unique id
// in Main.js to check if the current observation should be changed (and sub resetted)
var start = new Date(now.getTime());//-12*3600000);

//console.out("","START: "+now.toUTCString());

// Get the current schedule
var rows = db.query("SELECT * FROM Schedule WHERE fStart>='"+start.toISOString()+"' ORDER BY fStart, fMeasurementID");

var schedule = [];
var entry    = -1;
var sub      =  0;

for (var i=0; i<rows.length; i++)
{
    //console.out(JSON.stringify(rows[i]));

    var start = new Date(rows[i]['fStart']+" UTC");
    var id    = rows[i]['fScheduleID'];
    var src   = rows[i]['fSourceKey'];
    var task  = rows[i]['fMeasurementTypeKey'];
    var sub   = rows[i]['fMeasurementID'];
    var data  = rows[i]['fData'];

    if (sub==0)
        entry++;

    var m = { }
    m.task = measurementType[task];

    if (src)
    {
        // Convert SourceKey to SourceName
        var arr = sources.filter(function(e) { return e['fSourceKEY']==src; });
        if (arr.length==0)
            throw new Error("SourceKey "+src+" unknown.");

        m.source = arr[0]['fSourceName'];
    }

    if (data)
    {
        var obj = JSON.parse(("{"+data+"}").replace(/\ /g, "").replace(/(\w+):/gi, "\"$1\":"));
        for (var key in obj)
            m[key] = obj[key];
    }

    if (!schedule[entry])
        schedule[entry] = { };

    schedule[entry].id   = id;
    schedule[entry].date = start;

    if (!schedule[entry].measurements)
        schedule[entry].measurements = [];

    schedule[entry].measurements[sub] = m;
}

// -------------------------------------------------------------------------------------

console.out("[");
for (var i=0; i<schedule.length; i++)
{
    var obs = schedule[i];

    console.out(' { date:"'+obs.date.toISOString()+'" measurements:');
    var obs = obs.measurements;

    console.out("     [");
    for (var j=0; j<obs.length; j++)
    {
        console.out("      "+JSON.stringify(obs[j])+",");
    }
    console.out("     ]");
    console.out(" },");
}
console.out("]");
