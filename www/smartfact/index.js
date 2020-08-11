"use strict";

var debug = false;

var codedMap = "966676:6:A;68656364626Y?\\?;A=A<AGADAN4K4i5g5h5o506W?Z?]?_?>A@A?AJAIAFACAM4J4H4f5d5e5l5m5n516X?[?^?N?P?AA1ABAVAUAKAHAEAO4L4I4G4E4c5a5b5M6j5k5V6Y6\\6_6G?J?O?Q?S?2A4A3AYAXAWAbA_AnAkAhA3404F4D4B4`5^5_5J6K6L6S6T6W6Z6]6E?H?K?M?R?T?V?5A7A6A\\A[AZAeAdAaA^AmAjAgA24o3m3C4A4?4]5[5\\5G6H6I6P6Q6R6U6X6[6^6F?I?L?<?>?U?;@=@8Ah@9AMALA]ABCACfAcA`AoAlAiA4414n3l3<4@4>425Z5X5Y5D6E6F698N6O608E8H8K8H>K>N>?>B>=???A?<@>@@@i@k@j@PAOANAECDCCC<C9CZCWCTC=3:3734313=4;4943515o4W5U5V5A6B6C6687888m7n7C8F8I8F>I>L>=>@>C>E>@?B?D??@A@C@l@n@m@SARAQAHCGCFC?C>C;C8CYCVCSC<393633303n2:4846405n4l4T5R5S5>6?6@6384858j7k7l7o7D8G8J8G>J>M>>>A>D>4>6>C?3?5?B@2@4@o@_@0ALBKBTA0CoBIC8D7D@C=C:C[CXCUC>3;3835323o2m2k27454j3m4k4i4Q5O5P5C7<6=6g71828o8h7i7S9<8?8B8Q>T>W>@=C=F=e<h<5>7>9>4?6?8?3@5@7@`@b@a@OBNBMB3C2C1C;D:D9D_D\\DQCNCKCF3C3@3>2;282Z1W1l2j2h2k3i3g3j4h4f4N5L5M5@7A7B7d7e7f7l8m8n8P9Q9:8=8@8O>R>U>>=A=D=c<f<i<k<8>:><>7?9?;?6@8@:@c@e@d@RBQBPB6C5C4C>D=D<DbDaD^D[DPCMCJCE3B3?3=2:272Y1V1T1i2g2e2h3f3d3g4e4c4K5I5J5=7>7?7a7b7c7i8j8k8M9N9O9R9;8>8A8P>S>V>?=B=E=d<g<j<Z<\\<;>k=m=:?j>l>9@i?k?f@V@g@CBBBSBgBfB7CoCnC?DSDRDcD`D]DRCOCLCG3D3A3?2<292[1X1U1S1Q1f2d2b2e3c3a3d4b4`4H5F5G5:7;7<7^7_7`7f8g8h8J9K9L97:::=:@:C:F:I:I=L=O=7=:===n<1=[<]<_<l=n=0>k>m>o>j?l?n?W@Y@X@FBEBDBjBiBhB2D1D0DVDUDTDCE@EOELEIEXEUERE5222o1l1i1f1c1`1R1P1N1c2a2_2b3`3^3a4_4]4<5:5;5778797[7\\7]7c8d8e8G9H9I94:5:8:;:>:A:D:G:G=J=M=5=8=;=l<o<2=4=^<`<b<o=1>3>n>0?2?m?o?1@Z@\\@[@IBHBGBmBlBkB5D4D3DYDXDWDFEEEBE?ENEKEHEWETEQE4212n1k1h1e1b1_1]1O1M1K1`2^2\\2_3]3[3^4\\4Z4957585475767X7Y7Z7`8a8b8D9E9F91:2:3:6:9:<:?:B:E:H:H=K=N=6=9=<=m<0=3=d;f;a<H<J<2>b=d=1?a>c>0@`?b?]@D@^@:B9BJB^B]BnBfCeC6DJDIDZDnDmDGEDEAEPEMEJEYEVESE623202m1j1g1d1a1^1\\1[0L1J1?1]2[2Y2\\3Z3X3[4Y4W4654555172737U7V7W7]8^8_8A9B9C9e9o90:n9g:j:m:L:O:R:U:X:[:];`;c;T;W;Z;7<9<e;g;i;I<K<M<c=e=g=b>d>f>a?c?e?E@G@F@=B<B;BaB`B_BiChCgCMDLDKD1E0EoD9E7E<F9F6FaE^E[EjEgEdER0O0L0I0F0C0n0l0\\0Z0X0@1>1<1Z2X2V2Y3W3U3X4V4T4E5C5D5n6o607R7S7T7Z8[8\\8>9?9@9b9c9d9l9m9e:h:k:J:M:P:S:V:Y:[;^;a;R;U;X;6<8<:<;<h;j;l;L<N<P<f=h=j=e>g>i>d?f?h?N@Q@H@@B?B>BdBcBbBlCkCjCPDODND4E3E2E;E:E8E6E;F8F5F`E]EZEiEfEcEQ0N0K0H0E0B0m0k0j0Y0W0U0=1;191W2U2S2V3T3R3U4S4Q4B5@5A5k6l6m6O7P7Q7W8X8Y8;9<9=9_9`9a9j9k9\\:_:f:i:l:K:N:Q:T:W:Z:\\;_;b;S;V;Y;C<E<G<<<m;k;1<2<O<Q<S<i=[=P=h>X>Z>g?M@O@R@U@S@J@I@AB1B0BeBTB\\CmCAD@DQDiDhD5EdD<E4F2F0F=F:F7FbE_E\\EkEhEeES0P0M0J0G0D011o0h0g0e0V0T0`0:181Q2T2R2H2S3Q3P3R4P4K3?5=5>5c6i6j6h6M7N7L7U8V8T899:9W9]9^9\\9g9h9i9]:`:c:5;2;o:>;;;8;P;M;J;G;D;A;?<A<D<F<=<><n;o;3<5<R<T<Y=Z=Q=R=Y>[>\\>^>P@T@L@K@5B4B3B2BVBUB^C]CCDBDkDjDfDeD>E=E3F1FnElE@FCFFFIFLFOF;0>0A0205080513101i0f0d0c0b0_0I1H1D1P2O2G2F2D2O3N3M3J3H3a6b6e6f6g6I7J7K7R8S8397989V9Y9Z9[9f9^:a:d:4;1;n:=;:;7;O;L;I;F;C;@;@<B<0<4<U<V<X<`=]=\\=S=U=W=]>_>`>8B7B6BZBXBWB_CaC`CFDEDDDlDgDoEmE>FAFDFGFJFMF90<0?0003060714121a0^0]0G1C1B1N2L2E2C2B2@2L3I3`6d6E7F7G7H7O8Q8192969T9U9X96;3;0;?;<;9;Q;N;K;H;E;B;W<Y<^=_=a=T=V=X=\\B[BYBdCcCbCHDGD?FBFEFHFKFNF:0=0@0104070F1E1A1M2K2J2I2A2D7L8M8N8P809495961b:";
var map = new Array(1440);

function $(id) { return document.getElementById(id); }
function $new(name) { return document.createElement(name); }
function $txt(txt) { return document.createTextNode(txt); }
function trim(str) { return str.replace(/^\s\s*/, "").replace(/\s*\s$/, ""); }
function valid(str) { if (!str) return false; if (str.length==0) return false; return true;}
function isSliding() { var z = $("body").visiblePage/*getAttribute("data-visible")*/; return $("table"+z) ? $("table"+z).offsetLeft!=0 : false; }
function htmlDecode(input) { var e = $new('div'); e.innerHTML = input; return e.firstChild ? e.firstChild.nodeValue : input; }
function setUTC(el, time) { var str = time.toUTCString(); var utc = str.substr(str.length-12, 8); el.innerHTML = "&#8226;&nbsp;"+utc+"&nbsp;UTC&nbsp;&#8226;"; }

function cycleCol(el)
{
    var col = el.dotColor;//el.getAttribute("data-color");
    col++;
    col %= 31;
    el.dotColor = col; //setAttribute("data-color", col);
    if (col>16)
        col = 31-col;
    var hex = col.toString(16);
    el.style.color = "#"+hex+"0"+hex+"0"+hex+"f";
}

function onload()
{
    try
    {
        var xmlLoad = new XMLHttpRequest();
        xmlLoad.open('POST', "index.php?load", true);
        xmlLoad.setRequestHeader("Cache-Control", "no-cache");
        xmlLoad.setRequestHeader("If-Match", "*");
        xmlLoad.onload = function()
        {
            if (xmlLoad.status==401)
                login("");

            if (xmlLoad.status!=200)
            {
                //alert("ERROR[0] - HTTP request '"+xmlLoad.statusText+" ["+xmlLoad.status+"]");
                //return;
            }

            if (xmlLoad.status==200)
                login(xmlLoad.responseText);
        };
        xmlLoad.send(null);
    }
    catch(e)
    {
        // FIXME: Add a message to the body.
        alert("Your browser doesn't support dynamic reload.");
        return;
    }

    var name = location.hash.length==0 ? "fact" : location.hash.substr(1);

    var args = location.search.substr(1).split('&');

    for (var i=0; i<args.length; i++)
    {
        switch (args[i])
        {
        //case "max":     $("body").setAttribute("data-max",     "yes"); continue;
        //case "noslide": $("body").setAttribute("data-noslide", "yes"); continue;
        case "max":     $("body").displayMax     = true; continue;
        case "noslide": $("body").displayNoslide = true; continue;
        case "sound":   $("body").sound          = true; continue;
        }

        var entry = args[i].split('=');
        if (entry.length!=2)
            continue;

        switch (entry[0])
        {
        case "w": $("body").displayFixedWidth  = entry[1]; break; //setAttribute("data-width",  entry[1]); break;
        case "h": $("body").displayFixedHeight = entry[1]; break; //setAttribute("data-height", entry[1]); break;
        }
    }

    /*
     alert("0 -- "+navigator.appCodeName+"\n"+
          "1 -- "+navigator.appName+"\n"+
          "2 -- "+navigator.appVersion+"\n"+
          "3 -- "+navigator.platform+"\n"+
          "4 -- "+navigator.userAgent);
          */
    loadPage(name, 0, 0);
}

function login(user)
{
    var z = $("body").visiblePage;
    var l = $("login"+z);

    $("body").user = user;

    if (l)
    {
        l.setAttribute("style", "background-position:-"+(user?"720":"755")+"px 50%;");
        l.alt = user;
    }
}

function onresize()
{
    var z = $("body").visiblePage/*getAttribute("data-visible")*/;

    //$("table"+z).style.width="100%";
    $("image"+z).style.width="1px";
    $("canvas"+z).width=1;

    doresize(z);

}

function loadPage(name, z, dz)
{
    if (isSliding())
        return;

    var xmlPage = new XMLHttpRequest();
    xmlPage.open('GET', "struct/"+name+'.page', true);
    xmlPage.setRequestHeader("Cache-Control", "no-cache");
    xmlPage.setRequestHeader("If-Match", "*");
    xmlPage.onload = function ()
    {
        if (xmlPage.status!=200)
        {
            alert("ERROR[0] - HTTP request '"+name+".page': "+xmlPage.statusText+" ["+xmlPage.status+"]");
            //setTimeout("loadPage('+name+')", 5000);
            /****** invalidate ******/
            return;
        }

        if (!isSliding())
        {
            buildPage(name, xmlPage.responseText, z, dz);
            changePage(z, z+dz);
        }

        //changePage(name, xmlHttp.resposeText);
        //slideOut(name, xmlHttp.responseText);
        //displayPage(name, xmlHttp.responseText);
        //onresize(true);
    };

    xmlPage.send(null);

    location.hash = name;
}

function sendCommand(command)
{
    if (command=="stop")
    {
        if (!confirm("Do you really want to stop a running script?"))
            return;
    }

    var debug = false;

    var uri = "index.php?";
    if (debug==true)
        uri += "debug&";
    uri += command;

    var xmlCmd = new XMLHttpRequest();
    xmlCmd.open('POST', uri, true);
    xmlCmd.setRequestHeader("Cache-Control", "no-cache");
    xmlCmd.setRequestHeader("If-Match", "*");
    xmlCmd.onload = function ()
    {
        if (xmlCmd.status==401)
            login("");

        if (xmlCmd.status!=200)
        {
            alert("ERROR[1] - HTTP request: "+xmlCmd.statusText+" ["+xmlCmd.status+"]");
            return;
        }

        if (xmlCmd.responseText.length==0)
        {
            alert("No proper acknowledgment of command execution received.");
            return;
        }

        var txt = xmlCmd.responseText.split('\n');
        login(txt[0]);
        if (txt.length>1)
            alert(xmlCmd.responseText);
        else
            alert("Command submitted.");
    };
    xmlCmd.send(null);
}


function submit(script, isIrq)
{
    var inputs = document.getElementsByTagName("input");

    var args = isIrq ? "interrupt="+script : "start="+script+".js";

    for (var i=0; i<inputs.length; i++)
        args += "&"+inputs[i].name+"="+inputs[i].value;

    var selects = document.getElementsByTagName("select");
    for (var i=0; i<selects.length; i++)
        args += "&"+selects[i].name+"="+selects[i].value;

    sendCommand(args);
}

function buildPage(name, text, oldz, dz)
{
    var fname = dz==0 ? "fact" : $("table"+oldz).pageName;//getAttribute("data-file");

    var z = oldz + dz;

    var lines = text.split('\n');

    if (lines.length==0)
    {
        alert("buildPage - received data empty.");
        return;
    }

    if (lines[0].length==0)
    {
        alert("buildPage - title missing");
        return;
    }

    $("audio").date = new Date();

    var title  = lines[0];
    var is_cmd = title[0]=='*' || title[0]=='!';
    var is_irq = title[0]=='!';
    var script = title.split('|');
    if (is_cmd)
    {
        title = script.length>=1 ? script[0].substr(1) : title.substr(1);
        script = script.length>=1 ? script[1] : name;
    }

    // ==================================================================

    var th = $new("thead");
    th.colSpan = 3;
    th.width = "100%";

    var htr = $new("tr");
    th.appendChild(htr);

    var htd = $new("td");
    htd.setAttribute("class", "thead");
    htd.colSpan = 3;
    htd.width = "100%";
    htr.appendChild(htd);

    // -------------

    var htab = $new("table");
    htab.width = "100%";
    htd.appendChild(htab);

    var hhtr = $new("tr");
    htab.appendChild(hhtr);

    var htd0 = $new("td");
    var htd1 = $new("td");
    var htd2 = $new("td");
    var htd3 = $new("td");
    var htd4 = $new("td");
    var htd5 = $new("td");
    var htd6 = $new("td");
    htd0.setAttribute("class", "tcell1");
    htd1.setAttribute("class", "tcell2");
    htd2.setAttribute("class", "tcell1");
    htd2.setAttribute("width", "1px");
    htd3.setAttribute("class", "tcell1");
    htd3.setAttribute("width", "1px");
    htd4.setAttribute("width", "1px");
    htd5.setAttribute("width", "1px");
    htd6.setAttribute("width", "1px");
    hhtr.appendChild(htd6);
    hhtr.appendChild(htd4);
    hhtr.appendChild(htd3);
    hhtr.appendChild(htd0);
    hhtr.appendChild(htd1);
    hhtr.appendChild(htd2);
    hhtr.appendChild(htd5);

    var div0 = $new("div");
    var div1 = $new("div");
    var div2 = $new("div");
    var div3 = $new("div");
    var div4 = $new("div");
    var div5 = $new("div");
    div0.id = "login"+z;
    div0.alt = $("body").user;
    div4.id = "warn"+z;
    div5.id = "speaker"+z;
    div0.setAttribute("class", "icon_login");
    div2.setAttribute("class", "icon_white");
    div4.setAttribute("class", "icon_color");
    div5.setAttribute("class", "icon_color");
    div0.setAttribute("style", "background-position:-"+($("body").user?"720":"755")+"px 50%;");
    div2.setAttribute("style", "background-position:-396px 50%;");
    div4.setAttribute("style", "background-position:-12px -13px;display:none;");
    div5.setAttribute("style", "background-position:-189px -57px;");
    div0.onclick = function () { sendCommand("logout"); };
    div2.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage(fname,   z, -dz); };
    div4.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage('error', z,  +1); };

    if (name=="fact")
    {
        div3.setAttribute("class", "icon_color");
        div3.setAttribute("style", "background-position:-58px -146px;");
    }
    else
    {
        div3.setAttribute("class", "icon_white");
        div3.setAttribute("style", "background-position:-575px 50%;");
        div3.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage('fact',  z,  -1); };
    }

    var sp0 = $new("span");
    var sp1 = $new("span");
    var sp2 = $new("span");
    sp0.id = "ldot" +z;
    sp1.id = "title"+z;
    sp2.id = "rdot" +z;
    sp1.setAttribute("style", "font-size:large;");
    //sp0.setAttribute("data-color", "3");
    //sp2.setAttribute("data-color", "3");
    sp0.dotColor = 3;
    sp2.dotColor = 3;
    sp0.appendChild($txt(" \u2022 "));
    sp1.appendChild($txt(title));
    sp2.appendChild($txt(" \u2022 "));
    if (is_cmd)
    {
        sp1.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; submit(script, is_irq); this.style.backgroundColor=''; };
    }
    else
    {
        if (name!='control')
            sp1.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.15)'; loadPage('control', z, +1); };
    }

    div1.setAttribute("style", "font-size:small;");
    div1.id = "reporttime"+z;
    div1.appendChild($txt("---"));

    div1.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; sendCommand('stop'); this.style.backgroundColor=''; };

    htd0.appendChild(sp0);
    htd0.appendChild(sp1);
    htd0.appendChild(sp2);

    htd6.appendChild(div0);     // Login
    htd1.appendChild(div1);
    if (dz!=0/* && z+dz!=0*/)
        htd2.appendChild(div2); // back
    htd3.appendChild(div3);     // home
    htd4.appendChild(div4);     // Warning
    htd5.appendChild(div5);     // Speaker

    // ==================================================================

    var tf = $new("tfoot");

    var ftr = $new("tr");
    tf.appendChild(ftr);

    var ftd = $new("td");
    ftd.setAttribute("class",   "tfoot");
    ftd.width = "100%";
    ftd.colSpan = 3;
    ftr.appendChild(ftd);

    var ftab = $new("table");
    ftab.width = "100%";
    ftd.appendChild(ftab);

    var ftdH = $new("td");
    var ftd0 = $new("td");
    var ftd1 = $new("td");
    var ftd2 = $new("td");
    var ftd3 = $new("td");
    var ftd4 = $new("td");
    ftdH.setAttribute("width", "1px");
    ftd2.setAttribute("width", "1px");
    ftd3.setAttribute("width", "1px");
    ftd4.setAttribute("width", "1px");

    ftdH.setAttribute("class", "tcell1");
    ftd0.setAttribute("class", "tcell1");
    ftd1.setAttribute("class", "tcell2");
    ftd2.setAttribute("class", "tcell2");
    ftd3.setAttribute("class", "tcell2");
    ftd4.setAttribute("class", "tcell2");

    ftab.appendChild(ftdH);
    ftab.appendChild(ftd0);
    ftab.appendChild(ftd1);
    ftab.appendChild(ftd2);
    ftab.appendChild(ftd3);
    ftab.appendChild(ftd4);

    var fdivH = $new("div");
    var fdiv0 = $new("span");
    var fdiv1 = $new("span");
    var fdiv2 = $new("div");
    var fdiv3 = $new("div");
    var fdiv4 = $new("div");
    ftd0.style.paddingLeft = "5px";
    fdiv4.id="cmd"+z;

    fdiv2.setAttribute("class", "icon_white");
    fdiv3.setAttribute("class", "icon_white");
    fdiv4.setAttribute("class", "icon_white");
    fdiv2.setAttribute("style", "background-position:-72px 50%;");
    fdiv4.setAttribute("style", "background-position:-432px 50%;");
    fdiv2.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; sendCommand('stop'); this.style.backgroundColor=''; };
    if (is_cmd)
    {
        fdiv3.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; submit(script, is_irq); this.style.backgroundColor=''; };
        fdiv3.setAttribute("style", "background-position:-109px 50%;");
    }
    else
    {
        fdiv3.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage('control', z,  +1); };
        fdiv3.setAttribute("style", "background-position:-289px 50%;");
    }

    if (name.substr(0, 5)=="help-")
    {
        fdivH.setAttribute("class", "icon_color");
        fdivH.setAttribute("style", "background-position:-408px -57px;");
        //fdivH.setAttribute("style", "background-position:-13px -57px;");
    }
    else
    {
        fdivH.setAttribute("class", "icon_white");
        fdivH.setAttribute("style", "background-position:-611px 50%;");
        fdivH.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage('help-'+name, z, +1); };
    }
    fdiv4.onclick = function () { this.style.backgroundColor='rgba(0,0,0,0.77)'; loadPage('scriptlog', z,  +1); };


    fdiv0.setAttribute("style", "font-size:large;");
    fdiv1.setAttribute("style", "font-size:small;");
    fdiv1.id = "localtime"+z;

    fdiv0.onclick = function () { window.location='https://www.fact-project.org/logbook/misc.php?action=mobile'; }
    fdiv0.appendChild($txt("logbook"));
    fdiv1.appendChild($txt("loading..."));

    ftdH.appendChild(fdivH);
    ftd0.appendChild(fdiv0);
    ftd1.appendChild(fdiv1);
    ftd2.appendChild(fdiv2);
    if (is_cmd || name!='control')
        ftd3.appendChild(fdiv3);
    if (name!='scriptlog')
        ftd4.appendChild(fdiv4);

    // ==================================================================

    var tbody = $new("tbody");

    for (var i=0; i<lines.length; i++)
    {
        lines[i] = trim(lines[i]);

        if (lines[i][0] == '#')
            lines.splice(i--, 1);
    }

    // Concatenate consecutive lines until they have at least two colons
    for (var i=2; i<lines.length; i++)
    {
        if (lines[i].length==0)
            continue;

        while (i<lines.length)
        {
            var cols = lines[i-1].split('|');
            if (cols.length>=3)
                break;

            lines[i-1] += lines[i].length==0 ? '<p/>' : " "+lines[i];
            lines.splice(i,1);
        }
    }

    var counter = 1;
    for (var i=1; i<lines.length; i++)
    {
        lines[i] = trim(lines[i]);

        if (lines[i].length==0)
            continue;

        var cols = lines[i].split('|');
        if (cols.length != 3 && cols.length !=4)
        {
            alert("Wrong number of columns in line #"+i+" in '"+name+"': '"+lines[i]+"' N(cols)="+cols.length);
            continue;
        }

        var check = cols[1].split("=");

        if (check.length>1 && (check[0]=="camera" || check[0]=="hist"))
        {
            var data = cols[1].substring(check[0].length+1).split("/");

            var tr = $new("tr");
            tr.setAttribute("class", "row");
            //tr.setAttribute("style", "margin:0;padding:0;");

            var td = $new("td");
            td.setAttribute("class", "container");
            td.id = "container";
            td.colSpan = 3;
            tr.appendChild(td);

            var canv = $new("canvas");
            canv.id = "canvas"+z;
            canv.width = "1";
            canv.height = "1";
            //canv.onclick = function () { save(); }
            //canv.setAttribute("data-type", check[0]);
            //canv.setAttribute("data-file", data[0]);
            //canv.setAttribute("data-data", cols[1].substring(check[0].length+data[0].length+2));
            canv.dataType = check[0];
            canv.fileName = data[0];
            canv.dataUnit = htmlDecode(cols[1].substring(check[0].length+data[0].length+2));
//            canv.setAttribute("style", "display:none;");
            td.appendChild(canv);

            var img = $new("img");
            img.src = "img/dummy.png";//needed in firefox
            img.id = "image"+z;
            img.setAttribute("style", "width:1px;height:15px;display:none;");
            td.appendChild(img);

            tbody.appendChild(tr);
            continue;
        }

        var tr = $new("tr");
        tr.setAttribute("class", "row");

        if (valid(cols[0]))
        {
            tr.linkName = cols[0];
            tr.onclick = function () { this.style.background='#ccb'; loadPage(this.linkName, z, -1); };
        }

        if (valid(cols[3]))
        {
            tr.linkName = cols[3];
            tr.onclick = function () { this.style.background='#cbb'; loadPage(this.linkName, z, +1); };
        }

        var td0 = $new("td");
        td0.setAttribute("class", "tcol0");
        tr.appendChild(td0);

        if (check.length>0 && check[0]=="image")
        {
            var img = $new("img");
            img.style.width="100%";
            img.style.display="block";
            img.src = "img/"+check[1];
            td0.style.paddingLeft=0;
            td0.style.border=0;
            td0.colSpan=3;
            td0.appendChild(img);

            tbody.appendChild(tr);
            continue;
        }

        if (valid(cols[0]))
        {
            var sp = $new("div");
            sp.setAttribute("class", "icon_black");
            sp.setAttribute("style", "background-position: -144px 50%;");
            td0.appendChild(sp);
        }

        var td1 = $new("td");
        td1.setAttribute("class", "tcol1");
        td1.width = "100%";
        tr.appendChild(td1);

        var td2 = $new("td");
        td2.setAttribute("class", "tcol2");
        td2.width = "18px";

        if (valid(cols[3]))
        {
            var sp = $new("div");
            sp.setAttribute("class", "icon_black");
            sp.setAttribute("style", "background-position: -108px 50%;");
            td2.appendChild(sp);
        }
        tr.appendChild(td2);

        var tab = $new("table");
        tab.width = "100%";
        td1.appendChild(tab);

        var innertr = $new("tr");
        tab.appendChild(innertr);

        var cell1 = $new("td");
        cell1.setAttribute("class", "tcell1");

        var cell2 = $new("td");
        cell2.setAttribute("class", valid(cols[1]) ? "tcell2" : "tcell2l");

        if (check.length>0 && check[0]=="select")
        {
            var args = check[1].split('/');

            if (args.length<2)
                alert("Argument name missing for'"+check[1]+"'");
            else
            {
                var div = $new("div");
                div.innerHTML = args[0];
                cell1.appendChild(div);

                var input = $new("SELECT");
                input.name = args[1];
                for (var j=2; j<args.length; j++)
                    input.options.add(new Option(args[j]));
                cell2.appendChild(input);
            }

        }
        if (check.length>0 && check[0]=="input")
        {
            var opt = check[1].split('/');

            if (opt.length<2)
                alert("Argument name missing for'"+check[1]+"'");
            else
            {
                var div = $new("div");
                div.innerHTML = opt[0];
                cell1.appendChild(div);

                var input = $new("input");
                input.name = opt[1];
                input.type = "text";
                input.maxlength = 80;
                input.style.textAlign = "right";
                input.style.width = "100%";
                if (opt.length>2)
                    input.value=opt[2];

                cell2.appendChild(input);
            }
        }
        if (check.length>0 && check[0]=="checkbox")
        {
            var opt = check[1].split('/');

            var div = $new("div");
            div.innerHTML = opt[0];
            cell1.appendChild(div);

            var input = $new("input");
            input.type  = "checkbox";
            input.name  = opt[1];
            input.onclick = function() { this.value=this.checked; };
            var c = opt.length>2 && opt[2]=="1";
            input.checked = c;
            input.value = c;

            cell2.appendChild(input);
        }
        if (check.length==0 || (check[0]!="input" && check[0]!="select" && check[0]!="checkbox"))
        {
            var div = $new("div");
            div.innerHTML = cols[1];
            cell1.appendChild(div);

            if (cols.length>2 && cols[2].length>0)
            {
                cell2.id = "data"+z+"-"+counter;
                cell2.dataFormat = cols[2];
                cell2.appendChild($txt("---"));
                counter++;
            }
            else
                cell1.setAttribute("class", "description");
        }

        innertr.appendChild(cell1);
        innertr.appendChild(cell2);

        tbody.appendChild(tr);
    }

    // ==================================================================

    if (debug == true)
    {
        tr = $new("tr");
        tr.setAttribute("class", "row");

        td = $new("td");
        td.id = "debug"+z;
        td.colSpan = 3;
        tr.appendChild(td);

        tbody.appendChild(tr);
    }

    // ==================================================================

    var table = $("table"+z);
    if (table)
        $("body").removeChild(table);

    table = $new("table");
    table.id = "table"+z;
    table.border = 0;
    table.cellSpacing = 0;
    table.cellPadding = "0px";
    //table.setAttribute("style", "overflow:hidden;position:fixed;top:0px;left:"+window.innerWidth+"px;")
    table.setAttribute("style",
                       "position:fixed;width:100%;top:0px;"+
                       "left:"+window.innerWidth+"px;");

    table.appendChild(th);
    table.appendChild(tbody);
    table.appendChild(tf);

    $("body").appendChild(table);

    // ==================================================================

    /*
     // Scrollbar for just the body
     table.style.position  = "fixed";
     th.style.position     = "aboslute";
     tf.style.position     = "aboslute";
     tbody.style.overflowY = "auto";
     tbody.style.display   = "block";
     tbody.style.height    = (window.innerHeight-th.clientHeight-tf.clientHeight)+"px";
     tbody.id = "tbody"+z;
     th.id    = "thead"+z;
     tf.id    = "tfoot"+z;
     */

    // ==================================================================

    table.pageName = name;//setAttribute("data-file", name);
    table.counter = counter;

    // This is needed so that the page is extended in height
    // _before_ the sliding (in case it contains graphics
    doresize(z);
}

function doresize(z)
{
    var img  = $("image"+z);
    var canv = $("canvas"+z);
    if (!img || !canv)
        return;

    var h = $("table"+z).offsetHeight;
    if (h == 0)
        return;

    // ===========================================
    /*
     var tb = $("tbody"+z);
     var hw = $("thead"+z).clientHeight;
     var fw = $("tfoot"+z).clientHeight;

     tb.style.height = (window.innerHeight-hw-fw)+"px";
     */
    // ===========================================

    var fixedw = $("body").displayFixedWidth;//getAttribute("data-width");
    var fixedh = $("body").displayFixedHeight;//getAttribute("data-height");

    var W = fixedw>0 ? fixedw : window.innerWidth;
    var H = fixedh>0 ? fixedh : window.innerHeight;

    //var max = $("body").getAttribute("data-max")=="yes";
    var max = $("body").displayMax;

    var ih = max ? W : H - h + parseInt(img.style.height, 10);

    // This might create the scroll bar

    if (img.style.height!=ih+"px")
        img.style.height = ih+"px";
    if (canv.height!=ih)
        canv.height = ih;

    // now we can evaluate the correct view-port
    // (-2 is the border size of the parent element 'container')
    //var sW = (fixedw ? fixedw : $("table"+z).scrollWidth)-2;
    var sW = fixedw ? fixedw : canv.parentNode.clientWidth;

    if (img.style.width!=sW+"px")
        img.style.width = sW+"px";
    if (canv.width!=sW)
        canv.width = sW;

    // ------ debug -----
    if (debug == true)
    {
        $('debug'+z).innerHTML = "";
        $('debug'+z).innerHTML += "|W="+W +"/"+H;
        $('debug'+z).innerHTML += "|H="+h+"/"+$("table"+z).offsetHeight+"/"+img.offsetHeight;
        $('debug'+z).innerHTML += "|I="+img.style.height+"+"+H+"-"+h;
    }
}

var intervalSlide = null;

function changePage(oldz, newz)
{
    // No page displayed yet
    if (oldz==newz)
    {
        var tab = $("table"+newz);

        tab.style.left="0px";
        tab.style.position="absolute";

        $("body").visiblePage = newz; //.setAttribute("data-visible", newz);

        doresize(newz);

        //setInterval(refresh_text, 1000);
        //setInterval(refresh_graphics, 5000);

        refresh_text();

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
            alert("Pixel mapping table corrupted!");

        refresh_graphics();
        return;
    }

    var W = window.innerWidth;
    if (W==0 || $("body").displayNoslide)//$("body").getAttribute("data-noslide")=="yes")
    {
        $("body").visiblePage = newz;//setAttribute("data-visible", newz);
        $("body").removeChild($("table"+oldz));
        $("table"+newz).style.left="0px";
        return;
    }

    if (newz>oldz)
        $("table"+newz).style.left = W+"px";
    else
        $("table"+newz).style.left = (-W-1)+"px";

    $("body").visiblePage = newz;//setAttribute("data-visible", newz);

    // This is needed on my mobile to ensure that te browser
    // doesn't try to zoom during shifting
    $("table"+newz).style.position="fixed";
    $("table"+oldz).style.position="fixed";

    intervalSlide = setInterval(function (){doShift(oldz,newz);}, 75);
}

function doShift(oldz, newz)
{
    var t0 = $("table"+oldz);
    var t1 = $("table"+newz);

    if (t0.style.display=="none")
    {
        clearInterval(intervalSlide);
        $("body").removeChild(t0);

        t1.style.position="absolute";

        // Now the scroll bar might have to appear or disappear
        doresize(newz);
        return;
    }

    var x0 = t0.offsetLeft;
    var x1 = t1.offsetLeft;

    var W = window.innerWidth;

    if (newz<oldz)
    {
        x0 += W/5;
        x1 += W/5;
    }

    if (newz>oldz)
    {
        x0 -= W/5;
        x1 -= W/5;
    }

    if ((newz<oldz && x1>=0) || (newz>oldz && x1<=0))
    {
        t0.style.display="none";
        x1 = 0;
    }

    t0.style.left = x0+"px";
    t1.style.left = x1+"px";
}

var timeoutText = null;
var timeoutGraphics = null;

var test_counter = 0;

function refresh_text()
{
    var z=$("body").visiblePage;//getAttribute("data-visible");

    var fname = $("table"+z).pageName;//getAttribute("data-file");
    var counter = $("table"+z).counter;//getAttribute("data-file");

    var is_help = fname.substr(0,5)=="help-";

    // Is sliding, no file defined or just help text?
    if (isSliding() || !valid(fname) || is_help)
    {
        if (is_help)
        {
            setUTC($("localtime"+z), new Date());
            $("reporttime"+z).innerHTML="";
        }
            
        // invalidate?
        timeoutText = setTimeout(refresh_text, 1000);
        return;
    }
            
    var xmlText = new XMLHttpRequest();
    xmlText.open('GET', "data/"+fname+'.data', true);
    xmlText.setRequestHeader("Cache-Control", "no-cache");
    xmlText.setRequestHeader("If-Match", "*");
    xmlText.onload = function ()
    {
//        if (xmlText.status==412)
//        {
//            timeoutText = setTimeout(refresh_text, 3000);
//            return;
//        }

        if (counter>1 && xmlText.status!=200 && xmlText.status!=412)
        {
            alert("ERROR[2] - HTTP request '"+fname+".data': "+xmlText.statusText+" ["+xmlText.status+"]");
            timeoutText = setTimeout(refresh_text, 10000);
            return;
        }

        if (!isSliding())
        {
            cycleCol($("ldot"+z));
            update_text(fname, counter>1 ? xmlText.responseText : undefined);
            doresize(z); 
        }
        timeoutText = setTimeout(refresh_text, 3000);
    };
    xmlText.send(null);
}

var date0 = null;

var test = 0;
function update_text(fname, result)
{
    var z=$("body").visiblePage;//getAttribute("data-visible");
    var table = $("table"+z);

    if (table.pageName/*getAttribute("data-file")*/ != fname)
        return;

    // ----------------------------------------------------
    var now = new Date();

    var ltime = $("localtime"+z);
    setUTC(ltime, now);

    if (!result)
        return;

    var rtime = $("reporttime"+z);
    var tokens = result.split('\n');
    var header = tokens[0].split('\t');

    // File corrupted / should we remove the date?)
    if ((header.length>5 || header.length==2 || header.length==0) && header[0].length!=13)
    {
        // we ignore corrupted files for one minute
        if (date0==null || date0.getTime()+60000<now.getTime())
            rtime.style.color = "darkred";

        return;
    }

    // File OK
    date0 = now;

    var stamp = new Date();
    stamp.setTime(header[0]);

    // File older than 1min
    if (stamp.getTime()+60000<now.getTime())
        rtime.style.color = "darkred";
    else
        rtime.style.color = "";

    setUTC(rtime, stamp);

    $("warn"+z).style.display = header.length>=4 && header[3]=='1' ? "" : "none";

    if (header.length>=5)
        $("cmd"+z).style.backgroundColor = header[4]=='1' ? "darkgreen" : "darkred";

    // ----------------------------------------------------

    if (header.length>=3 && $("body").sound)
    {
        $("speaker"+z).style.display = "none";

        var audio = $("audio");

        var audio_date = new Date();
        audio_date.setTime(header[1]);

        // Time stamp of audio file must be newer than page load
        //  or last audio play respecitvely
        if (audio_date>audio.date && header[2].length>0)
        {
            var name = "audio/"+header[2];

            var mp3 = $new("SOURCE");
            var ogg = $new("SOURCE");
            mp3.src = name+".mp3";
            ogg.src = name+".ogg";
            mp3.type = "audio/mp3";
            ogg.type = "audio/ogg";

            audio.replaceChild(mp3, audio.firstChild);
            audio.replaceChild(ogg, audio.lastChild);

            audio.load();
            audio.play();

            audio.date = audio_date;
        }
    }

    // ----------------------------------------------------

    //var p = table.tBodies.length==3 ? 1 : 0;
    //var tbody = table.tBodies[p];

    for (var line=1; line<tokens.length; line++)
    {
        if (tokens[line].length==0)
            continue;

        var e = $("data"+z+"-"+line);
        if (!e)
            continue;

        var form = e.dataFormat;//getAttribute("data-form");
        if (!form)
            continue;

        var cols = tokens[line].split('\t');
        for (var col=1; col<cols.length; col++)
            form = form.replace("\$"+(col-1), cols[col].length==0 ? "&mdash;" : cols[col]);

        if (cols.length<=1)
            form = "&mdash;";

        form = form.replace(/<B#(.*?)>/g, "<b style='background:#$1'>");
        form = form.replace(/<#(.*?)>/g, "<font color='$1'>");
        form = form.replace(/<([\+-])>/g, "<font size='$11'>");
        form = form.replace(/<\/([#\+-])>/g, "</font>");
        form = form.replace(/([0-9][0-9]):([0-9][0-9]):([0-9][0-9])/g,
                            "<pre>$1</pre>:<pre>$2</pre>:<pre>$3</pre>");
        form = form.replace(/--:--:--/g, "<pre>  </pre> <pre>  </pre> <pre>  </pre>");

        var newe = $new("div");
        newe.innerHTML = form;
        e.replaceChild(newe, e.lastChild);

        e.parentNode.parentNode.parentNode.parentNode.style.background=cols[0];
    }
}

// http://billmill.org/static/canvastutorial/index.html
// http://www.netmagazine.com/tutorials/learning-basics-html5-canvas
// http://www.alistapart.com/articles/responsive-web-design/

function refresh_graphics()
{
    var z = $("body").visiblePage;//getAttribute("data-visible");

    var canvas = $("canvas"+z);

    // Is sliding or no data file defined?
    var fname = canvas==null ? "" : canvas.fileName;//getAttribute("data-file");
    if (isSliding() || !valid(fname))
    {
        // invalidate?
        timeoutGraphics = setTimeout(refresh_graphics, 3000);
        return;
    }

    var xmlGfx = new XMLHttpRequest();
    xmlGfx.open('GET', "data/"+fname, true);
    xmlGfx.setRequestHeader("Cache-Control", "no-cache");
    xmlGfx.setRequestHeader("If-Match", "*");
    xmlGfx.onload = function ()
    {
//        if (xmlGfx.status==412)
//        {
//            timeoutGraphics = setTimeout(refresh_graphics, 5000);
//            return;
//        }

        if (xmlGfx.status!=200 && xmlGfx.status!=412)
        {
            alert("ERROR[3] - Request '"+fname+"': "+xmlGfx.statusText+" ["+xmlGfx.status+"]");
            timeoutGraphics = setTimeout(refresh_graphics, 10000);
            //****** invalidate ******
            return;
        }

        if (!isSliding())
        {
            cycleCol($("rdot"+z));
            process_eventdata(xmlGfx.responseText);
        }
        timeoutGraphics = setTimeout(refresh_graphics, 5000)
    };
    xmlGfx.send(null);
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
    hue *= 14;

    var sr = hueToHex(20-hue);
    var sg = hueToHex(14-hue);
    var sb = hueToHex(26-hue);

    return sr+sg+sb;
}

function color(col)
{
    if (col==65533)
        return HLStoRGB(0);

    var hue = col/126;
    return HLStoRGB(hue);
}

function toHex(str, idx)
{
    var ch = str[idx].toString(16);
    return ch.length==2 ? ch : "0"+ch;
}

function drawHex(ctx, x, y, col)
{
    ctx.fillStyle = "#"+color(col);

    ctx.save();

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
}

function drawDisc(ctx, x, y, r, col)
{
    ctx.fillStyle = "#"+color(col);

    ctx.save();

    ctx.translate(x, y);

    ctx.beginPath();
    ctx.arc(0, 0, r, 0, Math.PI*2, true);
    ctx.fill();

    ctx.restore();
}

function beginDrawCam(scale)
{
    var z    = $("body").visiblePage;//getAttribute("data-visible");
    var canv = $("canvas"+z);

    var w = Math.min(canv.width/scale, canv.height/scale);

    var ctx = canv.getContext("2d");

    ctx.save();
    ctx.translate(canv.width/2, canv.height/2);
    ctx.scale(w*2, w*2);

    return ctx;
}

/**
 * @constructor
 */
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

function drawFullCam(data)
{
    if (data.length!=40 && data.length!=160 && data.length!=320 && data.length!=1440)
    {
        alert("Camera - Received data has invalid size ("+data.length+"b)");
        return;
    }

    var div = map.length/data.length;
    var off = data.length==320 ? 0.2 : 0;

    var ctx = beginDrawCam(83);
    // ctx.rotate(Math.PI/3);

    ctx.scale(1, Math.sqrt(3)/2);
    ctx.translate(-0.5, 0);

    drawHex(ctx, 0, 0, data.charCodeAt(parseInt(map[0]/div+off, 10)));

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

                var p = parseInt(map[cnt]/div+off, 10);

                drawHex(ctx, pos.x, pos.y, data.charCodeAt(p));
                cnt++;
            }
        }
    }

    drawHex(ctx, 7, -22, data.charCodeAt(parseInt(map[1438]/div+off, 10)));
    drawHex(ctx, 7,  22, data.charCodeAt(parseInt(map[1439]/div+off, 10)));

    ctx.restore();
}

function drawCam(data)
{
    var ctx = beginDrawCam(27);
    ctx.rotate(Math.PI/6);
    ctx.scale(1, Math.sqrt(3)/2);

    drawHex(ctx, 0, 0, data.charCodeAt(0));

    var cnt = 1;
    for (var ring=1; ring<=7; ring++)
    {
        for (var s=0; s<6; s++)
        {
            for (var i=1; i<=ring; i++)
            {
                var pos = new Position(s, ring, i);
                if (pos.d() > 44)
                    continue;

                if (ring==7)
                {
                    if (i==6 && (s==0 || s==3))
                        continue;
                    if (i==1 && (s==1 || s==4))
                        continue;
                }

                drawHex(ctx, pos.x, pos.y, data.charCodeAt(cnt++));
            }
        }
    }

    ctx.restore();
}

function drawCamLegend(canv, data)
{
    var unit = canv.dataUnit;//htmlDecode(canv.getAttribute("data-data"));

    var umin = data[1];
    var umax = data[2];

    var min  = data[3]+unit
    var med  = data[4]+unit;
    var max  = data[5]+unit;

    var v0 = parseFloat(umin);
    var v1 = parseFloat(umax);

    var diff = v1-v0;

    var cw = canv.width;
    //var ch = canv.height;

    var ctx = canv.getContext("2d");

    ctx.font         = "8pt Arial";
    ctx.textAlign    = "right";
    ctx.textBaseline = "top";

    for (var i=0; i<11; i++)
    {
        ctx.strokeStyle = "#"+color(126*i/10);
        ctx.strokeText((v0+diff*i/10).toPrecision(3)+unit, cw-5, 125-i*12);
    }

    var mw = Math.max(ctx.measureText(min).width,
                      ctx.measureText(med).width,
                      ctx.measureText(max).width);

    ctx.textBaseline = "top";
    ctx.strokeStyle  = "#000";

    ctx.strokeText(min, 5+mw, 5+24);
    ctx.strokeText(med, 5+mw, 5+12);
    ctx.strokeText(max, 5+mw, 5);
}

function drawGraph(canv, vals, data)
{
    var unit = canv.dataUnit;//htmlDecode(canv.getAttribute("data-data"));//.split("/");

    var umin = vals[1]+unit;
    var umax = vals[2]+unit;

    var stat = vals.length==4 ? vals[3] :
        vals[3]+unit+"   /   "+vals[4]+unit+"   /   "+vals[5]+unit;

    var cw = canv.width;
    var ch = canv.height;

    var ctx = canv.getContext("2d");

    var dw = 3;  // tick width
    var fs = 8;  // font size

    ctx.font      = fs+"pt Arial";
    ctx.textAlign = "right";

    var dim0 = ctx.measureText(umin);
    var dim1 = ctx.measureText(umax);

    var tw = Math.max(dim0.width, dim1.width)+dw+2;

    var ml = 5+tw; // margin left
    var mr = 10;   // margin right

    var mt = 5+2*fs+4; // margin top
    var mb = fs/2+4;   // margin bottom

    var nx = 20;
    var ny = 10;

    var w = cw-ml-mr;
    var h = ch-mt-mb;

    ctx.strokeStyle = "#666";
    ctx.fillStyle = "#"+color(100);

    // --- data ---
    var cnt = 0;
    for (var j=1; j<data.length; j++)
    {
        if (data[j].length<5)
            continue;

        ctx.strokeStyle = "#"+data[j].substr(0, 3);
        data[j] = data[j].substr(3);

        ctx.beginPath();
        ctx.moveTo(ml, ch-mb-data[j].charCodeAt(0)/126*h);
        for (var i=1; i<data[j].length; i++)
            ctx.lineTo(ml+w/(data[j].length-1)*i, ch-mb-data[j].charCodeAt(i)/126*h);

        // --- finalize data ---
        ctx.lineTo(cw-mr, ch-mb);
        ctx.lineTo(ml,    ch-mb);
        ctx.stroke();

        cnt++;
    }
    if (cnt==1)
        ctx.fill();

    ctx.beginPath();

    // --- grid ---

    ctx.strokeStyle = "#eee";

    for (var i=1; i<=nx; i++)
    {
        ctx.moveTo(ml+w*i/nx, ch-mb);
        ctx.lineTo(ml+w*i/nx,    mt);
    }
    for (var i=0; i<ny; i++)
    {
        ctx.moveTo(ml,   mt+h*i/ny);
        ctx.lineTo(ml+w, mt+h*i/ny);
    }
    ctx.stroke();
    ctx.closePath();
    ctx.beginPath();

    ctx.strokeStyle = "#000";

    // --- axes ---
    ctx.moveTo(ml,    mt);
    ctx.lineTo(ml,    ch-mb);
    ctx.lineTo(cw-mr, ch-mb);

    for (var i=1; i<=nx; i++)
    {
        ctx.moveTo(ml+w*i/nx, ch-mb-dw);
        ctx.lineTo(ml+w*i/nx, ch-mb+dw);
    }
    for (var i=0; i<ny; i++)
    {
        ctx.moveTo(ml-dw, mt+h*i/ny);
        ctx.lineTo(ml+dw, mt+h*i/ny);
    }
    ctx.stroke();
    ctx.closePath();

    ctx.textBaseline = "bottom";
    ctx.strokeText(umin, ml-dw-2, ch-1);

    ctx.textBaseline = mt>fs/2 ? "middle" : "top";
    ctx.strokeText(umax, ml-dw-2, mt);

    ctx.textBaseline = "top";
    ctx.textAlign    = "center";
    ctx.strokeText(stat, ml+w/2, 5);
}

function invalidateCanvas(canv)
{
    var ctx = canv.getContext("2d");

    ctx.fillStyle = "rgba(255, 255, 255, 0.5)";
    ctx.fillRect(0, 0, canv.width, canv.height);
}

function processGraphicsData(canv, result)
{
    if (result.length==0)
        return false;

    var ctx = canv.getContext("2d");
    ctx.clearRect(0, 0, canv.width, canv.height);

    var data = result.split('\x7f');
    if (data.length<2)
        return false;

    var header = data[0].split('\n');
    if (header.length<4)
        return false;

    switch (canv.dataType)
    {
        //case "camera": drawCam(result);     break;
    case "hist":
        drawGraph(canv, header, data);
        break;
    case "camera":
        drawFullCam(data[1]);
        drawCamLegend(canv, header);
        break;
    }

    var now = new Date();
    var tm  = new Date();
    tm.setTime(header[0]);

    if (tm.getTime()+60000<now.getTime())
        return false;

    //$("image"+z).src = canv.toDataURL("image/png");

    return true;
}

function process_eventdata(result)
{
    var z = $("body").visiblePage;//getAttribute("data-visible");
    var canv = $("canvas"+z);
    if (!canv)
        return;

    if (!processGraphicsData(canv, result))
        invalidateCanvas(canv);
}

function save()
{
    var z = $("body").visiblePage;//getAttribute("data-visible");

    var canvas = $("canvas"+z);
    var img    = canvas.toDataURL("image/png");

    img = img.replace("image/png", "image/octet-stream");

    document.location.href = img;
}

window['onload'] = onload;
