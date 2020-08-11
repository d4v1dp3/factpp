#include <boost/regex.hpp>

#include "Time.h"
#include "tools.h"
#include "WindowLog.h"
#include "Configuration.h"


using namespace std;

// ------------------------------------------------------------------------

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Showlog");
    control.add_options()
        ("file,f",    vars<string>(), "File names of log-files to be read.")
        ("begin,b",   var<string>(), "Start time to be displayed (e.g. 20:00:12)")
        ("end,e",     var<string>(), "End time to be displayed (e.g. 21:00:13)")
        ("verbose,v", var<int16_t>()->implicit_value(true)->default_value(8), "Verbosity level (0:only fatal errors, 8:everything)")
        ("color,c",   po_switch(), "Process a file which already contains color codes")
        ("strip,s",   po_switch(), "Strip color codes completely")
        ;

    po::positional_options_description p;
    p.add("file", -1); // The first positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "showlog - Log file converter\n"
        "\n"
        "This tool can be used to convert the log-files written by the\n"
        "datalogger back to colored output, limit the displayed time\n"
        "range and limit the displayed severity of the messages.\n"
        "Note that this tool will not work by default on logs containing\n"
        "already colored output as the logs directly written by the programs.\n"
        "Use -c or --color to process color coded files.\n"
        "\n"
        "The default is to read from stdin if no filoename as given. If, as "
        "a filename, just a number between 2000000 and 21000000 is given, "
        "e.g. 20111016 a log with the name /fact/aux/2011/10/16/20111016.log "
        "is read.\n"
        "\n"
        "Usage: showlog [-c] [-vN] [-b start] [-e end] [file1 ...]\n"
        "  or:  showlog [-c] [-vN] [-b start] [-e end] YYYYMMDD\n";
    cout << endl;
}

void PrintHelp()
{
    cout <<
        "\n"
        "Examples:\n"
        " cat temperature.log | showlog -c -v2\n"
        " showlog -c temperature.log -v2\n"
        " cat 20130909.log | showlog -v2\n"
        " showlog 20130909.log -v2\n"
        "\n";
    cout << endl;
}


void showlog(string fname, const Time &tbeg, const Time &tend, int16_t severity, bool color, bool strip)
{
    // Alternatives
    // \x1B\[[0-9;]*[mK]
    // \x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]
    // \x1B\[([0-9]{1,3}((;[0-9]{1,3})*)?)?[m|K]
    const boost::regex reg("\x1B\[([0-9]{1,3}(;[0-9]{1,3})?[a-zA-Z]");

    const uint32_t night = atoi(fname.c_str());
    if (night>20000000 && night<21000000 &&to_string(night)==fname)
        fname = Tools::Form("/fact/aux/%04d/%02d/%02d/%d.log",
                            night/10000, (night/100)%100, night%100, night);

    if (!fname.empty())
        cerr << "Reading " << fname << endl;

    ifstream fin(fname.empty() ? "/dev/stdin" : fname.c_str());
    if (!fin)
        throw runtime_error(strerror(errno));

    string buffer;

    WindowLog log;

    Time tprev;

    while (getline(fin, buffer, '\n'))
    {
        if (color || strip)
            buffer = boost::regex_replace(buffer, reg, "");

        if (buffer.size()==0)
            continue;

        if (buffer.size()>18)
        {
            const string tm = buffer.substr(4, 15);

            const Time t("1970-01-01 "+tm);

            if (tbeg.IsValid() && !tend.IsValid() && t<tbeg)
                continue;

            if (tend.IsValid() && !tbeg.IsValid() && t>tend)
                continue;

            if (tbeg.IsValid() && tend.IsValid())
            {
                if (tend>tbeg)
                {
                    if (t<tbeg)
                        continue;
                    if (t>tend)
                        continue;
                }
                else
                {
                    if (t>tbeg)
                        continue;
                    if (t<tend)
                        continue;
                }
            }
        }

        if (buffer.size()>1 && !strip)
        {
            int16_t lvl = -1;
            switch (buffer[1])
            {
            case ' ': lvl = 7; break; // kDebug
            case '#': lvl = 6; break; // kComment
            case '-': lvl = 5; break; // kMessage
            case '>': lvl = 4; break;
            case 'I': lvl = 3; break; // kInfo
            case 'W': lvl = 2; break; // kWarn
            case 'E': lvl = 1; break; // kError/kAlarm
            case '!': lvl = 0; break; // kFatal
            }

            if (lvl>severity)
                continue;

            switch (buffer[1])
            {
            case ' ': log << kBlue;          break; // kDebug
            case '#': log << kDefault;       break; // kComment
            case '-': log << kDefault;       break; // kMessage
            case '>': log << kBold;          break;
            case 'I': log << kGreen;         break; // kInfo
            case 'W': log << kYellow;        break; // kWarn
            case 'E': log << kRed;           break; // kError/kAlarm
            case '!': log << kRed << kBlink; break; // kFatal
            }
        }

        (strip?cout:log) << buffer << endl;
    }
}

int main(int argc, const char* argv[])
{
    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv, PrintHelp))
        return 127;

    const vector<string> files = conf.Vec<string>("file");

    Time tbeg(Time::none);
    Time tend(Time::none);

    if (conf.Has("begin"))
    {
        std::stringstream stream;
        stream << "1970-01-01 " << conf.Get<string>("begin");
        stream >> Time::iso >> tbeg;
    }

    if (conf.Has("end"))
    {
        std::stringstream stream;
        stream << "1970-01-01 " << conf.Get<string>("end");
        stream >> Time::iso >> tend;
    }

    if (files.size()==0)
        showlog("", tbeg, tend, conf.Get<int16_t>("verbose"), conf.Get<bool>("color"), conf.Get<bool>("strip"));

    for (auto it=files.begin(); it!=files.end(); it++)
        showlog(*it, tbeg, tend, conf.Get<int16_t>("verbose"), conf.Get<bool>("color"), conf.Get<bool>("strip"));

    return 0;
}
