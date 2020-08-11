#include "Prediction.h"

#include "Database.h"

#include "Time.h"
#include "Configuration.h"

#include <TROOT.h>
#include <TH1.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TLegend.h>

using namespace std;
using namespace Nova;

// ------------------------------------------------------------------------

void CheckForGap(TCanvas &c, TGraph &g, double axis)
{
    if (g.GetN()==0 || axis-g.GetX()[g.GetN()-1]<450)
        return;

    c.cd();
    ((TGraph*)g.DrawClone("C"))->SetBit(kCanDelete);
    while (g.GetN())
        g.RemovePoint(0);
}

void DrawClone(TCanvas &c, TGraph &g)
{
    if (g.GetN()==0)
        return;

    c.cd();
    ((TGraph*)g.DrawClone("C"))->SetBit(kCanDelete);
}

// ========================================================================
// ========================================================================
// ========================================================================

void SetupConfiguration(Configuration &conf)
{
    po::options_description control("Makeplots");
    control.add_options()
        //("ra",        var<double>(), "Source right ascension")
        //("dec",       var<double>(), "Source declination")
        ("date-time", var<string>(), "SQL time (UTC)")
        ("source-database", var<string>(""), "Database link as in\n\tuser:password@server[:port]/database[?compress=0|1].")
        ("max-current", var<double>(100), "Maximum current to display in other plots.")
        ("max-zd", var<double>(50), "Maximum zenith distance to display in other plots")
        ("no-limits", po_switch(), "Switch off limits in plots")
        ("no-ToO", po_switch(), "Skip all sources marked as target-of-opportunity (ToO)")
        ("where", var<string>(), "Define a where clause (it will be concatenate to the xisting ones with AND for source selection)")
        ("print-query", po_switch(), "Print query for source selection")
        ;

    po::positional_options_description p;
    p.add("date-time", 1); // The first positional options

    conf.AddOptions(control);
    conf.SetArgumentPositions(p);
}

void PrintUsage()
{
    cout <<
        "makeplots - The astronomy plotter\n"
        "\n"
        "Calculates several plots for the sources in the database\n"
        "helpful or needed for scheduling. The Plot is always calculated\n"
        "for the night which starts at the same so. So no matter if\n"
        "you specify '1974-09-09 00:00:00' or '1974-09-09 21:56:00'\n"
        "the plots will refer to the night 1974-09-09/1974-09-10.\n"
        "The advantage is that specification of the date as in\n"
        "1974-09-09 is enough. Time axis starts and ends at nautical\n"
        "twilight which is 12deg below horizon.\n"
        "\n"
        "Usage: makeplots sql-datetime\n";
//        "Usage: makeplots sql-datetime [--ra={ra} --dec={dec}]\n";
    cout << endl;
}

int main(int argc, const char* argv[])
{
    gROOT->SetBatch();

    Configuration conf(argv[0]);
    conf.SetPrintUsage(PrintUsage);
    SetupConfiguration(conf);

    if (!conf.DoParse(argc, argv))
        return 127;

//    if (conf.Has("ra")^conf.Has("dec"))
//    {
//        cout << "ERROR - Either --ra or --dec missing." << endl;
//        return 1;
//    }

    // ------------------ Eval config ---------------------

    Time time;
    if (conf.Has("date-time"))
        time.SetFromStr(conf.Get<string>("date-time"));

    const double max_current = conf.Get<double>("max-current");
    const double max_zd      = conf.Get<double>("max-zd");
    const double no_limits   = conf.Get<bool>("no-limits");

    // -12: nautical
    // Sun set with the same date than th provided date
    // Sun rise on the following day
    const RstTime sun_set  = GetSolarRst(time.JD()-0.5, -12);
    const RstTime sun_rise = GetSolarRst(time.JD()+0.5, -12);

    const double jd = floor(time.Mjd())+2400001;

    cout << "Time: " << time     << endl;
    cout << "Base: " << Time(jd-0.5) << endl;
    cout << "Set:  " << Time(sun_set.set)   << endl;
    cout << "Rise: " << Time(sun_rise.rise) << endl;

    const double sunset  = sun_set.set;
    const double sunrise = sun_rise.rise;

    const string fDatabase = conf.Get<string>("source-database");

    // ------------- Get Sources from databasse ---------------------

    const string isToO = conf.Has("no-ToO") && conf.Get<bool>("no-ToO") ? " AND fIsToO=0" : "";
    const string where = conf.Has("where") ? " AND ("+conf.Get<string>("where")+")" : "";

    const string query = "SELECT fSourceName, fRightAscension, fDeclination FROM Source WHERE fSourceTypeKEY=1"+isToO+where;

    if (conf.Get<bool>("print-query"))
        cout << "\nQuery:\n" << query << "\n" << endl;

    const mysqlpp::StoreQueryResult res =
        Database(fDatabase).query(query).store();

    // ------------- Create canvases and frames ---------------------

    // It is important to use an offset which is larger than
    // 1970-01-01 00:00:00. This one will not work if your
    // local time zone is positive!
    TH1S hframe("", "", 1, Time(sunset).Mjd()*24*3600, Time(sunrise).Mjd()*24*3600);
    hframe.SetStats(kFALSE);
    hframe.GetXaxis()->SetTimeFormat("%Hh%M%F1995-01-01 00:00:00 GMT");
    hframe.GetXaxis()->SetTitle((Time(jd).GetAsStr("%d/%m/%Y")+"  -  "+Time(jd+1).GetAsStr("%d/%m/%Y")+"  [UTC]").c_str());
    hframe.GetXaxis()->CenterTitle();
    hframe.GetYaxis()->CenterTitle();
    hframe.GetXaxis()->SetTimeDisplay(true);
    hframe.GetYaxis()->SetTitleSize(0.040);
    hframe.GetXaxis()->SetTitleSize(0.040);
    hframe.GetXaxis()->SetTitleOffset(1.1);
    hframe.GetYaxis()->SetLabelSize(0.040);
    hframe.GetXaxis()->SetLabelSize(0.040);

    TCanvas c1;
    c1.SetFillColor(kWhite);
    c1.SetBorderMode(0);
    c1.SetFrameBorderMode(0);
    c1.SetLeftMargin(0.085);
    c1.SetRightMargin(0.01);
    c1.SetTopMargin(0.03);
    c1.SetGrid();
    hframe.GetYaxis()->SetTitle("Altitude [deg]");
    hframe.SetMinimum(15);
    hframe.SetMaximum(90);
    hframe.DrawCopy();

    TCanvas c2;
    c2.SetFillColor(kWhite);
    c2.SetBorderMode(0);
    c2.SetFrameBorderMode(0);
    c2.SetLeftMargin(0.085);
    c2.SetRightMargin(0.01);
    c2.SetTopMargin(0.03);
    c2.SetGrid();
    hframe.GetYaxis()->SetTitle("Predicted Current [#muA]");
    hframe.SetMinimum(0);
    hframe.SetMaximum(100);
    hframe.DrawCopy();

    TCanvas c3;
    c3.SetFillColor(kWhite);
    c3.SetBorderMode(0);
    c3.SetFrameBorderMode(0);
    c3.SetLeftMargin(0.085);
    c3.SetRightMargin(0.01);
    c3.SetTopMargin(0.03);
    c3.SetGrid();
    c3.SetLogy();
    hframe.GetYaxis()->SetTitle("Estimated relative threshold");
    hframe.GetYaxis()->SetMoreLogLabels();
    hframe.SetMinimum(0.9);
    hframe.SetMaximum(11);
    hframe.DrawCopy();

    TCanvas c4;
    c4.SetFillColor(kWhite);
    c4.SetBorderMode(0);
    c4.SetFrameBorderMode(0);
    c4.SetLeftMargin(0.085);
    c4.SetRightMargin(0.01);
    c4.SetTopMargin(0.03);
    c4.SetGrid();
    hframe.GetYaxis()->SetTitle("Distance to moon [deg]");
    hframe.SetMinimum(0);
    hframe.SetMaximum(180);
    hframe.DrawCopy();

    Int_t color[] = { kBlack, kRed, kBlue, kGreen, kCyan, kMagenta };
    Int_t style[] = { kSolid, kDashed, kDotted };

    TLegend leg(0, 0, 1, 1);

    // ------------- Loop over sources ---------------------

    Int_t cnt=0;
    for (vector<mysqlpp::Row>::const_iterator v=res.begin(); v<res.end(); v++, cnt++)
    {
        // Eval row
        const string name = (*v)[0].c_str();

        EquPosn pos;
        pos.ra  = double((*v)[1])*15;
        pos.dec = double((*v)[2]);

        // Create graphs
        TGraph g1, g2, g3, g4, gr, gm;
        g1.SetName(name.data());
        g2.SetName(name.data());
        g3.SetName(name.data());
        g4.SetName(name.data());
        g1.SetLineWidth(2);
        g2.SetLineWidth(2);
        g3.SetLineWidth(2);
        g4.SetLineWidth(2);
        gm.SetLineWidth(1);
        g1.SetLineStyle(style[cnt/6]);
        g2.SetLineStyle(style[cnt/6]);
        g3.SetLineStyle(style[cnt/6]);
        g4.SetLineStyle(style[cnt/6]);
        g1.SetLineColor(color[cnt%6]);
        g2.SetLineColor(color[cnt%6]);
        g3.SetLineColor(color[cnt%6]);
        g4.SetLineColor(color[cnt%6]);
        gm.SetLineColor(kYellow);

        if (cnt==0)
            leg.AddEntry(gm.Clone(), "Moon", "l");
        leg.AddEntry(g1.Clone(), name.data(), "l");

        // Loop over 24 hours
        for (double h=0; h<1; h+=1./(24*12))
        {
            const SolarObjects so(jd+h);

            // get local position of source
            const HrzPosn hrz = GetHrzFromEqu(pos, so.fJD);

            if (v==res.begin())
                cout << Time(so.fJD) <<" " << 90-so.fMoonHrz.alt <<  endl;

            const double cur = FACT::PredictI(so, pos);

            // Relative  energy threshold prediction
            const double ratio = pow(cos((90-hrz.alt)*M_PI/180), -2.664);

            // Add points to curve
            const double axis = Time(so.fJD).Mjd()*24*3600;

            // If there is a gap of more than one bin, start a new curve
            CheckForGap(c1, g1, axis);
            CheckForGap(c1, gm, axis);
            CheckForGap(c2, g2, axis);
            CheckForGap(c3, g3, axis);
            CheckForGap(c4, g4, axis);

            // Add data
            if (no_limits || cur<max_current)
                g1.SetPoint(g1.GetN(), axis, hrz.alt);

            if (no_limits || 90-hrz.alt<max_zd)
                g2.SetPoint(g2.GetN(), axis, cur);

            if (no_limits || (cur<max_current && 90-hrz.alt<max_zd))
                g3.SetPoint(g3.GetN(), axis, ratio*pow(cur/6.2, 0.394));

            if (no_limits || (cur<max_current && 90-hrz.alt<max_zd))
            {
                const double angle = GetAngularSeparation(so.fMoonEqu, pos);
                g4.SetPoint(g4.GetN(), axis, angle);
            }

            if (cnt==0)
                gm.SetPoint(gm.GetN(), axis, so.fMoonHrz.alt);
        }

        if (cnt==0)
            DrawClone(c1, gm);

        DrawClone(c1, g1);
        DrawClone(c2, g2);
        DrawClone(c3, g3);
        DrawClone(c4, g4);
    }


    // Save three plots
    TCanvas c5;
    c5.SetFillColor(kWhite);
    c5.SetBorderMode(0);
    c5.SetFrameBorderMode(0);
    leg.Draw();

    const string t = Time(jd).GetAsStr("%Y%m%d");

    c1.SaveAs((t+"-ZenithDistance.eps").c_str());
    c2.SaveAs((t+"-PredictedCurrent.eps").c_str());
    c3.SaveAs((t+"-RelativeThreshold.eps").c_str());
    c4.SaveAs((t+"-MoonDist.eps").c_str());
    c5.SaveAs((t+"-Legend.eps").c_str());

    c1.SaveAs((t+"-ZenithDistance.root").c_str());
    c2.SaveAs((t+"-PredictedCurrent.root").c_str());
    c3.SaveAs((t+"-RelativeThreshold.root").c_str());
    c4.SaveAs((t+"-MoonDist.root").c_str());

    c1.Print((t+".pdf(").c_str(), "pdf");
    c2.Print((t+".pdf" ).c_str(), "pdf");
    c3.Print((t+".pdf" ).c_str(), "pdf");
    c4.Print((t+".pdf" ).c_str(), "pdf");
    c5.Print((t+".pdf)").c_str(), "pdf");

    return 0;
}
