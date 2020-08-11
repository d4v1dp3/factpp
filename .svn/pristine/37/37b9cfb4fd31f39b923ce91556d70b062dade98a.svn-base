/**********************************************************************
*          Calendar JavaScript [DOM] v3.11 by Michael Loesler          *
************************************************************************
* Copyright (C) 2005-09 by Michael Loesler, http//derletztekick.com    *
*                                                                      *
*                                                                      *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 3 of the License, or    *
* (at your option) any later version.                                  *
*                                                                      *
* This program is distributed in the hope that it will be useful,      *
* but WITHOUT ANY WARRANTY; without even the implied warranty of       *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
* GNU General Public License for more details.                         *
*                                                                      *
* You should have received a copy of the GNU General Public License    *
* along with this program; if not, see <http://www.gnu.org/licenses/>  *
* or write to the                                                      *
* Free Software Foundation, Inc.,                                      *
* 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
*                                                                      *
 **********************************************************************/
/*
function logout()
{
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open('POST', "calendar.php?logout=1", true);
    xmlHttp.onload = function ()
    {
        if (xmlHttp.status!=200)
        {
            alert("ERROR - HTTP request: "+xmlHttp.statusText+" ["+xmlHttp.status+"]");
            return;
        }

        alert("Logout successful!");
    };

    xmlHttp.send();
}
*/
function resize()
{
    var table = document.getElementById("table");

    var W = window.innerWidth;
    var H = window.innerHeight;

    table.style.width =W+"px";
    table.style.height=H+"px";
}

var institutes= ["Shift", "Debug", "moon", "ETHZ", "ISDC", "TUDO", "UNIWUE" ];

if (window.location.href.includes("ToO"))
{
//    var institutes= [ "XMM-Mrk421", "INTEGRAL-Mrk421", "NuSTAR-Mrk421", "Swift-Mrk421", "XMM-Mrk501", "INTEGRAL-Mrk501", "NuSTAR-Mrk501", "Swift-Mrk501" ];
    var institutes= [ "XMM", "INTEGRAL", "NuSTAR", "Swift", "ASTROSAT", "MAGIC" ];
    document.title = "Visibilities for ToOs";
}

function CalendarJS()
{
    this.now       = new Date();
    this.dayname   = ["Mo","Tu","We","Th","Fr","Sa","So"];
    this.monthname = ["January","February","March","April","May","June","July","August","September","October","November","December"];
    this.tooltip   = ["previous month","next month","current date","last year","next year"];
    this.monthCell = document.createElement("th");
    this.tableHead = null;
    this.tableFoot = null;
    this.parEl     = null;

    this.init = function( id, initDate )
    {
        this.now       = initDate ? initDate : new Date();
        this.date      = this.now.getDate();
        this.month     = this.mm = this.now.getMonth();
        this.year      = this.yy = this.now.getFullYear();
        this.monthCell.appendChild(document.createTextNode( this.monthname[this.mm]+"\u00a0"+this.yy ));

        this.tableHead = this.createTableHead();
        this.tableFoot = this.createTableFoot();

        this.parEl = document.getElementById( id );
        this.show();

        if (!initDate)
            this.checkDate();
    };

    this.checkDate = function()
    {
        var self  = this;
        var today = new Date();

        if (this.date != today.getDate())
        {
            this.tableHead = this.createTableHead();
            this.tableFoot = this.createTableFoot();

            this.date = today.getDate();
            if (this.mm == this.month && this.yy == this.year)
                this.switchMonth("current");

            this.month = today.getMonth();
            if (this.mm == this.month && this.yy == this.year)
                this.switchMonth("current");

            this.year  = today.getFullYear();
            if (this.mm == this.month && this.yy == this.year)
                this.switchMonth("current");
        }
        window.setTimeout(function() { self.checkDate(); }, Math.abs(new Date(this.year, this.month, this.date, 24, 0, 0)-this.now));
    },

    this.removeElements = function( Obj )
    {
        while( Obj.childNodes.length > 0)
            Obj.removeChild(Obj.lastChild);

        return Obj;
    };

    this.show = function()
    {
        this.parEl = this.removeElements( this.parEl );
        this.monthCell.firstChild.replaceData(0, this.monthCell.firstChild.nodeValue.length, this.monthname[this.mm]+"\u00a0"+this.yy);

        var table = document.createElement("table");
        table.id = "table";

        this.parEl.appendChild( table );

        table.appendChild( this.tableHead );
        table.appendChild( this.tableFoot );

        table.appendChild( this.createTableBody(window.innerHeight-table.offsetHeight) );
 
        resize();
    };

    this.createTableFoot = function()
    {
        var tfoot = document.createElement("tfoot");

        var tr = document.createElement("tr");
        var td = document.createElement("td");
        td.height = "1%";
        td.colSpan = 7;
        td.style.padding="3px 3px";
        tfoot.appendChild(tr);
        tr.appendChild(td);
        var table = document.createElement("table");
        table.width="100%";
        td.appendChild(table);
        tr = document.createElement("tr");
        table.appendChild(tr);
        for (var i=0; i<institutes.length; i++)
        {
            td = document.createElement("td");
            td.width=100/institutes.length+"%";
            td.setAttribute("style", "text-align:center;font-size:1em;border:solid #112A5D 2px;padding:3px 3px;");
            td.changeUser = this.changeUser;
            td.onclick = function(e) { this.changeUser(); }
            td.appendChild(document.createTextNode(institutes[i]));
            tr.appendChild(td);

            if (i==0)
                td.style.backgroundColor = "yellow";
        }
        document.getElementById("body").setAttribute("data-user", institutes[0]);


        tr = document.createElement("tr");
        td = document.createElement("td");
        td.colSpan = 7;
        td.style.paddingLeft = "0px";
        td.style.paddingTop  = "0px";
        td.height = "1%";
        var form  = document.createElement("form");
        var input = document.createElement("textarea");
        input.overflow    = "auto";
        input.wrap        = "virtual";
        input.id          = "comment";
        input.value       = "enter comment here";
        input.style.color = "#888";
        input.style.width = "100%";
        input.rows        = 5;
        input.title       = "Enter a comment. Click somewhere in the calender to store the comment.";
        input.onchange    = function() { pushComment(); };
        input.onfocus     = function() { if (input.value=="enter comment here" && input.style.color!="black") input.value=""; input.style.color="black"; };
        input.onblur      = function() { input.style.color="#888"; if (input.value=="") input.value="enter comment here"; };
        form.appendChild(input);
        td.appendChild( form );
        tr.appendChild( td );
        tfoot.appendChild(tr);

        tr = document.createElement("tr");

        var td = document.createElement("td");
        td.height="1%";
        td.colSpan=7;
        tr.appendChild(td);

        var tab = document.createElement("table");
        var tr2 = document.createElement("tr");
        tab.width="100%";
        tab.cellSpacing=0;
        tab.cellPadding=0;
        tab.style.borderWidth = 0;
        tab.style.fontSize = "1.5em";
        tab.style.marginBottom = "2px";
        td.appendChild(tab);
        tab.appendChild(tr2);

        var tm = this.getCell( "td", this.timeTrigger(), "clock" );
        tm.style.whiteSpace="nowrap";
        tm.style.paddingLeft = "0px";
        tm.style.width="33%";
        tr2.appendChild( tm );

        var self = this;
        window.setInterval(function() { tm.firstChild.nodeValue = self.timeTrigger(); }, 500);

        var td = document.createElement("td");
        td.style.width="33%";
        td.style.textAlign="center";
        var a = document.createElement("a");
        a.href = "overview.png";
        a.style.whiteSpace="nowrap";
        a.appendChild(document.createTextNode("click here for help"));
        td.appendChild(a);
        tr2.appendChild( td );

        td = this.getCell( "td", "logout", "logout");
        td.style.width="33%";
        td.onclick = function(e) { logout(); }
        td.style.paddingRight = "0px";
        tr2.appendChild( td );

        tfoot.appendChild( tr );

        return tfoot;
    }

    this.createTableHead = function()
    {
        var thead = document.createElement("thead");
        thead.style.height="1%";
        var tr = document.createElement("tr");
        var th = this.getCell( "th", "\u00AB", "prev_month" );

        th.rowSpan = 2;
        th.Instanz = this;
        th.onclick = function() { this.Instanz.switchMonth("prev"); };
        th.title = this.tooltip[0];

        try { th.style.cursor = "pointer"; } catch(e){ th.style.cursor = "hand"; }
        tr.appendChild( th );

        this.monthCell.Instanz = this;
        this.monthCell.rowSpan = 2;
        this.monthCell.colSpan = 4;
        this.monthCell.onclick = function() { this.Instanz.switchMonth("current"); };
        this.monthCell.title = this.tooltip[2];

        try { this.monthCell.style.cursor = "pointer"; } catch(e){ this.monthCell.style.cursor = "hand"; }
        tr.appendChild( this.monthCell );

        th = this.getCell( "th", "\u00BB", "next_month" );
        th.rowSpan = 2;
        th.Instanz = this;
        th.onclick = function() { this.Instanz.switchMonth("next"); };
        th.title = this.tooltip[1];

        try { th.style.cursor = "pointer"; } catch(e){ th.style.cursor = "hand"; }
        tr.appendChild( th );

        th = this.getCell( "th", "\u02c4", "prev_year" );
        th.Instanz = this;
        th.onclick = function() { this.Instanz.switchMonth("prev_year"); };
        th.title = this.tooltip[3];

        try { th.style.cursor = "pointer"; } catch(e){ th.style.cursor = "hand"; }
        tr.appendChild( th );

        thead.appendChild( tr );

        tr = document.createElement("tr");
        th = this.getCell( "th", "\u02c5", "next_year" );
        th.Instanz = this;
        th.onclick = function() { this.Instanz.switchMonth("next_year"); };
        th.title = this.tooltip[4];

        try { th.style.cursor = "pointer"; } catch(e){ th.style.cursor = "hand"; }
        tr.appendChild( th );

        thead.appendChild( tr );

        tr = document.createElement('tr');
        for (var i=0; i<this.dayname.length; i++)
        {
            var th = this.getCell("th", this.dayname[i], "weekday" );
            th.width=100/7+"%";
            tr.appendChild( th );
        }

        thead.appendChild( tr );

        return thead;
    },

    this.createTableBody = function(height)
    {
        var dayspermonth = [31,28,31,30,31,30,31,31,30,31,30,31];
        var sevendaysaweek = 0;
        var begin = new Date(this.yy, this.mm, 1);
        var firstday = begin.getDay()-1;
        if (firstday < 0)
            firstday = 6;
        if ((this.yy%4==0) && ((this.yy%100!=0) || (this.yy%400==0)))
            dayspermonth[1] = 29;

        var tbody = document.createElement("tbody");
        var tr    = document.createElement('tr');

        tbody.height="100%";

        var height="";//"20%";//100/8+"%";

        if (firstday == 0)
        {
            for (var i=0; i<this.dayname.length; i++)
            {
                var prevMonth = (this.mm == 0)?11:this.mm-1;
                var td = this.getCell( "td", dayspermonth[prevMonth]-6+i, "last_month" );
                td.style.height=height;
                tr.appendChild( td );
            }
            tbody.appendChild( tr );
            tr = document.createElement('tr');
        }

        for (var i=0; i<firstday; i++, sevendaysaweek++)
        {
            var prevMonth = (this.mm == 0)?11:this.mm-1;
            var td = this.getCell( "td", dayspermonth[prevMonth]-firstday+i+1, "last_month" );
            td.style.height=height;
            tr.appendChild( td );
        }

        for (var i=1; i<=dayspermonth[this.mm]; i++, sevendaysaweek++)
        {
            if (this.dayname.length == sevendaysaweek)
            {
                tbody.appendChild( tr );
                tr = document.createElement('tr');
                sevendaysaweek = 0;
            }

            var td = null;
            if (i==this.date && this.mm==this.month && this.yy==this.year && (sevendaysaweek == 5 || sevendaysaweek == 6))
                td = this.getCell( "td", i, "today weekend" );
            else
                if (i==this.date && this.mm==this.month && this.yy==this.year)
                    td = this.getCell( "td", i, "today" );
                else
                    if (sevendaysaweek == 5 || sevendaysaweek == 6)
                        td = this.getCell( "td", i, "weekend" );
                    else
                        td = this.getCell( "td", i, null);

            td.setDate    = this.setDate;
            td.chooseDate = this.chooseDate;
            td.dd = i;
            td.mm = this.mm;
            td.yy = this.yy;
            td.id = this.mm+"-"+i;
            td.title = "Click to select this date.";

            td.style.height=height;

            td.onclick = function(e) {
                this.chooseDate();
            };

            var tab = document.createElement("table");
            tab.width="100%";
            tab.style.border = 0;
            tab.style.padding = 0;
            tab.style.margin = 0;
            tab.style.fontSize = "1.5em";
            tab.style.backgroundColor = "transparent";
            var tr0 = document.createElement("tr");
            var td0 = document.createElement("td");
            var td1 = document.createElement("td");
            td0.style.textAlign = "left";
            td1.style.textAlign = "right";
            td1.style.textWeight= "normal";
            td0.style.color = "lightgray";
            td0.style.border=0;
            td1.style.border=0;
            td0.style.padding=0;
            td1.style.padding=0;
            tab.appendChild(tr0);
            tr0.appendChild(td0);
            tr0.appendChild(td1);
            //td0.appendChild(document.createTextNode(txt));

            td1.appendChild(td.firstChild);
            td.appendChild(tab);

            var IP    = this.getMoonPhase(this.yy, this.mm, i);
            var str   = this.getMoonPhaseStr(IP);
            var phase = 100-Math.abs(IP-0.5)*200;
            var txt   = parseInt(phase+0.5,10)+"%";
            if (phase>50)
                td0.style.color = "gray";
            if (phase<3.4)
            {
                txt = "o";
                td0.style.textWeight = "bolder";
                td0.style.fontSize = "0.7em";
                td0.style.color = "darkgreen";
            }
            if (phase>96.6)
            {
                txt = "&bull;";
                td0.style.textWeight = "bolder";
                td0.style.fontSize = "0.8em";
                td0.style.color = "darkred";
            }
            tab.title = str;
            td0.innerHTML = txt;

            var sp = document.createElement("span");
            sp.appendChild(document.createTextNode("*"));
            sp.style.color="darkred";
            sp.style.display="none";
            td1.appendChild(sp);


            tr.appendChild( td );
        }

        var daysNextMonth = 1;
        for (var i=sevendaysaweek; i<this.dayname.length; i++)
            tr.appendChild( this.getCell( "td", daysNextMonth++, "next_month"  ) );

        tbody.appendChild( tr );

        while (tbody.getElementsByTagName("tr").length<6) {
            tr = document.createElement('tr');
            for (var i=0; i<this.dayname.length; i++)
            {
                var td = this.getCell( "td", daysNextMonth++, "next_month"  );
                td.style.height=height;
                tr.appendChild( td );
            }
            tbody.appendChild( tr );
        }

        requestAll(this.yy, this.mm);
        requestAllComments(this.yy, this.mm);
        if (this.year==this.yy && this.month==this.mm)
            requestComment(this.year, this.month, this.date);
        else
        {
            var c = document.getElementById("comment");
            c.color="#888";
            c.value="enter comment here";
        }

        return tbody;
    };

    this.getCalendarWeek = function(j,m,t)
    {
        var cwDate = this.now;
        if (!t)
        {
            j = cwDate.getFullYear();
            m = cwDate.getMonth();
            t = cwDate.getDate();
        }
        cwDate = new Date(j,m,t);

        var doDat = new Date(cwDate.getTime() + (3-((cwDate.getDay()+6) % 7)) * 86400000);
        cwYear = doDat.getFullYear();

        var doCW = new Date(new Date(cwYear,0,4).getTime() + (3-((new Date(cwYear,0,4).getDay()+6) % 7)) * 86400000);
        cw = Math.floor(1.5+(doDat.getTime()-doCW.getTime())/86400000/7);
        return cw;
    };

    function request(td)
    {
        var user = document.getElementById("body").getAttribute("data-user");
        var uri = "calendar.php?toggle&y="+td.yy+"&m="+td.mm+"&d="+td.dd;

        if (user!="Shift" && user!="Debug")
            uri += "&u="+user;

        uri += "&x="+(user=="Debug"?1:0);

        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open('POST', uri, true);
        xmlHttp.onload = function ()
        {
            if (xmlHttp.status!=200)
            {
                alert("ERROR - HTTP request: "+xmlHttp.statusText+" ["+xmlHttp.status+"]");
                return;
            }

            var lines = xmlHttp.responseText.split('\n');
            if (lines.length==0)
                return;

            while (td.childNodes.length>1)
                td.removeChild(td.lastChild);

            for (var i=0; i<lines.length; i++)
            {
                var x = lines[i].split('\t');
                if (x.length!=3)
                    continue;

                var div = document.createElement("div");
                div.style.fontWeight="normal";
                div.appendChild(document.createTextNode(x[2]=="1"?'('+x[1]+')':x[1]));
                td.appendChild(div);

                for (var j=0; j<institutes.length; j++)
                    if (x[1]==institutes[j])
                    {
                        div.className += " institute";
                        break;
                    }
            }

            if (td.childNodes.length>1)
                td.className += " enabled";
            else
                td.className = td.className.replace(/enabled/g, "");
        };

        xmlHttp.send();
    }

    function logout()
    {
        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open('POST', "calendar.php?logout", true);
        xmlHttp.onload = function ()
        {
            if (xmlHttp.status!=401)
            {
                alert("ERROR - HTTP request: "+xmlHttp.statusText+" ["+xmlHttp.status+"]");
                return;
            }

            alert(xmlHttp.statusText);
        };

        xmlHttp.send();
    }

    function pushComment()
    {
        var c = document.getElementById("comment");

        var y = c.getAttribute("data-y");
        var m = c.getAttribute("data-m");
        var d = c.getAttribute("data-d");
        var v = c.value;

        var uri = "calendar.php?y="+y+"&m="+m+"&d="+d+"&c="+encodeURIComponent(v);

        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open('POST', uri, true);
        xmlHttp.onload = function()
        {
            if (xmlHttp.status!=200)
            {
                alert("ERROR - HTTP request: "+xmlHttp.statusText+" ["+xmlHttp.status+"]");
                return;
            }

            alert("Comment inserted successfully.");

            var td = document.getElementById(m+"-"+d);
            var sp = td.firstChild.firstChild.lastChild.lastChild;
            if (v=="")
            {
                sp.style.display="none";
                td.title="Click to select this date.";
            }
            else
            {
                sp.style.display="";
                td.title=v;
            }

        };

        xmlHttp.send();
    }

    function requestComment(yy, mm, dd)
    {
        var c = document.getElementById("comment");

        var y = c.getAttribute("data-y");
        var m = c.getAttribute("data-m");
        var d = c.getAttribute("data-d");

        if (y==yy && m==mm && d==dd)
            return;

        var uri = "calendar.php?comment&y="+yy+"&m="+mm+"&d="+dd;
        var xmlHttp = new XMLHttpRequest();
        xmlHttp.open('POST', uri, true);
        xmlHttp.onload = function ()
        {
            if (xmlHttp.status!=200)
            {
                alert("ERROR - HTTP request: "+xmlHttp.statusText+" ["+xmlHttp.status+"]");
                return;
            }

            var td = document.getElementById(mm+"-"+dd);
            var sp = td.firstChild.firstChild.lastChild.lastChild;

            if (sp!=undefined)
            {
                c.color="#888";
                if (xmlHttp.responseText=="")
                {
                    c.value="enter comment here";
                    sp.style.display="none";
                    td.title="";
                }
                else
                {
                    c.value = xmlHttp.responseText;
                    sp.style.display="";
                    td.title=xmlHttp.responseText;
                }
            }

            c.setAttribute("data-y", yy);
            c.setAttribute("data-m", mm);
            c.setAttribute("data-d", dd);
        };

        xmlHttp.send();
    }

    var xmlReqAll = null;
    function requestAll(yy, mm)
    {
        if (xmlReqAll)
            xmlReqAll.abort();

        var uri = "calendar.php?y="+yy+"&m="+mm;

        xmlReqAll = new XMLHttpRequest();
        xmlReqAll.open('POST', uri, true);
        xmlReqAll.onload = function ()
        {
            if (xmlReqAll.status!=200)
            {
                alert("ERROR - HTTP request: "+xmlReqAll.statusText+" ["+xmlReqAll.status+"]");
                return;
            }

            var lines = xmlReqAll.responseText.split('\n');
            if (lines.length==0)
                return;

            for (var i=0; i<lines.length; i++)
            {
                var x = lines[i].split('\t');
                if (x.length!=3)
                    continue;

                var td = document.getElementById(mm+"-"+x[0]);

                var div = document.createElement("div");
                div.style.fontWeight="normal";
                div.appendChild(document.createTextNode(x[2]=="1"?'('+x[1]+')':x[1]));
                td.appendChild(div);

                for (var j=0; j<institutes.length; j++)
                    if (x[1]==institutes[j])
                    {
                        div.className += " institute";
                        break;
                    }

                td.className += " enabled";
            }
        };

        xmlReqAll.send();
    }

    var xmlReqCom = null;
    function requestAllComments(yy, mm)
    {
        if (xmlReqCom)
            xmlReqCom.abort();

        var uri = "calendar.php?comment&y="+yy+"&m="+mm;
        xmlReqCom = new XMLHttpRequest();
        xmlReqCom.open('POST', uri, true);
        xmlReqCom.onload = function ()
        {
            if (xmlReqCom.status!=200)
            {
                alert("ERROR - HTTP request: "+xmlReqCom.statusText+" ["+xmlReqCom.status+"]");
                return;
            }

            if (xmlReqCom.responseText<4)
                return;

            var pos = 6;

            while (pos<xmlReqCom.responseText.length)
            {
                var len = parseInt(xmlReqCom.responseText.substr(pos-6, 4), 10);
                var dd  = parseInt(xmlReqCom.responseText.substr(pos-2, 2), 10);
                var com = xmlReqCom.responseText.substr(pos, len);
                pos += len+6;

                if (com!="")
                {
                    var td = document.getElementById(mm+"-"+dd);
                    var sp = td.firstChild.firstChild.lastChild.lastChild;
                    sp.style.display="";
                    td.title=com;
                }
            }
        };

        xmlReqCom.send();
    }

    this.setDate = function()
    {
        request(this);
    };

    this.changeUser = function()
    {
        var sib = this.nextSibling;
        while (sib)
        {
            sib.style.backgroundColor = "";
            sib = sib.nextSibling;
        }

        sib = this.previousSibling;
        while (sib)
        {
            sib.style.backgroundColor = "";
            sib = sib.previousSibling;
        }

        this.style.backgroundColor = "yellow";

        document.getElementById("body").setAttribute("data-user", this.firstChild.textContent);
    };

    this.chooseDate = function()
    {
        while (document.getElementsByClassName("choosen")[0])
        {
            var e = document.getElementsByClassName("choosen")[0];
            e.title = "Click to select this date.";
            e.className = e.className.replace(/choosen/g, "");
            e.onclick = function() {
                this.chooseDate();
            };
        }

        this.className += " choosen";
        this.title = "Click again to add or remove your name.";

        requestComment(this.yy, this.mm, this.dd);

        this.onclick = function() {
            this.setDate();
        };
    };

    this.timeTrigger = function()
    {
        var now = new Date();
        var ss  = (now.getSeconds()<10)?"0"+now.getSeconds():now.getSeconds();
        var mm  = (now.getMinutes()<10)?"0"+now.getMinutes():now.getMinutes();
        var hh  = (now.getHours()  <10)?"0"+now.getHours()  :now.getHours();

        var kw = "KW" + this.getCalendarWeek(this.year, this.month, this.date);
        var str = hh+":"+mm+":"+ss+"\u00a0["+kw+"]";
        return str;
    };

    this.getCell = function(tag, str, cssClass)
    {
        var El = document.createElement( tag );
        El.appendChild(document.createTextNode( str ));
        if (cssClass != null)
            El.className = cssClass;
        return El;
    },

    this.switchMonth = function( s )
    {
        switch (s)
        {
        case "prev":
            this.yy = (this.mm == 0) ? this.yy-1 : this.yy;
            this.mm = (this.mm == 0) ? 11        : this.mm-1;
            break;

        case "next":
            this.yy = (this.mm == 11) ? this.yy+1 : this.yy;
            this.mm = (this.mm == 11) ? 0         : this.mm+1;
            break;

        case "prev_year":
            this.yy = this.yy-1;
            break;

        case "next_year":
            this.yy = this.yy+1;
            break;

        case "current":
            this.yy = this.year;
            this.mm = this.month;
            break;
        }
        this.show();
    }

    this.getMoonPhase = function(Y, M, D)
    {
        // M=0..11 --> 1..12
        M += 1;

        // calculate the Julian date at 12h UT
        var YY = Y - Math.floor( ( 12 - M ) / 10 );
        var MM = ( M + 9 ) % 12;

        var K1 = Math.floor( 365.25 * ( YY + 4712 ) );
        var K2 = Math.floor( 30.6 * MM + 0.5 );
        var K3 = Math.floor( Math.floor( ( YY / 100 ) + 49 ) * 0.75 ) - 38;

        var JD = K1 + K2 + D + 59;  // for dates in Julian calendar
        if ( JD > 2299160 )         // for Gregorian calendar
            JD = JD - K3;

        // calculate moon's age in days
        var IP = ( ( JD - 2451550.1 ) / 29.530588853 ) % 1;

        return IP;

        // Moon's age
        //var AG = IP*29.53;
    }

    this.getMoonPhaseStr = function(IP)
    {
        var phase = " ("+(100-Math.abs(IP-0.5)*200).toPrecision(2)+"%)";

        if (IP*16 < 1) return "New moon" + phase;
        if (IP*16 < 3) return "Evening crescent" + phase;
        if (IP*16 < 5) return "First quarter" + phase;
        if (IP*16 < 7) return "Waxing gibbous" + phase;
        if (IP*16 < 9) return "Full moon" + phase;
        if (IP*16 <11) return "Waning gibbous" + phase;
        if (IP*16 <13) return "Last quarter" + phase;
        if (IP*16 <15) return "Morning crescent" + phase;

        return "New moon"+phase;
    }
}

var DOMContentLoaded = false;
function addContentLoadListener (func)
{
    if (document.addEventListener)
    {
        var DOMContentLoadFunction = function ()
        {
            window.DOMContentLoaded = true;
            func();
        };

        document.addEventListener("DOMContentLoaded", DOMContentLoadFunction, false);
    }

    var oldfunc = (window.onload || new Function());

    window.onload = function ()
    {
        if (!window.DOMContentLoaded)
        {
            oldfunc();
            func();
        }
    };
}

addContentLoadListener( function() {
new CalendarJS().init("calendar");
//new CalendarJS().init("calendar", new Date(2009, 1, 15));
} );