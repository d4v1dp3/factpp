'use strict';

function pad(n)
{
    return (n<10) ? '0' + n : n;
}

function getYMD(d)
{
    return d.getUTCFullYear()+'-'+pad(d.getUTCMonth()+1)+'-'+pad(d.getUTCDate());
}

function updateClock()
{
    var now = new Date();
    var s = getYMD(now)+' '+pad(now.getUTCHours())+':'+pad(now.getUTCMinutes())+' UTC';
    $('#clock').html(s);

    var table = document.getElementById("TableHolder");

    if (!table.isTonight)
        return;

    var rows = table.childNodes;

    for (var i=2; i<rows.length; i++)
    {
        var el = rows[i].firstChild.firstChild;

        // FIXME: replace by classes?
        if (!el.valueAsDate)
        {
//            el.setAttribute("style", "color:#000000");
            continue;
        }

        var t0 = now.getTime()%86400000;
        var t1 = el.valueAsDate%86400000;
        if (t1<43200000)
            t1 += 86400000;

        el.setAttribute("style", t1>t0 ? "color:darkgreen" : "color:darkred");
    }
};

function addEmptyRow(prev, start)
{
    var empty =
    {
        fStart: "0000/00/00 "+(start?start:""),
        fMeasurementTypeKey:0,
        fMeasurementID:0,
        fSourceKEY:0,
    };

    addRow(empty, false, prev);
}

function debug(txt)
{
    var dbg = document.getElementById("debug");
    dbg.appendChild(document.createTextNode(txt));
    dbg.appendChild(document.createElement("br"));
}

function addRow(row, disabled, prev)
{
    var table = document.getElementById("TableHolder");

    var sources = table.sources;
    var measurements = table.measurements;

    var tr = document.createElement("tr");
    tr.setAttribute("width", "100%");

    if (!prev)
        table.appendChild(tr);
    else
        table.insertBefore(tr, prev.nextSibling);

    // ---------- column 1 ----------

    var input1t = document.createElement("input");
    input1t.setAttribute("type","time");
    input1t.setAttribute("size","10");
    input1t.setAttribute("autofocus","true");
    if (row.fMeasurementID!=0)
        input1t.setAttribute("hidden", "true");
    input1t.setAttribute("value", row.fStart.substr(11));
    if (disabled)
        input1t.setAttribute("disabled", "true");

    /*
    input1t.onblur = function()
    {
        var prevRow = tr.previousSibling;
        while (prevRow && prevRow.firstChild.firstChild.hidden)
            prevRow = prevRow.previousSibling;

        var nextRow = tr.nextSibling;
        while (nextRow && nextRow.firstChild.firstChild.hidden)
            nextRow = nextRow.nextSibling;

        var prevEl = prevRow ? prevRow.firstChild : undefined;
        var nextEl = nextRow ? nextRow.firstChild : undefined;

        var prevTime = "";
        if (prevEl && prevEl.firstChild.constructor.name=="HTMLInputElement")
            prevTime = prevEl.firstChild.value;

        var nextTime = "";
        if (nextEl && nextEl.firstChild.constructor.name=="HTMLInputElement")
            nextTime = nextEl.firstChild.value;

        alert(prevTime+"/"+input1t.value+"/"+nextTime);
    }*/

    var td1 = document.createElement("td");
    td1.setAttribute("style","white-space:nowrap;padding-left:4px;padding-right:2px;");
    td1.setAttribute("align","center");
    td1.appendChild(input1t);

    // Check if this is the transition from disabled to enabled.
    // In this case enable all previous [+]
    if (tr.previousSibling && tr.previousSibling.firstChild.firstChild.disabled && !disabled)
    {
        var prevRow = tr.previousSibling;

        var tm = prevRow.firstChild.firstChild.value;
        while (1)
        {
            prevRow.firstChild.childNodes[1].removeAttribute("disabled");
            prevRow = prevRow.previousSibling;
            if (prevRow.firstChild.firstChild.value!=tm)
                break;
        }
    }

    var input1p = document.createElement("input");
    input1p.setAttribute("style","width:20px");
    input1p.setAttribute("type","button");
    input1p.setAttribute("value","+");
    if (row.fMeasurementID!=0)
        input1p.setAttribute("hidden", "true");
    // FIXME: Enable if this is the last "+" in tonight
    if (disabled && ! (table.isTonight && row.last))
        input1p.setAttribute("disabled", "true");
    input1p.onclick = function()
    {
        // FiXME: Do not allow deleting of last line
        var nextRow = tr;
        while (nextRow.nextSibling)
        {
            if (!nextRow.nextSibling.firstChild.firstChild.hidden)
                break;

            nextRow = nextRow.nextSibling;
        }

        addEmptyRow(nextRow, input1t.value);
    }
    td1.appendChild(input1p);

    var input1m = document.createElement("input");
    input1m.setAttribute("style","width:20px");
    input1m.setAttribute("type","button");
    input1m.setAttribute("value","-");
    if (row.fMeasurementID!=0)
        input1m.setAttribute("hidden", "true");
    if (disabled)
        input1m.setAttribute("disabled", "true");
    input1m.setAttribute("data-toggle", "tooltip");
    if (row.fScheduleID)
        input1m.setAttribute("title", row.fScheduleID+" ["+row.fLastUpdate+"]");
    input1m.onclick = function()
    {
        //if (table.childNodes.length==3)
        //    return;

        var nextRow = tr.nextSibling;
        table.removeChild(tr);

        while (nextRow)
        {
            var inp = nextRow.firstChild.firstChild;
            if (!inp.hidden)
                break;

            var xx = nextRow;
            nextRow = nextRow.nextSibling;
            table.removeChild(xx);
        }

        if (table.childNodes.length==2)
            addEmptyRow();
    }
    td1.appendChild(input1m);

    tr.appendChild(td1);

    // ---------- column 2 -----------

    var select2 = document.createElement("select");
    // select2.setAttribute("style", "width:100px");
    select2.setAttribute("class", "measurement");
    if (disabled)
        select2.setAttribute("disabled", "true");

    for (var i=0; i<measurements.length; i++)
    {
        var option = document.createElement("option");
        select2.appendChild(option);
        option.setAttribute('value', measurements[i].key);
        option.appendChild(document.createTextNode(measurements[i].val));
        if (row.fMeasurementTypeKey==measurements[i].key)
            select2.selectedIndex = i;
    }

    var td2 = document.createElement("td");
    td2.setAttribute("style","white-space:nowrap;padding-left:2px;padding-right:2px;");
    td2.setAttribute("align","center");
    td2.appendChild(select2);

    var input2 = document.createElement("input");
    input2.setAttribute("type","button");
    input2.setAttribute("value","+");
    input2.setAttribute("style","width:20px");
    if (disabled)
        input2.setAttribute("disabled", "true");
    input2.onclick = function()
    {
        var empty =
        {
            fStart: row.fStart,
            fMeasurementTypeKey:0,
            fMeasurementID:-1,
            fSourceKEY:0,
        };

        addRow(empty, false, tr);
    }
    td2.appendChild(input2);

    input2 = document.createElement("input");
    input2.setAttribute("type","button");
    input2.setAttribute("style","width:20px");
    input2.setAttribute("value","-");
    if (disabled)
        input2.setAttribute("disabled", "true");
    input2.onclick = function()
    {
        //if (table.childNodes.length==3)
        //    return;

        if (!tr.firstChild.childNodes[0].hidden)
        {
            var tm = tr.firstChild.childNodes[0].value;

            var nextRow = tr.nextSibling;
            if (nextRow)
            {
                var e = nextRow.firstChild.childNodes;
                e[0].removeAttribute("hidden");
                e[1].removeAttribute("hidden");
                e[2].removeAttribute("hidden");
                e[0].value = tm;
            }
        }
        table.removeChild(tr);

        if (table.childNodes.length==2)
            addEmptyRow();
    }
    td2.appendChild(input2);

    tr.appendChild(td2);

    // ---------- column 3 -----------

    var select3 = document.createElement("select");
    //select3.setAttribute("style", "width:100px");
    select3.setAttribute("class", "sources");
    if (disabled)
        select3.setAttribute("disabled", "true");

    for (var i=0; i<sources.length; i++)
    {
        var option = document.createElement("option");
        select3.appendChild(option);
        option.setAttribute('value', sources[i].key);
        option.appendChild(document.createTextNode(sources[i].val));
        if (row.fSourceKey==sources[i].key)
            select3.selectedIndex = i;
    }

    var td3 = document.createElement("td");
    td3.setAttribute("style","white-space:nowrap;padding-left:2px;padding-right:2px;");
    td3.setAttribute("align","center");
    td3.appendChild(select3);

    tr.appendChild(td3);

    // ---------- column 4 ------------

    var input4 = document.createElement("input");
    input4.setAttribute("type","text");
    input4.setAttribute("style","width:100%");
    input4.setAttribute("placeholder","JSON object");
    if (row.fData)
        input4.setAttribute("value",row.fData);
    if (disabled)
        input4.setAttribute("disabled","true");

    var td4 = document.createElement("td");
    td4.setAttribute("style", "padding-left:2px;padding-right:4px;");
    td4.setAttribute("align","center");
    td4.appendChild(input4);

    tr.appendChild(td4);
}

function setupCalendar(result, date)
{
    debug("cal="+date);

    date = date.replace('-','');
    date = date.replace('-','');

    var dates = { };

    for (var i=0; i<result.length; i++)
    {
        result[i].d = result[i].d.replace('-','');
        result[i].d = result[i].d.replace('-','');

        dates[result[i].d] = { klass: "highlight", tooltip: "Schedule set." };
    }

    dates[date] = { klass: "selected", tooltip: "Currently selected." };

    function getDateInfo(date, wantsClassName)
    {
        var as_number = Calendar.dateToInt(date);
        return dates[as_number];
    };

    var cont = document.getElementById("cont");
    while (cont.firstChild)
        cont.removeChild(cont.firstChild);

    var setup =
    {
        cont: "cont",
        selectionType: Calendar.SEL_MULTIPLE,
        bottomBar: false,
        date:parseInt(date),
        dateInfo:getDateInfo
    };

    debug("cal.date="+date);

    var cal = Calendar.setup(setup); 
    cal.addEventListener("onSelect", function(){ loadDay(this.selection.print("%Y-%m-%d")); });
}


function onDataReceived(result)
{
    var table = document.getElementById("TableHolder");
    if (!table.currentDay)
        return;

    // Split the results of the different queries
    // They are separated by newlines
    var data = result.split('\\n');
    if (data.length<5)
    {
        alert("Malformed result returned["+data.length+"]:\n"+data[data.length-1]);
        return;
    }

    debug("table.currentDay="+table.currentDay);

    // Decode the results into variables

    // Does that work in all browsers, or do we need "YYYY-MM-DDTHH:MM:SSZ" ?
    //data[0] = data[0].replace('-', '/');
    //data[0] = data[0].replace('-', '/');

    // year, month, day, hours, minutes, seconds, milliseconds
    var tonight      = new String(table.currentDay);
    var day          = new Date(table.currentDay);
    var dates        = JSON.parse(data[0]);
    var sources      = JSON.parse(data[1]);
    var measurements = JSON.parse(data[2]);
    var schedule     = JSON.parse(data[3]);

    if (data[4])
        alert(data[4]);

    var ld = document.getElementById("loaddate");
    ld.setAttribute("size","10");

    ld.value = table.prevDay;

    // First update the calender
    setupCalendar(dates, tonight);

    // Add a fake source to the list of sources to allow 'deselection' of source
    sources.splice(0, 0, { key: 0, val: "---" });

    table.sources = sources;
    table.measurements = measurements;

    // Enable or disable the SAVE and LoadPrev button
    var save  = document.getElementById("save");
    var load  = document.getElementById("load");
    var ldate = document.getElementById("loaddate");

    if (day.getTime()+36*3600*1000>table.loadTime)
    {
        save.removeAttribute("disabled");
        load.removeAttribute("disabled");
        ldate.removeAttribute("disabled");


        // If this is a dayin the future, but no schedule is in the db,
        // create an empty one
        if (schedule.length==0)
            addEmptyRow();
    }
    else
    {
        save.setAttribute("disabled", "true");
        load.setAttribute("disabled", "true");
        ldate.setAttribute("disabled", "true");
    }

    // Update the header of the date/time column
    var tm = document.getElementById("time");
    while (tm.firstChild)
        tm.removeChild(tm.firstChild);

    var nxt = new Date(day.getTime()+24*3600*1000);

    var d1 = pad(day.getUTCMonth()+1)+'-'+pad(day.getUTCDate());
    var d2 = pad(nxt.getUTCMonth()+1)+'-'+pad(nxt.getUTCDate());

    tm.appendChild(document.createTextNode(d1+ " / "+d2));

    var offset = new Date(table.loadedDay).getTime();

    // other day loaded and tonight

    if (!table.isTonight || !table.loadedDay)
        table.cutTime = "12:00:00";

    // Now loop over all rows and add them one by one to the table
    for (var i=0; i<schedule.length; i++)
    {
        schedule[i].last   = schedule[schedule.length-1].fStart==schedule[i].fStart;
        schedule[i].fStart = schedule[i].fStart.replace('-', '/');
        schedule[i].fStart = schedule[i].fStart.replace('-', '/');

        var stamp = new Date(schedule[i].fStart+" UTC");

        if (table.loadedDay)
            stamp = new Date(stamp.getTime()+day.getTime()-offset);

        var disabled = stamp.getTime()<table.loadTime && !table.loadedDay;
        if (disabled)
            table.cutTime = pad(stamp.getUTCHours())+":"+pad(stamp.getUTCMinutes())+":"+pad(stamp.getUTCSeconds());

        addRow(schedule[i], disabled);
    }

    debug("currentDay="+table.currentDay);
    debug("nextDay="+table.nextDay);
    debug("isTonight="+table.isTonight);
    debug("cutTime="+table.cutTime);
}

function loadDay(date, dateToLoad)
{
    // Clean elements from table before new elements are added
    var table = document.getElementById("TableHolder");

    // In very rare cases (fast and frequent clicks on a date with a long schedule)
    // the event listened of the calender returns a wrong value
    if (new String(date).length!=10)
        return;

    var dbg = document.getElementById("debug");
    while (dbg.firstChild)
        dbg.removeChild(dbg.firstChild);

    debug("loadDay="+date+"|"+dateToLoad);

    var count = 0;

    var cut;
    while (table.childNodes.length>2)
    {
        var cols = table.lastChild.childNodes;
        var time = cols[0].firstChild;

        if (time.disabled)
        {
            debug("disabled="+time.value);

            if (dateToLoad)
            {
                cut = time.value;
                break;
            }
        }

        count++;
        table.removeChild(table.lastChild);
    }

    debug(count+" lines removed ");

    document.getElementById("savedate").value = date;

    // it helps to know if this is tonight or not
    var now   = new Date();
    var day   = new Date(date);
    var night = getYMD(new Date(now.getTime()-12*3600*1000));

    table.isTonight  = date==night;
    table.currentDay = date;
    table.loadedDay  = dateToLoad;
    table.loadTime   = now.getTime();

    if (!table.isTonight || !table.loadedDay)
        table.cutTime = undefined;

    // remember the currently displayed day (FIXME: Move to table property?)
    table.prevDay = getYMD(new Date(day.getTime()-24*3600*1000));
    table.nextDay = getYMD(new Date(day.getTime()+24*3600*1000));

    debug("day="+date+"|"+date.length);
    debug("dayToLoad="+table.loadedDay);
    debug("cut="+cut);

    var data = "n="+(dateToLoad?dateToLoad:date);
    if (cut && (!table.isTonight || table.loadedDay))
        data += "&t="+cut;

    debug("data="+data);

    // request data from the datanbase and on reception, display the data
    $.ajax({
        type:    "POST",
        cache:   false,
        url:     "load.php",
        data:    data,
        success: onDataReceived,
        error:   function(xhr) { if (xhr.status==0) alert("ERROR[0] - Request failed!"); else alert("ERROR[0] - "+xhr.statusText+" ["+xhr.status+"]"); }
    });
}

function onReady()
{
     if (location.href.search('debug')==-1)
        $("#debug").hide();

    /*---------------------------------------------------------------------------------------------
     Initialize jQuery datapicker (type='date' not supported by firefox)
     ----------------------------------------------------------------------------------------------*/

     if ($('input').prop('type') != 'date' ) 
     {
        var date_opt =
        {
            dateFormat: 'yy-mm-dd',
            showButtonPanel: true,
            showOtherMonths: true,
            selectOtherMonths: true,
            autoSize: true,
        }

        $('[type="date"]').datepicker(date_opt);
     }
     
    /*---------------------------------------------------------------------------------------------
     Initialize jQuery tooltips
     ----------------------------------------------------------------------------------------------*/

     //$(document).ready(function(){$('[data-toggle="tooltip"]').tooltip();});
     $('[data-toggle="tooltip"]').tooltip();

    /*---------------------------------------------------------------------------------------------
     Prevent any user interaction during AJAX requests
     ----------------------------------------------------------------------------------------------*/

    $(document).ajaxStart(function() { $('#wait').fadeIn(); }).ajaxStop(function() { $('#wait').fadeOut(200); });

    /*---------------------------------------------------------------------------------------------
     Check if a dedicated day was requested, if not start with the current day.
     Load the data from the database and display the calendar and the data.
     ----------------------------------------------------------------------------------------------*/

    var reg = /^\?day=(20[123][0-9]-[01][0-9]-[0123][0-9])/;
    var res = reg.exec(location.search);

    var day;
    if (!res)
    {
        var now = new Date();

        if (now.getUTCHours()<12)
            now = new Date(now.getTime()-12*3600*1000);

        day = getYMD(now);
    }
    else
        day = res[1];

    loadDay(day);

    /*---------------------------------------------------------------------------------------------
     Start updating the clock
     ----------------------------------------------------------------------------------------------*/

    updateClock();
    setInterval(updateClock, 1000);

    /*---------------------------------------------------------------------------------------------
     Loading of previous data. Get the previous date with existing data through PreviousData.php
     Same Shedule.ph passed to load on the controls with extra parameter 'prev' to indicate that
     data is from previous schedule.
     ----------------------------------------------------------------------------------------------*/

    // FIXME: Do not overwrite the disabled part of the schedule if it is TONIGHT!!!!

    function onLoad()
    {
        var cd = document.getElementById("TableHolder");
        var dt = document.getElementById("loaddate");
        loadDay(cd.currentDay, dt.value);
    }

    $('#load').click(onLoad);
    $('#loaddate').keypress(function(event) { if (event.which==13) { onLoad(); } event.preventDefault(); });

    /*---------------------------------------------------------------------------------------------
     Savng and updating of schedule. Data array is generated from the current table to be submitted
     to saveSchedule.php for the execution of queries.
     ----------------------------------------------------------------------------------------------*/

    function onSaveClick()
    {
        var table = document.getElementById("TableHolder");

        var rows = table.childNodes;

        var schedule = [];

        // cutTime is the last time of a measurement which is past
        // and should not be updated. This assumes that all
        // time values are sequential.
        //var cutTime  = "12:00:00";
        //var prevTime = new Date(currentDay+" 12:00:00");

        // FIXME: Make sure dates are sequentiel

        for (var i=2; i<rows.length; i++)
        {
            var cols = rows[i].childNodes;

            var time     = cols[0].firstChild.value;
            var measure  = cols[1].firstChild.value;
            var source   = cols[2].firstChild.value;
            var value    = cols[3].firstChild.value;
            var hidden   = cols[0].firstChild.hidden;
            var disabled = cols[0].firstChild.disabled;

            if (!hidden && !time && rows.length!=3)
            {
                alert("ERROR - Invalid time fields detected.");
                return;
            }

            /*
            if (!hidden)
            {
                // This is just to check the times... theoretically,
                // these times could be sent, so that the php does not
                // have to do that again, or should the php check things
                // to ensure that it cannot be hacked?
                var t = time;
                t = t.replace(':','');
                t = t.replace(':','');

                t = new Date(t<120000 ? currentDay+" "+time : nextDay+" "+time);

                if (t.getTime()<prevTime.getTime())
                {
                    alert("Times not sequential... cannot save schedule.");
                    return;
                }

                prevTime = t;
            }

            if (disabled)
            {
                // if (cutTime>time) // no time yet!
                cutTime = time;
                continue;
            }*/

            if (disabled)
                continue;

            if (hidden)
                time = null;

            schedule.push([ time, measure, source, value ]);
        }

        if (schedule.length==0)
        {
            alert("No active tasks - nothing to be saved.");
            return;
        }

        //alert(table.isTonight+"/"+table.cutTime+"/"+schedule.length);

        var data = "n="+table.currentDay+"&d="+JSON.stringify(schedule);
        if (table.isTonight)
            data += "&t="+table.cutTime;

        $.ajax({
           type:    "POST",
           cache:   false,
           url:     "save.php",
           data:    data,
           success: function(result) { if (result.length==0) { /*alert("Success.");*/ loadDay(table.currentDay); } else alert("ERROR - "+result); },
           error:   function(xhr) { if (xhr.status==0) alert("ERROR[1] - Unauthorized!"); else alert("ERROR[1] - "+xhr.statusText+" ["+xhr.status+"]"); }
        });
    }

    $('#save').click(onSaveClick);


    function onHelp()
    {
        var ov  = $("#Overlay");
        var pos = $("#help").offset();
        //var doc = $(document);
        ov.css({
           left:   pos.left + 'px',
           top:    pos.top + 'px',
           width:  0,
           height: 0
        })
        .show()
        .animate({
           left:   0,
           top:    0,
           width:  '100%',
           height: '100%'
        }, "slow");

        event.preventDefault();
    }

    function onClose()
    {
        $("#Overlay").hide("slow");
    }

    $('#help').click(onHelp);
    $(document).keydown(function(event) { if (event.which==27) { onClose(); event.preventDefault(); } });
    $('#close').click(onClose);

}

$('document').ready(onReady);
