/** @mainpage

@brief FACT++ - The FACT slow control software

@author thomas.bretz@phys.ethz.ch et al.
<!--@version 1.0-->

<hr width="100%">

@section toc Table of contents
<table border='1' bgcolor=#FAFAFA width='100%'>
<tr>
<td>
<ul>
<li> @ref install_sec
    <ul>
    <li> @ref rootwarning
    <li> @ref packages
    <li> @ref installroot
    </ul>
<li> @ref demos
<li> @ref dimremarks
<li> @ref addtab
<li> @ref Documentation
<li> @ref References
    <ul>
    <li> @ref generalref 
    <li> @ref boostref
    <li> @ref fitsref
    <li> @ref qtroot
    </ul>
<li> @ref availableprograms
<li> @ref Examples
</ul>
</tr>
</td>
</table>

@section install_sec Installation

FACT++ can be downloaded from the svn by

\verbatim
   svn checkout https://www.fact-project.org/svn/trunk/FACT++ [localdir]
\endverbatim

it includes a dim version which is automatically compiled.

Installation instructions can be found at
- <A HREF="https://trac.fact-project.org/wiki/InstallingFACT++">TRAC:InstallingFACT++</A>.

WARNING - Some infomration here migzt be outdated. Please refer to the
TRAC for up-to-date information.


@subsection packages Required packages

The following section gives a list of packages which were necessary after
a fresh Ubuntu 11.04 installation. In addition to all the development
packages the corresponding package with the library is needed.

Note that a recent C++ compiler is needed supporting the latest C++0x 
standard.

<i>Required (configure will fail without them)</i>
- subversion
- gcc 
- g++ 
- make 
- libreadline6-dev 
- libboost-all-dev 
- libx11-dev (needed for lesstif, qt4, root)

<i>FITS file support (datalogger, event builder)</i>
- libccfits-dev 

<i>MySQL support (command line options, scheduler)</i>
- libmysqlclient-dev (optional for MySQL support)
- libmysql++-dev (option for MySQL support)

<i>If you want 'did'</i>
- lesstif2-dev

<i>For JavaScrip support</i>
- libv8-dev

<i>To compile the GUIs</i>
- libqt4-dev
- root (see section about root, currently recommended versions 5.18/00b-5.26/00e)

<i>To compile the raw data viewer</i>
- libglu1-mesa-dev

<i>To compile smartfact with astronomy support and moon</i>
- libnova-dev

<i>To compile tngweather</i>
- libsoprano-dev

<i>To compile skypeclient</i>
- libdbus-1-dev
- libdbus-glib-1-dev

<i>To create your own documentation</i>
- graphviz
- doxygen
- help2man
- groff 
- ps2pdf

<i>To create JavaScript documentation</i>
- jsdoc-toolkit

<i>For developers</i>
- autoconf
- autoconf-archive
- libtool
- qt4-designer

If you intend to change only Makefile.am but not configure.ac the \b automake
package instead of the \b autoconf package should be enough.

<i>Some nice to have (FACT++)</i>
- colorgcc
- colordiff

<i>Some nice to have (system)</i>
- fte
- efte
- htop

<i>Documentation (usually accessible through http://localhost/ for the tools above:</I>
- autoconf-doc
- gcc-doc
- graphviz-doc
- libboost-doc
- libmysql++-doc
- libtool-doc
- make-doc
- qt4-dev-tools [qt4-assistant]
- qt4-doc-html


<!--
VIEWER
libqwt5-qt4-dev
libqwt5-doc
-->

@subsection installroot How to install root 5.26/00 on Ubuntu 11.04 (natty)

- install gpp4.4, gcc4.4, g++4.4 (root does not compile with gcc4.5)
- make links to hidden X11 libraries:
<B><pre>
cd /usr/local
sudo ln -s x86_64-linux-gnu/libX* .
</pre></B>
- in the root source directory
<B><pre>
./configure --enable-qt --with-cc=gcc-4.4 --with-cxx=g++-4.4 --with-xrootd-opts=--syslibs=/usr/lib/x86_64-linux-gnu --prefix=/usr/local
</pre></B>
- \b make
- <b>sudo make install</b>
- pray
- don't forget to set LD_LIBRARY_PATH correctly before you try to start the fact gui


@section demos Current demonstration programs

- \b dserver2: A virtual board (A TCP/IP server). It is sending a
  "hello" message after accepting a communication and then in 3s
  intervals the current UTC time. The board can be set to state 1 or back
  to state 0 (just as a demonstration)
- \b dclient5: A control program. It accesses two viratual boards (start them
  with 'dserver2 5000' and 'dserver2 4001') If both boards are connected the START
  command can be issued to get them to state 'Running'. In this state
  an asynchronous time stamp can be requested sending the TIME command.
  to get back from Running to Connected use STOP. 
- \b test3: a dim console which allows to control all dim servers
  by sending commands via the dim network.
- Both, \b dclient5 and \b test3 accept the command line options -c0, -c1, -c2
  to switch between different console types (or no console in the case of
  \b dclient5). In the console you get help with 'h' and the available
  command with 'c' You get the avilable command-line options with --help

First start the two dserver2s. Then start a dclient5 (if you want it
with console use one of the -c options) and a test3 console (with one
of the console options if you like) you can now control the hardware
boards with the START, STOP and TIME commands or stop (Ctrl-C) and
start one of the programs to see what's happening. In the test3 case
you first have to \e cd to the server to which you want to talk by \b
DATA_LOGGER. Don't forget to start \b dns if you want to control dclient5
from test3 via Dim.

@section dimremarks Remarks about Dim usage

To be able to write all received data directly to the FITS files,
padding has been disabled calling dic_diable_padding() and 
dis_disable_padding(). This is done in our own error handler
DimErrorRedirecter. Since this should be one of the first 
objects created in any environment it is quite save. However, every
Dim client or server in our network which does not use the 
DimErrorRedirecter \b must call these two functions as early as
possible.

<!--
@section exitcodes Exit Code
@section newcommand How to add a new command?
@section description How to add help textes to services and commands?
-->

@section blocking Blocking programs at startup

At startup most programs try to resolve the name of the dim-dns
as well as their local IP address. After this Dim is initialized 
and tries to contact the dns. These are so far the only blocking operations.
Be patient at program startup. They will usually timeout after a while and
give you proper informations.


@section addtab How to add a new tab in the gui?

Do the following steps in exactly this order:
- Insert the new page from the context menu of the QTabWidget
- Copy the QDockWidget from one of the other tabs to the clipboard
- Paste the copied QDockWidget and add it to the new tab (only the tab should be highlited)
- Now click on the context menu of the region in the tab (QWidget) and change the layout to grid layout


@section Documentation

Each program has an extensive help text (except the examples). This
help text can be displayed with the \b --help option. For each program
a man-page is automatically created (from the help-output), which (at
the moment) can be accessed with <B>man ./program.man</B> (Don't forget
the ./ before the filename). With <B>make program.html</B> and 
<B>make program.pdf</B> a HTML page and a pdf document can be created
from the man-page.

With <B>make doxygen-doc</B> the HTML documentation as well as a pdf
with the whole code documentation can be created.

@subsection FACT++ programs

Each documentation is also available with <B>program --help</B> or
<B>man ./program.man</B>.

In alphabetic order:

- <A HREF="man/biasctrl.html">biasctrl</A> [<A HREF="pdf/biasctrl.pdf">pdf</A>]
- <A HREF="man/datalogger.html">datalogger</A> [<A HREF="pdf/datalogger.pdf">pdf</A>]
- <A HREF="man/dimctrl.html">dimctrl</A> [<A HREF="pdf/dimctrl.pdf">pdf</A>]
- <A HREF="man/drivectrl.html">drivectrl</A> [<A HREF="pdf/drivrctrl.pdf">pdf</A>]
- <A HREF="man/evtserver.html">evtserver</A> [<A HREF="pdf/evtserver.pdf">pdf</A>]
- <A HREF="man/fadctrl.html">fadctrl</A> [<A HREF="pdf/fadctrl.pdf">pdf</A>]
- <A HREF="man/feedback.html">feedback</A> [<A HREF="pdf/feedback.pdf">pdf</A>]
- <A HREF="man/fitsdump.html">fitsdump</A> [<A HREF="pdf/fitsdump.pdf">pdf</A>]
- <A HREF="man/fitscheck.html">fitscheck</A> [<A HREF="pdf/fitscheck.pdf">pdf</A>]
- <A HREF="man/fitsselect.html">fitsselect</A> [<A HREF="pdf/fitsselect.pdf">pdf</A>]
- <A HREF="man/fscctrl.html">fscctrl</A> [<A HREF="pdf/fscctrl.pdf">pdf</A>]
- <A HREF="man/ftmctrl.html">ftmctrl</A> [<A HREF="pdf/ftmctrl.pdf">pdf</A>]
- <A HREF="man/getevent.html">getevent</A> [<A HREF="pdf/getevent.pdf">pdf</A>]
- <A HREF="man/gpsctrl.html">gpsctrl</A> [<A HREF="pdf/gpsctrl.pdf">pdf</A>]
- <A HREF="man/lidctrl.html">lidctrl</A> [<A HREF="pdf/lidctrl.pdf">pdf</A>]
- <A HREF="man/magiclidar.html">magiclidar</A> [<A HREF="pdf/magiclidar.pdf">pdf</A>]
- <A HREF="man/magicweather.html">magicweather</A> [<A HREF="pdf/magicweather.pdf">pdf</A>]
- <A HREF="man/mcp.html">mcp</A> [<A HREF="pdf/mcp.pdf">pdf</A>]
- <A HREF="man/pfminictrl.html">pfminictrl</A> [<A HREF="pdf/pfminictrl.pdf">pdf</A>]
- <A HREF="man/pwrctrl.html">pwrctrl</A> [<A HREF="pdf/pwrctrl.pdf">pdf</A>]
- <A HREF="man/ratecontrol.html">ratecontrol</A> [<A HREF="pdf/ratecontrol.pdf">pdf</A>]
- <A HREF="man/ratescan.html">ratescan</A> [<A HREF="pdf/ratescan.pdf">pdf</A>]
- <A HREF="man/showlog.html">showlog</A> [<A HREF="pdf/showlog.pdf">pdf</A>]
- <A HREF="man/smartfact.html">smartfact</A> [<A HREF="pdf/smartfact.pdf">pdf</A>]
- <A HREF="man/sqmctrl.html">sqmctrl</A> [<A HREF="pdf/sqmctrl.pdf">pdf</A>]
- <A HREF="man/temperature.html">temperature</A> [<A HREF="pdf/temperature.pdf">pdf</A>]
- <A HREF="man/timecheck.html">timecheck</A> [<A HREF="pdf/timecheck.pdf">pdf</A>]
- <A HREF="man/tngweather.html">tngweather</A> [<A HREF="pdf/tngweather.pdf">pdf</A>]
- <A HREF="man/zfits.html">zfits</A> [<A HREF="pdf/zfits.pdf">pdf</A>]

@section References

@subsection generalref General references
- <A HREF="http://www.cplusplus.com/reference">The C++ reference</A>
- <A HREF="http://www.boost.org">boost.org: The boost C++ libraries</A>
- <A HREF="http://www.highscore.de/cpp/boost/titelseite.html">Boris Sch&auml;ling: Die Boost C++ Bibliotheken</A>
- <A HREF="http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html">GNU Readline</A>
- <A HREF="http://www.gnu.org/software/ncurses">GNU Ncurses</A>
- <A HREF="http://dim.web.cern.ch/">Distributed Information Management (DIM)</A>
- <A HREF="http://dim.web.cern.ch/dim/cpp_doc/DimCpp.html">Distributed Information Management (DIM) - C++ reference</A>
- <A HREF="http://qt.nokia.com/">Qt homepage</A>
- <A HREF="http://qt.nokia.com/downloads/">Qt downloads</A>

@subsection boostref Boost references
- <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/bind/bind.html">boost::bind (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/doc/html/boost_asio.html">boost asio (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/doc/html/date_time.html">boost date_time (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/doc/html/program_options.html">boost program_options (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/filesystem/v3/doc/index.htm">boost filesystem (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/regex/doc/html/index.html">boost regex (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/system/doc/index.html">boost system (error codes) (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/doc/html/thread.html">boost thread (V1.45.0)</A>
- <A HREF="http://www.boost.org/doc/libs/1_45_0/libs/conversion/lexical_cast.htm">boost lexical_cast (V1.45.0)</A>

@subsection fitsref FITS references
- <A HREF="http://heasarc.gsfc.nasa.gov/docs/heasarc/fits.html">The FITS data format</A>
- <A HREF="http://heasarc.gsfc.nasa.gov/fitsio/">FITS homepage</A>
- <A HREF="http://heasarc.gsfc.nasa.gov/fitsio/CCfits/">CCfits - A C++ wrapper to cfitsio</A>
- <A HREF="http://heasarc.gsfc.nasa.gov/docs/software/ftools/fv/">fv - A very simple viewer to FITS file contents</A>
- <A HREF="http://www.star.bris.ac.uk/~mbt/topcat/">topcat - <B>T</B>ool for <B>OP</B>erations on <B>C</B>atalogues <B>A</B>nd <B>T</B>ables

@subsection qtroot How to integrate root in QT?

- <A HREF="http://doc.trolltech.com/4.3/designer-creating-custom-widgets.html">QT4: Creating custom widgets</A>
- <A HREF="http://root.cern.ch/download/doc/26ROOTandQt.pdf">root: QT integration (pdf)</A>


@section availableprograms Available programs

- dns: Dim's domain-name-server (needed for any communication between Dim servers and clients)
- did: A simple graphical interface to analyse everything in a Dim network

@section Examples

There are a few example programs
- \b ./argv: Example for usage of the class Configure (command line options, configuration file)
- \b ./time: Example for the usage of the class Time (time input/output, conversion)
- \b ./log, \b ./logtime: A simple Dim-Service/-Client combination using MessageDimRX/MessageDimTX

**/
