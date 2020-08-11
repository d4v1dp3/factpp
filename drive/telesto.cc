#include <TROOT.h>
#include <TClass.h>
#include <TSystem.h>
#include <TGClient.h>
#include <TApplication.h>
#include <TObjectTable.h>

#include "MAGIC.h"

#include "MLog.h"
#include "MLogManip.h"

//#include "MEnv.h"
#include "MArgs.h"
#include "MArray.h"
#include "MParContainer.h"
//#include "MDirIter.h"

#include "TPointGui.h"
//#include "MStatusDisplay.h"

//#include "MSequence.h"
//#include "MJStar.h"

using namespace std;

static void StartUpMessage()
{
    gLog << all << endl;

    //                1         2         3         4         5         6
    //       123456789012345678901234567890123456789012345678901234567890
    gLog << "========================================================" << endl;
    gLog << "                       Telesto - COSY"                    << endl;
    gLog << "           Telesto - Telescope TPoint organizer"          << endl;
    gLog << "       Compiled with ROOT v" << ROOT_RELEASE << " on <" << __DATE__ << ">" << endl;
    gLog << "========================================================" << endl;
    gLog << endl;
}

static void Usage()
{
    //                1         2         3         4         5         6         7         8
    //       12345678901234567890123456789012345678901234567890123456789012345678901234567890
    gLog << all << endl;
    gLog << "Sorry the usage is:" << endl;
    gLog << " telestop [file.txt|file.col [pointing.mod]]" << endl << endl;
    gLog << " Arguments:" << endl;
    gLog << "   file.txt|file.col         A collection of files or a file with tpoints" << endl;
    gLog << "   pointing.mod              A pointing model to load at startup" << endl << endl;
    gLog << " Root Options:" << endl;
    gLog << "   -b                        Batch mode (no graphical output to screen)" << endl<<endl;
    gLog << " Options:" << endl;
    gLog.Usage();
//    gLog << "   --debug-env=0             Disable debugging setting resources <default>" << endl;
//    gLog << "   --debug-env[=1]           Display untouched resources after program execution" << endl;
//    gLog << "   --debug-env=2             Display untouched resources after eventloop setup" << endl;
//    gLog << "   --debug-env=3             Debug setting resources from resource file and command line" << endl;
    gLog << "   --debug-mem               Debug memory usage" << endl << endl;
//    gLog << "   --rc=Name:option          Set or overwrite a resource of the resource file." << endl;
    gLog << "                             (Note, that this option can be used multiple times." << endl;
    gLog << endl;
    gLog << " Output options:" << endl;
//    gLog << "   -q                        Quit when job is finished" << endl;
//    gLog << "   -f                        Force overwrite of existing files" << endl;
    gLog << endl;
    gLog << "   --version, -V             Show startup message with version number" << endl;
    gLog << "   -?, -h, --help            This help" << endl << endl;
    gLog << "Background:" << endl;
    gLog << " Telesto is a moon of Saturn.  It was discovered by  Smith,  Reitsema," << endl;
    gLog << " Larson and Fountain in 1980 from ground-based observations,  and  was" << endl;
    gLog << " provisionally designated S/1980 S 13." << endl;
    gLog << " In 1983 it was officially named after Telesto  of Greek mythology. It" << endl;
    gLog << " is also designated as Saturn XIII or Tethys B." << endl;
    gLog << " Telesto  is  co-orbital  with  Tethys,  residing  in  Tethys' leading" << endl;
    gLog << " Lagrangian  point  (L4).  This relationship  was  first identified by" << endl;
    gLog << " Seidelmann et al.   The  moon  Calypso  also  resides  in  the  other" << endl;
    gLog << " (trailing) lagrangian point of Tethys, 60 deg in the other direction." << endl;
    gLog << " The  Cassini probe  performed a distant flyby  of Telesto on Oct. 11," << endl;
    gLog << " 2005.  The resulting  images show  that  its surface  is surprisingly" << endl;
    gLog << " smooth, devoid of small impact craters." << endl << endl;
}

int main(int argc, char **argv)
{
    if (!MARS::CheckRootVer())
        return 0xff;

    MLog::RedirectErrorHandler(MLog::kColor);

    //
    // Evaluate arguments
    //
    MArgs arg(argc, argv);
    gLog.Setup(arg);

    StartUpMessage();

    if (arg.HasOnly("-V") || arg.HasOnly("--version"))
        return 0;

    if (arg.HasOnly("-?") || arg.HasOnly("-h") || arg.HasOnly("--help"))
    {
        Usage();
        return 2;
    }

    const Bool_t kDebugMem   = arg.HasOnlyAndRemove("--debug-mem");

    //
    // check for the right usage of the program (number of arguments)
    //
    if (arg.GetNumArguments()>2)
    {
        gLog << warn << "WARNING - Wrong number of arguments..." << endl;
        Usage();
        return 2;
    }

    TString fname=arg.GetArgumentStr(0);
    TString mod  =arg.GetArgumentStr(1);

    //
    // check for the right usage of the program (number of options)
    //
    if (arg.GetNumOptions()>0)
    {
        gLog << warn << "WARNING - Unknown commandline options..." << endl;
        arg.Print("options");
        gLog << endl;
        return 2;
    }

//    MArray::Class()->IgnoreTObjectStreamer();
//    MParContainer::Class()->IgnoreTObjectStreamer();

    TApplication app("telesto", &argc, argv);
    if (!gClient || gROOT->IsBatch())
    {
        gLog << err << "Bombing... maybe your DISPLAY variable is not set correctly!" << endl;
        return 1;
    }

    if (kDebugMem)
        TObject::SetObjectStat(kTRUE);

    TPointGui *gui = new TPointGui(fname, mod);
    gui->SetExitLoopOnClose();

    // Wait until the user decides to exit the application
    app.Run(kFALSE);

    if (TObject::GetObjectStat())
    {
        TObject::SetObjectStat(kFALSE);
        gObjectTable->Print();
    }

    return 0;
}
