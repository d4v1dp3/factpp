// ======================================================================================

function getSchedule()
{
    // Get current time
    var start = new Date();//new Date("2013-04-07 19:00:00 UTC");

    // Because Main.js could start a new observations just in the moment between 'now'
    // and entering the new data in the database, we have to use the unique id
    // in Main.js to check if the current observation should be changed (and sub resetted)
    start = new Date(start.getTime()-10*3600000);

    // ----------------------------------------------------------------------

    // Connect to database
    var db = new Database($['schedule-database']);

    // Get the current schedule
    var rows = db.query("SELECT * FROM Schedule "+
                        "LEFT JOIN MeasurementType USING (fMeasurementTypeKey) "+
                        "LEFT JOIN Source USING (fSourceKey) "+
                        "WHERE fStart>'"+start.toISOString()+"' "+
                        "ORDER BY fStart ASC, fMeasurementID ASC");

    // Close db connection
    db.close();

    // ----------------------------------------------------------------------

    var schedule = [];
    var entry    = -1;
    var sub      =  0;

    for (var i=0; i<rows.length; i++)
    {
        var id  = rows[i]['fScheduleID'];
        var sub = rows[i]['fMeasurementID'];
        if (sub==0)
            entry++;

        var m = { }

        m.task = rows[i]['fMeasurementTypeName'];
        if (!m.task)
            throw new Error("No valid measurement type for id=("+id+":"+sub+")");

        // For simplicity, measurements suspend and resume must be unique in an observation
        if ((m.task=="suspend" || m.task=="resume") && sub>0)
            throw new Error("Measurement "+m.task+" not the only one in the observation (id="+id+")");

        m.source = rows[i]['fSourceName'];

        var data = rows[i]['fData'];
        if (data)
        {
            var obj = JSON.parse(("{"+data+"}").replace(/\ /g, "").replace(/(\w+):/gi, "\"$1\":"));
            for (var key in obj)
                m[key] = obj[key];
        }

        if (!schedule[entry])
            schedule[entry] = { };

        schedule[entry].id   = id;
        schedule[entry].date = new Date(rows[i]['fStart']+" UTC");

        if (!schedule[entry].measurements)
            schedule[entry].measurements = [];

        schedule[entry].measurements[sub] = m;
    }

    for (var i=0; i<schedule.length; i++)
        schedule[i] = new Observation(schedule[i]);

    return schedule;
}

// -------------------------------------------------------------------------------------

/*
 // remove "
 conv = conv.replace(/\"(\w+)\":/ig, "$1:");
 // must contain one , less than :
 // must not contain " and '
 //var test = "{ra:12,dec:13}".replace(/\ /g, "").replace(/(\w+):/gi, "\"$1\":");
*/
