'use strict';

// ==========================================================================

var camera_type = "FACT";

var scale    = camera_type=="FACT" ?      83 :  22;
var npix     = camera_type=="FACT" ?    1440 :  64;
var diameter = camera_type=="FACT" ?  0.1111 : 1.5;
var center   = camera_type=="FACT" ? [0.5,0] : [0,0]

function onRightMouseClick(event)
{
    if (event.button!=2)
        return;

    var strData = event.target.toDataURL("image/png");

    var img = document.createElement("img");
    img.src = strData;

    $(img).css({ "z-index": "9999", "position": "absolute" });
    $(img).insertBefore($(event.target));

    setTimeout(function () { $(img).remove(); }, 100);
}

(function ($)
 {

    $.plot.plugins.push({
        init: function(plot, classes)
            {
                plot.hooks.bindEvents.push(function(plot, eventHolder) { eventHolder.mousedown(onRightMouseClick); });
                plot.hooks.shutdown.push(function(plot, eventHolder) { eventHolder.unbind("mousedown", onRightMouseClick); });
            },
        name: 'saveAsImage',
        version: '1.0'
    });

 })(jQuery);

// ==========================================================================

var editor1;
var editor2;
var plot;

function debug(txt)
{
    var dbg = document.getElementById("debug");
    dbg.appendChild(document.createTextNode(txt));
    dbg.appendChild(document.createElement("br"));
}

function setupAccordion(accordion, container, inactive)
{
    function onAccordion(event, ui)
    {
        if (ui.oldHeader.length==0)
            $(container).slideDown(400);
        else
            $(container).slideUp(400);
    }

    var config = { collapsible: true };

    if (inactive)
    {
        config.active = false;
        $(container).hide();
    }

    var acc = $(accordion);

    acc.accordion(config);
    acc.on("accordionbeforeactivate", onAccordion);
}

function onResize(event, ui)
{
    if (!ui.size)
        return;

    $(event.target.id).css({marginRight:'2px'});

    var editor = event.target.id=="textcontainer1" ? editor1 : editor2;

    editor.setSize("100%", ui.size.height);
    editor.refresh();
}

function setSize(id, w, h)
{
    w = parseInt(w);
    h = parseInt(h);

    $("#"+id).width(w);
    $("#"+id).height(h);

    document.getElementById(id).width=w;
    document.getElementById(id).height=h;
}

function onResizeGrid(id)
{
    var w = document.getElementById(id+"container").clientWidth/4;

    var offy = 0;
    var offx = 5;

    var cont = document.getElementById("center"+id).childNodes[0];

    var nn;
    if (cont)
    {
        nn = parseInt(cont.id[cont.id.length-1]);
        setSize(id+nn, w*2, w*2);
    }

    if (nn!=1)
        setSize(id+'1', w-offx, w-offy);
    if (nn!=2)
        setSize(id+'2', w-offx, w-offy);
    if (nn!=3)
        setSize(id+'3', w-offx, w-offy);
    if (nn!=4)
        setSize(id+'4', w-offx, w-offy);

    document.getElementById("center"+id).width=parseInt(w*2);

    setSize('cont'+id+'1', w, w);
    setSize('cont'+id+'2', w, w);
    setSize('cont'+id+'3', w, w);
    setSize('cont'+id+'4', w, w);
}

function onResizeCameras(event, ui)
{
    onResizeGrid('camera');

    drawFullCam("camera1");
    drawFullCam("camera2");
    drawFullCam("camera3");
    drawFullCam("camera4");
}

function onResizeHistograms(event, ui)
{
    onResizeGrid('hist');
}

function createEditor(textarea)
{
    var editor;

    var config =
    {
        //value: "function myScript(){return 100;}\n",
        mode:  { name: "text/typescript", globalVars: true },
        indentUnit: 4,
        styleActiveLine: true,
        matchBrackets: true,
        lineNumbers: true,
        foldGutter: true,
        lint: true,
        highlightSelectionMatches: {showToken: /\w/},
        gutters: ["CodeMirror-lint-markers", "CodeMirror-linenumbers", "CodeMirror-foldgutter"],
        extraKeys: {
            //"Ctrl-D": "duplicateLine",
            //"Alt--": "goToBracket",
            //"Ctrl-H": "findPrev",
            "Ctrl-Down": "autocomplete",
            "Tab": "indentAuto",
            "Ctrl-Y": "deleteLine",
            "Ctrl-.": function(cm) {
                cm.foldCode(cm.getCursor());
            },
            "F11": function(cm) {
                editor.setOption("fullScreen", !editor.getOption("fullScreen"));
            },
            "Ctrl-R": function(cm) {
                editor.execCommand("replace");
            },
            "Esc": function(cm) {
                if (editor.getOption("fullScreen")) editor.setOption("fullScreen", false);
            },
            "Enter": function(cm) {
                editor.execCommand("indentAuto");
                editor.execCommand("newlineAndIndent");
            },
        }
    };

    editor = CodeMirror.fromTextArea(document.getElementById(textarea), config);
    editor.setOption("theme", "fact");

    return editor;
}

function colorizeHTML(textarea)
{
    var config =
    {
        //value: "function myScript(){return 100;}\n",
        mode:  { name: "text/typescript", globalVars: true },
        readOnly: true,
    };

    CodeMirror.fromTextArea(document.getElementById(textarea), config);
}

function setFileChecks(file)
{
    var list = document.getElementById("file").data;
    if (!list)
        return;

    if (!file)
        file = document.getElementById("file").value;

    var drs  = document.getElementById("drsfile");

    var hasDrs = list[file]&2;
    var hasCal = list[file]&4;
    var isMC   = list[file]&8;

    if (!hasDrs || isMC)
        $('#drsfile').prop('checked', false);
    if (!hasCal || isMC)
        $('#calibrated').prop('checked', false);

    $('#calibrated').prop('disabled', (!hasCal && !drs.checked) || isMC);
    $('#drsfile').prop('disabled',     !hasDrs || isMC);

    $('#txtcalibrated').css('color', (!hasCal && !drs.checked) || isMC ? 'darkgrey' : 'black');
    $('#txtdrsfile').css(   'color',  !hasDrs || isMC                  ? 'darkgrey' : 'black');
    $('#txtmontecarlo').css('color',  !isMC                            ? 'darkgrey' : 'black');

    $('#montecarlo').prop('checked', isMC);
}

function disableControls(disabled)
{
    $('#submit').prop('disabled', disabled);
    $('#getcamera').prop('disabled', disabled);
    $('#getwaveforms').prop('disabled', disabled);
    $('#event').prop('disabled', disabled);
    $('#pixel').prop('disabled', disabled);
    $('#cbpx').prop('disabled', disabled);
    $('#cbpx-c').prop('disabled', disabled);
    $('#cbpx-b').prop('disabled', disabled);
    $('#cbpx-p').prop('disabled', disabled);
    $('#cbpx-x').prop('disabled', disabled);
    $('#file').prop('disabled', disabled);

    if (disabled)
    {
        $('#calibrated').prop('disabled', true);
        $('#drsfile').prop('disabled', true);
        $('#montecarlo').prop('disabled', true);
    }
    else
    {
        $(document.getElementById("file").value ? '#event' : '#file').focus();
        setFileChecks();
    }
}

function onGetCameras()
{
    var arr =
    [
     document.getElementById("camera1").dataAbs,
     document.getElementById("camera2").dataAbs,
     document.getElementById("camera3").dataAbs,
     document.getElementById("camera4").dataAbs,
     ];

    $('#controls > input[name=data]').val(JSON.stringify(arr));
    $('#controls > input[name=name]').val('cameras.txt');
    $('#controls').attr('action','index.php');
    $('#controls').submit();
}

function onGetWaveforms()
{
    var arr = document.getElementById("waveform").data;

    $('#controls > input[name=data]').val(JSON.stringify(arr));
    $('#controls > input[name=name]').val('waveforms.txt');
    $('#controls').attr('action','index.php');
    $('#controls').submit();
}

function onGetGeometry()
{
    var sqrt32 = Math.sqrt(3)/2;
    
    var arr = [ new Array(npix), new Array(npix) ];

    for (var i=0; i<npix; i++)
    {
        arr[0][i] = coord[i][0]*diameter;
        arr[1][i] = coord[i][1]*diameter*sqrt32;
    }

    $('#controls > input[name=data]').val(JSON.stringify(arr));
    $('#controls > input[name=name]').val('geometry.txt');
    $('#controls').attr('action','index.php');
    $('#controls').submit();
}

function onReady()
{
    CodeMirror.colorize(null, 'javascript');

    //$('input,select').keypress(function(event) { return event.keyCode != 13; });

    //colorizeHTML("code0");
    //colorizeHTML("code1");

    $("#accordion").accordion({collapsible:true,active:false,heightStyle:'content'});
    $("#accordion").find('h3').filter(':contains(Runtime)').hide();
    if (location.href.search('debug')==-1)
        $("#accordion").find('h3').filter(':contains(Debug)').hide();

    $("#textcontainer1").resizable({handles:"s",autoHide:true,});
    $("#textcontainer1").on("resize", onResize);

    $("#textcontainer2").resizable({handles:"s",autoHide:true,});
    $("#textcontainer2").on("resize", onResize);

    $("#cameracontainer").on("resize", onResizeCameras);
    onResizeCameras();

    $("#histcontainer").on("resize", onResizeHistograms);
    onResizeHistograms();

    $("#contcamera1").click(onClickContCamera);
    $("#contcamera2").click(onClickContCamera);
    $("#contcamera3").click(onClickContCamera);
    $("#contcamera4").click(onClickContCamera);

    $("#conthist1").click(onClickContHist);
    $("#conthist2").click(onClickContHist);
    $("#conthist3").click(onClickContHist);
    $("#conthist4").click(onClickContHist);

    $("#camera1").click(onClick);
    $("#camera2").click(onClick);
    $("#camera3").click(onClick);
    $("#camera4").click(onClick);

    $('#camera1').mousedown(onRightMouseClick);
    $('#camera2').mousedown(onRightMouseClick);
    $('#camera3').mousedown(onRightMouseClick);
    $('#camera4').mousedown(onRightMouseClick);

    editor1 = createEditor("editor1");
    editor2 = createEditor("editor2");

    setupAccordion('#accordion5', '#editorcontainer1', true);
    setupAccordion('#accordion1', '#editorcontainer2');

    $('#accordion5').on("accordionactivate", function() { $('#editorcontainer1fake').hide(); editor1.refresh();  });
    $('#accordion1').on("accordionactivate", function() { $('#editorcontainer2fake').hide(); editor2.refresh();  });

    setupAccordion('#accordion2', '#cameracontainer');
    setupAccordion('#accordion7', '#histcontainer', true);
    setupAccordion('#accordion3', '#waveformcontainer');
    setupAccordion('#accordion4', '#helpcontainer', true);
    setupAccordion('#accordion6', '#ctrlcontainer', true);

    $("#selectfile1").on('change', onFile);
    $("#selectfile2").on('change', onFile);

    $(document).ajaxStart(function() { disableControls(true) }).ajaxStop(function() { disableControls(false); });

    $.ajax({
        type:    "POST",
        cache:   false,
        url:     "index.php",
        success: onFilelistReceived,
        error:   function(xhr) { if (xhr.status==0) alert("ERROR[0] - Request failed!"); else alert("ERROR[0] - "+xhr.statusText+" ["+xhr.status+"]"); }
    });
}

function onFileSelect(event, ui)
{
    setFileChecks(ui.item.value);
    document.getElementById("event").value = 0;
    onSubmit(ui.item.value);
}

function onFilelistReceived(result)
{
    if (result.indexOf("<?php")>=0)
    {
        alert("Call to 'index.php' returned its contents.\n"+
              "It seems the web-server does not support php.\n"+
              "Accessing data files not possible.");
        return;
    }

    if (result=="V8Js missing")
    {
        alert("The php module V8Js is missing or not properly installed.\n"+
              "Check here for details: https://github.com/phpv8/v8js");
        return;
    }
    //var dbg = document.getElementById("debug");

    //var pre = document.createElement("pre");
    //pre.appendChild(document.createTextNode(rc));
    //dbg.appendChild(pre);

    var rc;
    try
    {
        rc = JSON.parse(result);
    }
    catch (e)
    {
        alert("ERROR[0] - Decoding answer:\n"+e);
        debug(result);
        return;
    }

    document.getElementById("file").data = rc;

    var list = [ ];
    for (var file in rc)
        list.push(file);

    var opts =
    {
        source: list,
        select: onFileSelect,
        position: { my: "right top", at: "right bottom", collision: "flipfit" },
    };

    $("#file").autocomplete(opts);
    //document.getElementById("file").value = "2014/04/17-181";

    //onSubmit("2014/04/17-181");
}

function setZoom(xfrom, xto, yfrom, yto)
{
    var xaxis = plot.getXAxes()[0];
    var yaxis = plot.getYAxes()[0];

    if (xfrom!==undefined)
        xaxis.options.min = xfrom;
    if (xto!==undefined)
        xaxis.options.max = xto;

    if (yfrom!==undefined)
        yaxis.options.min = yfrom;
    if (yto!==undefined)
        yaxis.options.max = yto;

    plot.setupGrid();
    plot.draw();
    plot.clearSelection();
}

function onPlotHover(event, pos, item)
{
    if (!item)
    {
        $("#tooltip").hide();//fadeOut(100);
        return;
    }

    var x = item.datapoint[0].toFixed(2);
    var y = item.datapoint[1].toFixed(2);

    var tooltip = $("#tooltip");
    tooltip.html(parseInt(x) + " / " + y);
    tooltip.css({top: item.pageY-20, left: item.pageX+5});
    tooltip.show();//fadeIn(200);
}

function drawHist(n)
{
    var canv = document.getElementById("camera"+n);
    var hist = document.getElementById("hist"+n);

    var xmin  = parseFloat(document.getElementById("histmin"+n).value);
    var xmax  = parseFloat(document.getElementById("histmax"+n).value);
    var nbins = 100;//parseInt(xmax-xmin);
    var step  = (xmax-xmin)/nbins;
    if (step<1)
    {
        step = 1;
        nbins = parseInt(xmax-xmin)+1;
    }

    var bins = new Array(nbins);
    for (var i=0; i<nbins; i++)
        bins[i] = [ xmin+i*step, 0 ];

    var data = canv.dataAbs;
    for (var i=0; i<1440; i++)
        if (data[i]!==undefined/* && data[i]!==null*/)
        {
            var ix = parseInt((data[i]-xmin)/step);
            if (ix>=0 && ix<nbins)
                bins[ix][1] ++;
        }

    var opts =
    {
        grid: {
            hoverable: true,
        }
    };

    var hist = $.plot("#hist"+n, [ { data:bins, bars: {show:true} } ], opts);
    $('#hist'+n).bind("plothover", onPlotHover);
}

function processCameraData(n, data)
{
    var canv = document.getElementById("camera"+n);
    canv.dataRel = null;

    if (!Array.isArray(data))
        return;

    canv.dataAbs = new Array(1440);
    for (var i=0; i<1440; i++)
    {
        var val = data[map[i]];
        if (!isNaN(val) && val!==null)
            canv.dataAbs[i] = val;
    }

    canv.min = Math.min.apply(Math, canv.dataAbs.filter(function(e){return e!==undefined;}));
    canv.max = Math.max.apply(Math, canv.dataAbs.filter(function(e){return e!==undefined;}));

    canv.dataRel = new Array(1440);
    for (var i=0; i<1440; i++)
    {
        var val = canv.dataAbs[i];
        if (val!==undefined)
            canv.dataRel[i] = (val-canv.min)/canv.max;
    }

    if (document.getElementById("cameraminon"+n).checked)
        document.getElementById("cameramin"+n).value = canv.min;
    if (document.getElementById("cameramaxon"+n).checked)
        document.getElementById("cameramax"+n).value = canv.max;

    // ---------------------------

    var hist = document.getElementById("hist"+n);

    hist.min = canv.min;
    hist.max = canv.max;

    if (document.getElementById("histminon"+n).checked)
        document.getElementById("histmin"+n).value = canv.min;
    if (document.getElementById("histmaxon"+n).checked)
        document.getElementById("histmax"+n).value = canv.max;

    drawHist(n);
}

function onDataReceived(result)
{
    var err = document.getElementById("error");
    var dbg = document.getElementById("debug");
    var con = document.getElementById("console");

    //var pre = document.createElement("pre");
    //pre.appendChild(document.createTextNode(rc));
    //dbg.appendChild(pre);

    var rc;
    try
    {
        rc = JSON.parse(result);
        if (!rc)
            return;
    }
    catch (e)
    {
        alert("ERROR[1] - Decoding answer:\n"+e);
        debug(result);
        return;
    }

    debug(rc.command);

    var evt = rc.event;
    var file = rc.file;

    document.getElementById("event").max = file.numEvents-1;
    var el = document.getElementById("numevents");
    if (el.firstChild)
        el.removeChild(el.firstChild);
    el.appendChild(document.createTextNode(file.numEvents));

    var infotxt = "<pre>";
    infotxt += "\nStart time: "+new Date(file.runStart*24*3600*1000).toUTCString();
    infotxt += "\nEnd   time: "+new Date(file.runEnd*24*3600*1000).toUTCString();
    infotxt += "\nRun   type: "+file.runType;
    if (file.drsFile>=0)
        infotxt += " [drs-step "+file.drsFile+"]";

    $("#runinfo").html(infotxt);
    $("#eventinfo").html("Trigger: "+evt.trigger.join(' | ')+" [0x"+evt.triggerType.toString(16)+"]");

    if (rc.ret)
    {
        while (con.lastChild)
            con.removeChild(con.lastChild);
    }

    if (rc.err)
    {
        while (err.lastChild)
            err.removeChild(err.lastChild);

        err.appendChild(document.createTextNode("Javascript runtime exception: "+rc.err.file+":"+rc.err.lineNumber));
        err.appendChild(document.createTextNode("\n"));
        err.appendChild(document.createTextNode(rc.err.sourceLine));
        err.appendChild(document.createTextNode("\n"));
        err.appendChild(document.createTextNode(rc.err.trace));

        var editor = rc.err.file=="main" ? editor2 : editor1;
        editor.setCursor(rc.err.lineNumber-1, 1);

        $("#accordion").find('h3').filter(':contains(Runtime)').show();
        $("#accordion").accordion("option", "active", 0);
    }

    if (rc.debug!==undefined)
    {
        con.appendChild(document.createTextNode(rc.debug));

        debug("PHP execution:");
        debug("Time Javascripts = "+(rc.timeJs[0]*1000).toFixed(2)+","+(rc.timeJs[1]*1000).toFixed(2)+","+(rc.timeJs[2]*1000).toFixed(2)+ " [ms]");
    }

    if (rc.ret!==undefined && Array.isArray(rc.ret))
    {
        var now = new Date();

        if (rc.ret[0] instanceof Object)
            processCameraData(1, rc.ret[0]);
        else
            processCameraData(1, rc.ret);

        if (rc.ret.length>1)
            processCameraData(2, rc.ret[1]);

        if (rc.ret.length>2)
            processCameraData(3, rc.ret[2]);

        if (rc.ret.length>3)
            processCameraData(4, rc.ret[3]);

        debug("Calc Time = "+(new Date()-now)+" ms");
    }

    // We have to redraw all of them to display the changed pixel value
    onCameraMinMax(1);
    onCameraMinMax(2);
    onCameraMinMax(3);
    onCameraMinMax(4);

    debug("Total time = "+(rc.timePhp*1000).toFixed(1)+" ms");
    debug("Peak memory = "+rc.memory+" MiB");

    if (Array.isArray(rc.waveform))
    {
        var waveform = document.getElementById("waveform");
        waveform.data = [ ];

        var data = [
                    { label: "[0] ", data: new Array(evt.numRoi) },
                    { label: "[1] ", data: new Array(evt.numRoi) },
                    { label: "[2] ", data: new Array(evt.numRoi) },
                    { label: "[3] ", data: new Array(evt.numRoi) },
                    ];

        var min = [];
        var max = [];
        if (Array.isArray(rc.waveform) && rc.waveform.length==evt.numRoi)
        {
            min.push(Math.min.apply(Math, rc.waveform));
            max.push(Math.max.apply(Math, rc.waveform));

            var d = data[0].data;
            for (var i=0; i<evt.numRoi; i++)
                d[i] = [ i, rc.waveform[i] ];

            waveform.data[0] = rc.waveform;
        }

        for (var j=0; j<4; j++)
        {
            var ref = rc.waveform[j];

            if (Array.isArray(ref) && ref.length==evt.numRoi)
            {
                min.push(Math.min.apply(Math, ref));
                max.push(Math.max.apply(Math, ref));

                var d = data[j].data;
                for (var i=0; i<evt.numRoi; i++)
                    d[i] = [ i, ref[i] ];

                waveform.data[j] = ref;
            }
        }

        waveform.ymin = Math.min.apply(Math, min);
        waveform.ymax = Math.max.apply(Math, max);
        waveform.xmin = 0;
        waveform.xmax = evt.numRoi;

        if (document.getElementById("waveformxminon").checked)
            document.getElementById("waveformxmin").value = waveform.xmin;
        if (document.getElementById("waveformxmaxon").checked)
            document.getElementById("waveformxmax").value = waveform.xmax;

        if (document.getElementById("waveformminon").checked)
            document.getElementById("waveformmin").value = waveform.ymin;
        if (document.getElementById("waveformmaxon").checked)
            document.getElementById("waveformmax").value = waveform.ymax;

        var xmin = document.getElementById("waveformxminon").checked ? waveform.xmin : parseInt(document.getElementById("waveformxmin").value);
        var xmax = document.getElementById("waveformxmaxon").checked ? waveform.xmax : parseInt(document.getElementById("waveformxmax").value);

        var ymin = document.getElementById("waveformminon").checked ? waveform.ymin : parseInt(document.getElementById("waveformmin").value);
        var ymax = document.getElementById("waveformmaxon").checked ? waveform.ymax : parseInt(document.getElementById("waveformmax").value);

        var opts =
        {
           xaxis: {
               min: xmin-1,
               max: xmax+1,
           },
           yaxis: {
               min: ymin-5,
               max: ymax+5,
           },
           series: {
               lines: {
                   show: true
               },
               points: {
                   show: true,
                   symbol: 'cross',
               }
           },
           selection: {
               mode: "xy"
           },
           grid: {
               hoverable: true,
           }
        };

        plot = $.plot("#waveform", data, opts);

        waveform = $('#waveform');
        waveform.bind("plotselected", function (event, ranges)
                      {
                          setZoom(ranges.xaxis.from, ranges.xaxis.to,
                                  ranges.yaxis.from, ranges.yaxis.to);
                      });

        waveform.dblclick(function ()
                          {
                              var waveform = document.getElementById("waveform");
                              setZoom(waveform.xmin-1, waveform.xmax+1, waveform.ymin-5, waveform.ymax+5);
                          });
        waveform.bind("plothover", onPlotHover);
    }
}

function onSubmit(file, pixelOnly)
{
    if (!file)
        file = document.getElementById("file").value;

    var dbg = document.getElementById("debug");
    while (dbg.lastChild)
        dbg.removeChild(dbg.lastChild);

    var active = $("#accordion").accordion("option", "active");
    if (active==0)
    {
        $("#accordion").accordion("option", "active", false);
        $("#accordion").find('h3').filter(':contains(Runtime)').hide();
    }

    var calibrated = document.getElementById("calibrated");
    var drsfile    = document.getElementById("drsfile");
    var montecarlo = document.getElementById("montecarlo");
    var ismc       = montecarlo.checked;
    var calib      = !calibrated.disabled && calibrated.checked;
    var drs        = !drsfile.disabled && drsfile.checked;
    var event      = document.getElementById("event").value;
    var pixel      = document.getElementById("pixel").value;
    var source1    = editor1.getValue();
    var source2    = editor2.getValue();

    var uri = "file="+file+"&event="+event+"&pixel="+map[pixel];
    uri += "&source1="+encodeURIComponent(source1);
    if (!pixelOnly)
        uri += "&source2="+encodeURIComponent(source2);
    if (calib)
        uri += "&calibrated=1";
    if (drs)
        uri += "&drsfile=1";
    if (ismc)
        uri += "&montecarlo=1";

    $.ajax({
        type:    "POST",
        cache:   false,
        url:     "index.php",
        data:    uri,
        success: onDataReceived,
        error:   function(xhr) { if (xhr.status==0) alert("ERROR[1] - Request failed!"); else alert("ERROR[1] - "+xhr.statusText+" ["+xhr.status+"]"); }
    });
}

function onFile(event, ui)
{
    var f = event.target.files[0];
    if (!f)
        return;

    if (!f.type.match('text/plain') && !f.type.match('text/javascript') && !f.type.match('application/javascript') && !f.type.match('application/x-javascript'))
    {
        alert("ERROR - Unknown file type: "+f.type);
        return;
    }

    var id     = event.target.id;
    var editor = id[id.length-1]=='1' ? editor1 : editor2;

    var reader = new FileReader();

    // Closure to capture the file information.
    reader.onload = (function(theFile) { return function(e) { editor.setValue(e.target.result); }; })(f);
    // onloadstart
    // onloadend
    // onprogress

    // Read in the text file
    reader.readAsText(f);
}

function refreshCameras()
{
    drawFullCam("camera1");
    drawFullCam("camera2");
    drawFullCam("camera3");
    drawFullCam("camera4");
}

function onEvent()
{
    onSubmit();
}

function checkPixel()
{
    var pix  = parseInt(document.getElementById("pixel").value);
    var c    = parseInt(document.getElementById("cbpx-c").value);
    var b    = parseInt(document.getElementById("cbpx-b").value);
    var p    = parseInt(document.getElementById("cbpx-p").value);
    var x    = parseInt(document.getElementById("cbpx-x").value);
    var cbpx = parseInt(document.getElementById("cbpx").value);;

    if (pix >=0 && pix <1440 &&
        c   >=0 && c   <   4 &&
        b   >=0 && b   <  10 &&
        p   >=0 && p   <   4 &&
        x   >=0 && x   <   9 &&
        cbpx>=0 && cbpx<1440)
        return;

    document.getElementById("pixel").value = 0;
    document.getElementById("cbpx-c").value = 1;
    document.getElementById("cbpx-b").value = 0;
    document.getElementById("cbpx-p").value = 3;
    document.getElementById("cbpx-x").value = 6;
    document.getElementById("cbpx").value = 393;
}


function onPixel()
{
    checkPixel();

    var p = parseInt(document.getElementById("pixel").value);

    var cbpx = map[p];

    document.getElementById("cbpx-c").value = parseInt((cbpx/360));
    document.getElementById("cbpx-b").value = parseInt((cbpx/36)%10);
    document.getElementById("cbpx-p").value = parseInt((cbpx/9)%4);
    document.getElementById("cbpx-x").value = parseInt((cbpx%9));
    document.getElementById("cbpx").value = parseInt(cbpx);

    onSubmit("", true);
}

function onCBPX()
{
    checkPixel();

    var c = parseInt(document.getElementById("cbpx-c").value);
    var b = parseInt(document.getElementById("cbpx-b").value);
    var p = parseInt(document.getElementById("cbpx-p").value);
    var x = parseInt(document.getElementById("cbpx-x").value);

    var cbpx = c*360 + b*36 + p*9 + x;

    document.getElementById("cbpx").value = parseInt(cbpx);
    document.getElementById("pixel").value = map.indexOf(cbpx);

    onSubmit("", true);
}

function onHW()
{
    checkPixel();

    var cbpx = parseInt(document.getElementById("cbpx").value);;

    document.getElementById("cbpx-c").value = parseInt((cbpx/360));
    document.getElementById("cbpx-b").value = parseInt((cbpx/36)%10);
    document.getElementById("cbpx-p").value = parseInt((cbpx/9)%4);
    document.getElementById("cbpx-x").value = parseInt((cbpx%9));

    document.getElementById("pixel").value = map.indexOf(cbpx);

    onSubmit("", true);
}

function isInside(x, y, mouse)
{
    var dist = Math.sqrt((mouse.x-x)*(mouse.x-x)+(mouse.y-y)*(mouse.y-y));
    return dist<0.5;

    /*
    ctx.translate(x, y);
    ctx.scale(1/2, 1/3);

    ctx.beginPath();
    ctx.moveTo( 1,  1);
    ctx.lineTo( 0,  2);
    ctx.lineTo(-1,  1);
    ctx.lineTo(-1, -1);
    ctx.lineTo( 0, -2);
    ctx.lineTo( 1, -1);
    ctx.fill();

    ctx.restore();
    */
}

var inprogress = { };
function moveElement(id, n, target, callback)
{
    if (inprogress[id]==n || inprogress[id]<0)
        return;

    inprogress[id] = target ? -n : n;

    var element   = $("#"+id+n); //Allow passing in either a JQuery object or selector
    var newParent = $(target ? target : "#cont"+id+n); //Allow passing in either a JQuery object or selector

    var oldOffset = element.offset();

    var newOffset = newParent.offset();

    var w = newParent.width();
    var h = newParent.height();

    var temp = element.appendTo('body');
    temp.css('position', 'absolute')
        .css('left', oldOffset.left)
        .css('top',  oldOffset.top)
        .css('zIndex', 999);

    temp.animate( {'top': newOffset.top, 'left':newOffset.left, 'width':w, 'height': h},
    'slow', function()
    {
        temp = temp.appendTo(newParent);
        temp.css('position', 'relative');
        temp.css('width', '');
        temp.css('height', '');
        temp.css('zIndex', '');
        temp.css('left', '0');
        temp.css('top', '0');

        setSize(id+n, w, h);

        if (callback)
            callback(id+n);

        inprogress[id] = 0;
    });
}

function onClickCont(event, callback)
{
    var id = event.target.id;
    if (!id)
        id = event.target.parentNode.id;

    var n = parseInt(id[id.length-1]);
    var type = id.substr(0, id.length-1);

    if (id.substr(0, 4)=="cont")
        id = id.substr(4, id.length-4);
    if (type.substr(0, 4)=="cont")
        type = type.substr(4, type.length-4);

    if (id.substr(0, type.length)==type)
    {
        var cont = document.getElementById("center"+type).childNodes[0];
        if (cont)
        {
            var nn = parseInt(cont.id[cont.id.length-1]);
            moveElement(type, nn, null, callback);
        }
        moveElement(type, n, "#center"+type, callback);

    }
    else
        moveElement(type, n, null, callback);

}

function onClickContCamera(event)
{
    onClickCont(event, function(el) { drawFullCam(el); });
}

function onClickContHist(event)
{
    onClickCont(event);
}

function onClick(event)
{
    var cont = document.getElementById("centercamera").childNodes[0];
    if (!cont)
        return;

    if (cont.id!=event.target.id)
        return;

    // get click position relative to canvas
    var rect = event.target.getBoundingClientRect();

    var x =  event.clientX - rect.left;
    var y =  event.clientY - rect.top;

    var mouse = { x: x, y: y };

    // convert click position to pixel index
    var index = getIndex(event.target.id, mouse);
    if (index<0)
        return;

    document.getElementById("pixel").value = index;

    onPixel();
}

function getClickPosition(event)
{
    var rect = event.target.getBoundingClientRect();

    var x =  event.clientX - rect.left;
    var y =  event.clientY - rect.top;

    return { x: x, y: y };
}

function onMinMax(id, n)
{
    var el = document.getElementById(id+n);

    el.zmin = document.getElementById(id+"min"+n).value;
    el.zmax = document.getElementById(id+"max"+n).value;
}

function onCameraMinMax(n)
{
    onMinMax("camera", n);
    drawFullCam("camera"+n);
}

function onHistMinMax(n)
{
    onMinMax("hist", n);
    drawHist(n);
}

function onMinMaxOn(id, n)
{
    var el = document.getElementById(id+n);

    var redraw;
    if (document.getElementById(id+"minon"+n).checked)
    {
        document.getElementById(id+"min"+n).setAttribute("disabled", "true");
        document.getElementById(id+"min"+n).value = el.min;
        redraw = true;
    }
    else
        document.getElementById(id+"min"+n).removeAttribute("disabled");

    if (document.getElementById(id+"maxon"+n).checked)
    {
        document.getElementById(id+"max"+n).setAttribute("disabled", "true");
        document.getElementById(id+"max"+n).value = el.max;
        redraw = true;
    }
    else
        document.getElementById(id+"max"+n).removeAttribute("disabled");

    return redraw;
}

function onCameraMinMaxOn(n)
{
    if (onMinMaxOn("camera", n))
        onCameraMinMax(n);
}

function onHistMinMaxOn(n)
{
    if (onMinMaxOn("hist", n))
        onHistMinMax(n);
}

function onWaveformMinMax()
{
    var wf = document.getElementById("waveform");

    var xmin, xmax, ymin, ymax;

    var redraw;
    if (!document.getElementById("waveformxminon").checked)
        xmin = document.getElementById("waveformxmin").value;
    if (!document.getElementById("waveformxmaxon").checked)
        xmax = document.getElementById("waveformxmax").value;
    if (!document.getElementById("waveformminon").checked)
        ymin = document.getElementById("waveformmin").value;
    if (!document.getElementById("waveformmaxon").checked)
        ymax = document.getElementById("waveformmax").value;

    setZoom(xmin, xmax, ymin, ymax);

}

function onWaveformMinMaxOn()
{
    var wf = document.getElementById("waveform");

    var xmin, xmax, ymin, ymax;

    var redraw;
    if (document.getElementById("waveformxminon").checked)
    {
        document.getElementById("waveformxmin").setAttribute("disabled", "true");
        document.getElementById("waveformxmin").value = wf.xmin;
        xmin = wf.xmin-1;
    }
    else
        document.getElementById("waveformxmin").removeAttribute("disabled");

    if (document.getElementById("waveformxmaxon").checked)
    {
        document.getElementById("waveformxmax").setAttribute("disabled", "true");
        document.getElementById("waveformxmax").value = wf.xmax;
        xmax = wf.xmax+1;
    }
    else
        document.getElementById("waveformxmax").removeAttribute("disabled");

    if (document.getElementById("waveformminon").checked)
    {
        document.getElementById("waveformmin").setAttribute("disabled", "true");
        document.getElementById("waveformmin").value = wf.ymin;
        ymin = wf.ymin-5;
    }
    else
        document.getElementById("waveformmin").removeAttribute("disabled");

    if (document.getElementById("waveformmaxon").checked)
    {
        document.getElementById("waveformmax").setAttribute("disabled", "true");
        document.getElementById("waveformmax").value = wf.ymax;
        ymax = wf.ymax+5;
    }
    else
        document.getElementById("waveformmax").removeAttribute("disabled");

    setZoom(xmin, xmax, ymin, ymax);
}

//document.addEventListener("click", getClickPosition, false);

$(document).ready(onReady);

// ================================== Pixel mapping =================================================

var map = new Array(1440);

function initPixelMapFACT()
{
    var codedMap = "966676:6:A;68656364626Y?\\?;A=A<AGADAN4K4i5g5h5o506W?Z?]?_?>A@A?AJAIAFACAM4J4H4f5d5e5l5m5n516X?[?^?N?P?AA1ABAVAUAKAHAEAO4L4I4G4E4c5a5b5M6j5k5V6Y6\\6_6G?J?O?Q?S?2A4A3AYAXAWAbA_AnAkAhA3404F4D4B4`5^5_5J6K6L6S6T6W6Z6]6E?H?K?M?R?T?V?5A7A6A\\A[AZAeAdAaA^AmAjAgA24o3m3C4A4?4]5[5\\5G6H6I6P6Q6R6U6X6[6^6F?I?L?<?>?U?;@=@8Ah@9AMALA]ABCACfAcA`AoAlAiA4414n3l3<4@4>425Z5X5Y5D6E6F698N6O608E8H8K8H>K>N>?>B>=???A?<@>@@@i@k@j@PAOANAECDCCC<C9CZCWCTC=3:3734313=4;4943515o4W5U5V5A6B6C6687888m7n7C8F8I8F>I>L>=>@>C>E>@?B?D??@A@C@l@n@m@SARAQAHCGCFC?C>C;C8CYCVCSC<393633303n2:4846405n4l4T5R5S5>6?6@6384858j7k7l7o7D8G8J8G>J>M>>>A>D>4>6>C?3?5?B@2@4@o@_@0ALBKBTA0CoBIC8D7D@C=C:C[CXCUC>3;3835323o2m2k27454j3m4k4i4Q5O5P5C7<6=6g71828o8h7i7S9<8?8B8Q>T>W>@=C=F=e<h<5>7>9>4?6?8?3@5@7@`@b@a@OBNBMB3C2C1C;D:D9D_D\\DQCNCKCF3C3@3>2;282Z1W1l2j2h2k3i3g3j4h4f4N5L5M5@7A7B7d7e7f7l8m8n8P9Q9:8=8@8O>R>U>>=A=D=c<f<i<k<8>:><>7?9?;?6@8@:@c@e@d@RBQBPB6C5C4C>D=D<DbDaD^D[DPCMCJCE3B3?3=2:272Y1V1T1i2g2e2h3f3d3g4e4c4K5I5J5=7>7?7a7b7c7i8j8k8M9N9O9R9;8>8A8P>S>V>?=B=E=d<g<j<Z<\\<;>k=m=:?j>l>9@i?k?f@V@g@CBBBSBgBfB7CoCnC?DSDRDcD`D]DRCOCLCG3D3A3?2<292[1X1U1S1Q1f2d2b2e3c3a3d4b4`4H5F5G5:7;7<7^7_7`7f8g8h8J9K9L97:::=:@:C:F:I:I=L=O=7=:===n<1=[<]<_<l=n=0>k>m>o>j?l?n?W@Y@X@FBEBDBjBiBhB2D1D0DVDUDTDCE@EOELEIEXEUERE5222o1l1i1f1c1`1R1P1N1c2a2_2b3`3^3a4_4]4<5:5;5778797[7\\7]7c8d8e8G9H9I94:5:8:;:>:A:D:G:G=J=M=5=8=;=l<o<2=4=^<`<b<o=1>3>n>0?2?m?o?1@Z@\\@[@IBHBGBmBlBkB5D4D3DYDXDWDFEEEBE?ENEKEHEWETEQE4212n1k1h1e1b1_1]1O1M1K1`2^2\\2_3]3[3^4\\4Z4957585475767X7Y7Z7`8a8b8D9E9F91:2:3:6:9:<:?:B:E:H:H=K=N=6=9=<=m<0=3=d;f;a<H<J<2>b=d=1?a>c>0@`?b?]@D@^@:B9BJB^B]BnBfCeC6DJDIDZDnDmDGEDEAEPEMEJEYEVESE623202m1j1g1d1a1^1\\1[0L1J1?1]2[2Y2\\3Z3X3[4Y4W4654555172737U7V7W7]8^8_8A9B9C9e9o90:n9g:j:m:L:O:R:U:X:[:];`;c;T;W;Z;7<9<e;g;i;I<K<M<c=e=g=b>d>f>a?c?e?E@G@F@=B<B;BaB`B_BiChCgCMDLDKD1E0EoD9E7E<F9F6FaE^E[EjEgEdER0O0L0I0F0C0n0l0\\0Z0X0@1>1<1Z2X2V2Y3W3U3X4V4T4E5C5D5n6o607R7S7T7Z8[8\\8>9?9@9b9c9d9l9m9e:h:k:J:M:P:S:V:Y:[;^;a;R;U;X;6<8<:<;<h;j;l;L<N<P<f=h=j=e>g>i>d?f?h?N@Q@H@@B?B>BdBcBbBlCkCjCPDODND4E3E2E;E:E8E6E;F8F5F`E]EZEiEfEcEQ0N0K0H0E0B0m0k0j0Y0W0U0=1;191W2U2S2V3T3R3U4S4Q4B5@5A5k6l6m6O7P7Q7W8X8Y8;9<9=9_9`9a9j9k9\\:_:f:i:l:K:N:Q:T:W:Z:\\;_;b;S;V;Y;C<E<G<<<m;k;1<2<O<Q<S<i=[=P=h>X>Z>g?M@O@R@U@S@J@I@AB1B0BeBTB\\CmCAD@DQDiDhD5EdD<E4F2F0F=F:F7FbE_E\\EkEhEeES0P0M0J0G0D011o0h0g0e0V0T0`0:181Q2T2R2H2S3Q3P3R4P4K3?5=5>5c6i6j6h6M7N7L7U8V8T899:9W9]9^9\\9g9h9i9]:`:c:5;2;o:>;;;8;P;M;J;G;D;A;?<A<D<F<=<><n;o;3<5<R<T<Y=Z=Q=R=Y>[>\\>^>P@T@L@K@5B4B3B2BVBUB^C]CCDBDkDjDfDeD>E=E3F1FnElE@FCFFFIFLFOF;0>0A0205080513101i0f0d0c0b0_0I1H1D1P2O2G2F2D2O3N3M3J3H3a6b6e6f6g6I7J7K7R8S8397989V9Y9Z9[9f9^:a:d:4;1;n:=;:;7;O;L;I;F;C;@;@<B<0<4<U<V<X<`=]=\\=S=U=W=]>_>`>8B7B6BZBXBWB_CaC`CFDEDDDlDgDoEmE>FAFDFGFJFMF90<0?0003060714121a0^0]0G1C1B1N2L2E2C2B2@2L3I3`6d6E7F7G7H7O8Q8192969T9U9X96;3;0;?;<;9;Q;N;K;H;E;B;W<Y<^=_=a=T=V=X=\\B[BYBdCcCbCHDGD?FBFEFHFKFNF:0=0@0104070F1E1A1M2K2J2I2A2D7L8M8N8P809495961b:";
    // first: decode the pixel mapping!
    var sum = 1036080;
    for (var i=0; i<1440; i++)
    {
        var d0 = codedMap.charCodeAt(i*2)  -48;
        var d1 = codedMap.charCodeAt(i*2+1)-48;

        map[i] = d0 | (d1<<6);
        sum -= map[i];
    }
    if (sum!=0)
        alert("Pixel mapping table corrupted ["+sum+"]!");
}

function initPixelMapFAMOUS()
{
    var codedMap = "C0D0B0J0I0H0E0\\0=0<07080L0K0h0k0^0]0[0Y0@0>0:0403060M0O0N0g0i0l0a0b0_0Z0X0U0V0A0?0;09010002050P0S0R0Q0f0j0m0n0d0e0c0`0W0T0o01101213141516171F0G0";
    // first: decode the pixel mapping!
    var sum = 2556;
    for (var i=0; i<72; i++)
    {
        var d0 = codedMap.charCodeAt(i*2)  -48;
        var d1 = codedMap.charCodeAt(i*2+1)-48;

        map[i] = d0 | (d1<<6);
        sum -= map[i];
    }
    if (sum!=0)
        alert("Pixel mapping table corrupted ["+sum+"]!");
}

if (camera_type=="FACT")
    initPixelMapFACT();
else
    initPixelMapFAMOUS();

// ================================== Camera Display ================================================

var coord = new Array(1440);
function initCameraCoordinatesFACT()
{
    coord[0] = [0, 0];
    var cnt = 1;
    for (var ring=1; ring<24; ring++)
    {
        for (var s=0; s<6; s++)
        {
            for (var i=1; i<=ring; i++)
            {
                var pos = new Position(s, ring, i);
                if (pos.d() - pos.x > 395.75)
                    continue;

                coord[cnt++] = [ pos.x, pos.y];
            }
        }
    }

    coord[1438] = [7, -22];
    coord[1439] = [7,  22];
}

function initCameraCoordinatesFAMOUS()
{
    var pos;

    coord[0] = [0,0];
    var cnt = 1;
    for (var ring=1; ring<5; ring++)
    {
        for (var s=0; s<6; s++)
        {
            for (var i=1; i<=ring; i++)
            {
                pos = new Position(s, ring, i);
                coord[cnt++] = [ pos.x, -pos.y];
            }
        }
    }

    pos = new Position(3, 6, 2);
    coord[cnt++] = [pos.x, -pos.y];
    pos = new Position(4, 6, 4);
    coord[cnt++] = [pos.x, -pos.y];
    pos = new Position(0, 6, 2);
    coord[cnt++] = [pos.x, -pos.y];
}

if (camera_type=="FACT")
    initCameraCoordinatesFACT();
else
    initCameraCoordinatesFAMOUS();


function getIndex(id, mouse)
{
    var canv = document.getElementById(id);

    var w = Math.min(canv.width/scale, canv.height/scale);

    //ctx.translate(canv.width/2, canv.height/2);
    //ctx.scale(w*2, w*2);
    //ctx.scale(1, Math.sqrt(3)/2);
    //ctx.translate(-0.5, 0);

    mouse.x -= canv.width/2;
    mouse.y -= canv.height/2;
    mouse.x /= w*2;
    mouse.y /= w*2;
    mouse.y /= Math.sqrt(3)/2;
    mouse.x -= -0.5;

    for (var i=0; i<npix; i++)
        if (isInside(coord[i][0], coord[i][1], mouse))
            return i;

    return -1;
}


function hueToRGB(hue)
{
    hue /= 3;
    hue %= 6;

    if (hue<1) return parseInt(255*hue,     10);
    if (hue<3) return parseInt(255,         10);
    if (hue<4) return parseInt(255*(4-hue), 10);

    return 0.
}

function hueToHex(flt)
{
    var s = hueToRGB(flt).toString(16);
    return s.length==2 ? s : "0"+s;
}

function HLStoRGB(hue)
{
    if (isNaN(hue))
        return "fff";

    if (hue<0)
        return "e5f"; // 555

    if (hue>1)
        return "600";//"dde";//"700"; // 666

    hue *= 14;

    var sr = hueToHex(20-hue);
    var sg = hueToHex(14-hue);
    var sb = hueToHex(26-hue);

    return sr+sg+sb;
}

function outlineHex(ctx)
{
    ctx.scale(1/2, 1/3);

    ctx.beginPath();
    ctx.moveTo( 1,  1);
    ctx.lineTo( 0,  2);
    ctx.lineTo(-1,  1);
    ctx.lineTo(-1, -1);
    ctx.lineTo( 0, -2);
    ctx.lineTo( 1, -1);
    ctx.lineTo( 1,  1);
}

function fillHex(ctx, i, col, min, max)
{
    if (col===undefined/* || col===null*/)
        return false;

    var lvl = max==min ? 0.5 : (col-min)/(max-min);

    ctx.fillStyle = "#"+HLStoRGB(lvl);

    ctx.save();
    ctx.translate(coord[i][0], coord[i][1]);
    outlineHex(ctx);
    ctx.fill();
    ctx.restore();

    return true;
}

function drawHex(ctx, i)
{
    ctx.save();
    ctx.translate(coord[i][0], coord[i][1]);
    outlineHex(ctx);
    ctx.stroke();
    ctx.restore();
}


function Position(s, ring, i)
{
    switch (s)
    {
    case 1: this.x =  ring     - i*0.5;  this.y =       + i; break;
    case 2: this.x =  ring*0.5 - i;      this.y =  ring    ; break;
    case 3: this.x = -ring*0.5 - i*0.5;  this.y =  ring - i; break;
    case 4: this.x = -ring     + i*0.5;  this.y =       - i; break;
    case 5: this.x = -ring*0.5 + i;      this.y = -ring    ; break;
    case 0: this.x =  ring*0.5 + i*0.5;  this.y = -ring + i; break;
    }
    this.d = (function () { return this.x*this.x + this.y*this.y*3/4; });
}

function drawFullCam(id)
{
    var canv = document.getElementById(id);
    if (!canv)
        return;

    var ctx = canv.getContext("2d");

    ctx.clearRect(0, 0, canv.width, canv.height);

    // ======================= Draw Graphics ======================

    var data = canv.dataRel;
    if (!data)
        return;

    var pixel = document.getElementById('pixel').value;

    var min = (canv.zmin-canv.min)/canv.max;
    var max = (canv.zmax-canv.min)/canv.max;

    var w = Math.min(canv.width/scale, canv.height/scale);

    ctx.save();
    ctx.translate(canv.width/2, canv.height/2);
    ctx.scale(w*2, w*2);
    // ctx.rotate(Math.PI/3);

    ctx.scale(1, Math.sqrt(3)/2);
    ctx.translate(-0.5, 0);

    if (document.getElementById('grid').checked)
    {
        ctx.lineWidth = 0.02;
        ctx.strokeStyle = "#000";
        for (var i=0; i<npix; i++)
            drawHex(ctx, i);
    }

    var hasData = false;
    if (max>=min)
        for (var i=0; i<npix; i++)
            hasData |= fillHex(ctx, i, data[i], min, max);

    // ======================= Draw Ellipse ======================

    if (document.getElementById('image').checked)
    {
        var h = Hillas(canv.dataAbs, canv.zmin, canv.zmax);
        if (h)
        {
            ctx.save();

            ctx.scale(1, 2/Math.sqrt(3));

            ctx.beginPath();
            ctx.moveTo(center[0], center[1]);
            ctx.lineTo(h.mean[0], h.mean[1]);

            ctx.strokeStyle = "#CCC";
            ctx.lineWidth = 0.1;
            ctx.stroke();

            ctx.translate(h.mean[0], h.mean[1]);
            ctx.rotate(h.phi);

            ctx.beginPath();
            ctx.moveTo(0, -h.disp);
            ctx.lineTo(0,  h.disp);

            ctx.strokeStyle = "#888";
            ctx.lineWidth = 0.15;
            ctx.stroke();

            ctx.save();
            ctx.scale(h.axis[0], h.axis[1]);
            ctx.beginPath();
            ctx.arc(0, 0, 1, 0, 2*Math.PI);
            ctx.restore();

            ctx.strokeStyle = "#555";
            ctx.lineWidth = 0.15;
            ctx.stroke();

            ctx.restore();
        }
    }

    // =================== Draw Pixel marker ====================

    if (document.getElementById('marker').checked)
    {
        // Draw marker
        ctx.lineWidth = 0.25;
        ctx.strokeStyle = "#000";
        drawHex(ctx, pixel);
    }


    ctx.restore();

    if (!hasData)
        return;

    // ======================= Draw Legend ======================

    var v0 = parseFloat(canv.zmin);
    var v1 = parseFloat(canv.zmax);

    var diff = v1-v0;

    var cw = canv.width;
    //var ch = canv.height;

    ctx.font         = "8pt Arial";
    ctx.textAlign    = "right";
    ctx.textBaseline = "top";

    for (var i=0; i<11; i++)
    {
        ctx.strokeStyle = "#"+HLStoRGB(i/10);
        ctx.strokeText((v0+diff*i/10).toPrecision(3), cw-5, 125-i*12);
    }

    var pval = parseFloat(canv.dataAbs[pixel]).toFixed(1);
    var lmin = parseFloat(canv.min).toFixed(1);
    var lmax = parseFloat(canv.max).toFixed(1);

    if (isNaN(pval))
        pval = "";

    var mw = Math.max(ctx.measureText(lmin).width,
                      ctx.measureText(pval).width,
                      ctx.measureText(lmax).width);

    ctx.textBaseline = "top";
    ctx.strokeStyle  = "#000";

    ctx.strokeText(lmax, 5+mw, 5+24);
    ctx.strokeText(pval, 5+mw, 5+12);
    ctx.strokeText(lmin, 5+mw, 5);
}

// ===================================================================

function Hillas(data, min, max)
{
    var mx = 0;
    var my = 0;
    var sz = 0;

    var mx2 = 0;
    var my2 = 0;
    var mxy = 0;

    var cnt = 0;
    for (var i=0; i<npix; i++)
    {
        if (data[i]===undefined || data[i]<min || data[i]>max)
            continue;

        sz  += data[i];
        mx  += data[i] * coord[i][0];
        my  += data[i] * coord[i][1];

        mx2 += data[i] * coord[i][0]*coord[i][0];
        my2 += data[i] * coord[i][1]*coord[i][1];
        mxy += data[i] * coord[i][0]*coord[i][1];

        cnt++;
    }

    if (sz==0 || cnt<3)
        return;

    // Coordinates need to be scaled in y
    var f = Math.sqrt(3)/2;

    my  *= f;
    mxy *= f;
    my2 *= f*f;

    var xx = mx2 - mx*mx/sz;
    var yy = my2 - my*my/sz;
    var xy = mxy - mx*my/sz;

    var d0  = yy - xx;
    var d1  = xy*2;
    var d2  = Math.sqrt(d0*d0 + d1*d1) + d0;

    var phi = 0;
    var cos = 0;
    var sin = 1;

    var axis1 = yy;
    var axis2 = xx;

    // Correction for scale in x
    var ratio = xx/yy;

    if (d1!=0 || d2==0)
    {
        var tand  = d2==0 ? 0 : d2 / d1;
        var tand2 = tand*tand;

        var s2    = tand2+1;
        var s     = Math.sqrt(s2);

        phi = Math.atan(tand)-Math.PI/2;
        cos = 1.0 /s;
        sin = tand/s;

        axis1 = (tand2*yy + d2 + xx);
        axis2 = (tand2*xx - d2 + yy);

        ratio = axis2/axis1;

        axis1 /= s2;
        axis2 /= s2;
    }

    var length = axis1<0 ? 0 : Math.sqrt(axis1/sz);
    var width  = axis2<0 ? 0 : Math.sqrt(axis2/sz);

    return {
        "mean":  [ mx/sz, my/sz ],
        "axis":  [ width, length ],
        "phi":   phi,
        "delta": [ cos, sin ],
        "sumw":  sz,
        "count": cnt,
        "disp":  1.47/diameter*(1-Math.sqrt(ratio)),
    };
}
