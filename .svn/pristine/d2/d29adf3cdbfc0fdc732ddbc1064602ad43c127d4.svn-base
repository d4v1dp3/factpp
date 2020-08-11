void display(const char *filename="spectrum.hist.root")
{
    gEnv->SetValue("Canvas.ShowEventStatus", 1);
    gEnv->SetValue("TFile.Recover", 0);

    // --------------------------------

    TFile file(filename);
    if (file.IsZombie())
        return;

    // --------------------------------

    cout << endl;

    file.ReadAll();
    file.ls();

    cout << endl;

    // --------------------------------

    TH1D *h1 = 0;
    TH2D *h2 = 0;

    TCanvas *c = 0;

    gStyle->SetErrorX(0.5);

    TF1 fx("fx", "x", 0, 90);
    fx.SetLineStyle(kDashed);
    fx.SetLineColor(kBlack);

    TF1 f0("f0", "[0]", 0, 90);
    f0.SetParameter(0, 0);
    f0.SetLineStyle(kDashed);
    f0.SetLineColor(kBlack);

    TF1 f("spectrum", "[0]*pow(pow(10, x)/1000, -[1])", 2.8, 4.5);
    f.SetLineColor(kBlue);

    // --------------------------------

    c = new TCanvas("Zd Weights", "Zd Weights");
    c->Divide(2,2);
    c->cd(1);
    file.GetObject("Zd/OnTime", h1);
    h1->SetTitle("OnTime (hist), Scaled excess (markers)");
    double time = h1->Integral();
    h1->DrawCopy();
    file.GetObject("Data/Theta/Excess", h1);
    h1->Scale(time/h1->Integral());
    h1->DrawCopy("same");
    c->cd(2);
    file.GetObject("Zd/CountN", h1);
    h1->DrawCopy("P");
    c->cd(3);
    file.GetObject("Zd/ZdWeight", h1);
    h1->DrawCopy("P");

    // --------------------------------

    c = new TCanvas("Zenith Angle", "Zenith Angle");
    c->Divide(2,2);

    c->cd(1);
    gPad->SetGridy();
    file.GetObject("MC/theta/BiasW", h1);
    h1->DrawCopy("P");
    f0.DrawCopy("same");
    c->cd(2);
    gPad->SetGridy();
    file.GetObject("MC/theta/ResolutionW", h1);
    h1->DrawCopy("][ P");
    f0.DrawCopy("same");

    c->cd(3);
    gPad->SetGridy();
    file.GetObject("MC/theta/TriggerEfficiencyW", h1);
    h1->SetTitle("Trigger (black) and Cut Efficiency (blue)");
    h1->DrawCopy("P");
    file.GetObject("MC/theta/CutEfficiencyW", h1);
    h1->SetMarkerColor(kBlue);
    h1->DrawCopy("P same");

    c->cd(4);
    gPad->SetLogy();
    file.GetObject("Data/Theta/Spectrum", h1);
    h1->DrawCopy("P");
    file.GetObject("Data/Theta/RolkeUL", h1);
    h1->SetMarkerStyle(23);
    h1->DrawCopy("P same");

    // --------------------------------

    c = new TCanvas("Impact", "Impact");
    c->Divide(2,2);
    c->cd(1);
    file.GetObject("MC/dense/TrueEnergy/Impact", h2);
    h2->DrawCopy("colz");
    h2->DrawCopy("same");
    c->cd(2);
    file.GetObject("MC/sparse/TrueEnergy/Impact", h2);
    h2->DrawCopy("colz");
    h2->DrawCopy("same");
    c->cd(3);
    file.GetObject("MC/theta/Impact", h2);
    h2->DrawCopy("colz");
    h2->DrawCopy("same");

    // --------------------------------

    c = new TCanvas("Migration", "Migration");
    c->Divide(2,1);
    c->cd(1);
    file.GetObject("MC/dense/Migration", h2);
    h2->DrawCopy("colz");
    fx.DrawCopy("same");
    c->cd(2);
    file.GetObject("MC/sparse/Migration", h2);
    h2->DrawCopy("colz");
    fx.DrawCopy("same");

    // --------------------------------
    c = new TCanvas("Resolution", "Resolution");
    c->Divide(2,2);
    c->cd(1);
    gPad->SetGridy();
    file.GetObject("MC/dense/TrueEnergy/BiasW", h1);
    h1->DrawCopy("P");
    f0.DrawCopy("same");
    c->cd(2);
    gPad->SetGridy();
    file.GetObject("MC/dense/EstimatedEnergy/BiasW", h1);
    h1->DrawCopy("P");
    f0.DrawCopy("same");
    c->cd(3);
    file.GetObject("MC/dense/TrueEnergy/ResolutionW", h1);
    h1->DrawCopy("][ P");
    c->cd(4);
    file.GetObject("MC/dense/EstimatedEnergy/ResolutionW", h1);
    h1->DrawCopy("][ P");

    // --------------------------------

    c = new TCanvas("Efficiency", "Efficiency");
    c->Divide(2,2);
    c->cd(1);
    file.GetObject("MC/dense/TrueEnergy/TriggerEfficiencyW", h1);
    h1->SetTitle("Trigger (black) and Cut (blue) efficiency");
    h1->DrawCopy("P X0");
    h1->DrawCopy("C hist same");
    file.GetObject("MC/dense/TrueEnergy/CutEfficiencyW", h1);
    h1->SetLineColor(kBlue);
    h1->DrawCopy("P X0 same");
    h1->DrawCopy("C hist same");

    c->cd(2);
    gPad->SetLogy();
    file.GetObject("MC/dense/TrueEnergy/EffectiveAreaW", h1);
    h1->SetTitle("Effective Collection Area");
    h1->DrawCopy("P X0");
    h1->DrawCopy("C hist same");

    c->cd(3);

    gPad->SetLogy();
    file.GetObject("MC/dense/TrueEnergy/SimFluxW", h1);
    h1->SetTitle("Simulated (blk), Triggered (blk), Cut (blu), Bg (blu), Reconstructed (red)  spectrum");
    h1->DrawCopy("P X0");
    h1->DrawCopy("C hist same");

    gPad->SetLogy();
    file.GetObject("MC/dense/TrueEnergy/TrigFluxW", h1);
    h1->DrawCopy("P X0 same");
    h1->DrawCopy("C hist same");

    gPad->SetLogy();
    file.GetObject("MC/dense/TrueEnergy/ExcessFluxW", h1);
    h1->DrawCopy("P X0 same");
    h1->DrawCopy("C hist same");

    TSpline3 spline(h1);
    spline.SetLineColor(kBlue);
    spline.DrawClone("same");

    float  max = 0;
    float xmax = 0;
    for (float x=2.5; x<5; x+=0.01)
    {
        if (spline.Eval(x)>max)
        {
            xmax = x;
            max = spline.Eval(x);
        }
    }

    TMarker m;
    m.SetMarkerColor(kBlue);
    m.SetMarkerStyle(kStar);
    m.DrawMarker(xmax, max);

    cout << endl;
    cout << "Threshold Max: " << pow(10, xmax) << endl;
    cout << endl;

    file.GetObject("MC/dense/EstimatedEnergy/ExcessFluxW", h1);
    h1->SetLineColor(kRed);
    h1->DrawCopy("X0 P same");
    h1->DrawCopy("C hist same");

    file.GetObject("MC/dense/TrueEnergy/BackgroundFluxW", h1);
    h1->SetLineColor(kBlue);
    h1->DrawCopy("X0 P same");
    h1->DrawCopy("C hist same");

    c->cd(4);
    gPad->SetLogy();
    file.GetObject("Data/Energy/Differential/Excess", h1);
    h1->SetTitle("Measured Signal (blue) / Simulated Excess (black)");
    h1->SetLineColor(kBlue);
    h1->DrawCopy("P X0");
    h1->DrawCopy("C hist same");
    double scale = h1->Integral();
    file.GetObject("MC/sparse/EstimatedEnergy/ExcessFluxW", h1);
    h1->Scale(scale/h1->Integral());
    h1->DrawCopy("X0 P same");
    h1->DrawCopy("C hist same");

    // --------------------------------
    f0.SetLineColor(kRed);
    // --------------------------------

    c = new TCanvas("Integral Spectrum", "Integral Spectrum");
    c->Divide(2,2);
    c->cd(4);
    gPad->SetLogy();
    file.GetObject("Data/Energy/Differential/IntegratedSpectrum", h1);
    h1->SetLineColor(kGray);
    h1->SetMarkerColor(kGray);
    if (h1->GetMinimum()<=0)
    {
        h1->SetMinimum(h1->GetMinimum(0)*0.1);
        cout << "WARNING: Integral Spectrum contains negative flux points!\n" << endl;
    }
    h1->DrawCopy("P");
    file.GetObject("Data/Energy/Integral/Spectrum", h1);
    h1->DrawCopy("P same");

    f.SetParameters(h1->GetMaximum(), 1.4);
    h1->Fit(&f, "N0EM", "");
    f.DrawCopy("same");

    cout << endl;
    cout << "ChiSq " << f.GetChisquare() << " / " << f.GetNDF() << endl;
    cout << "Prob. " << f.GetProb() << endl;
    cout << endl;

    file.GetObject("Data/Energy/Integral/RolkeUL", h1);
    h1->SetMarkerStyle(23);
    h1->DrawCopy("P same");

    //file.GetObject("Data/Energy/Integral/RolkeLL", h1);
    //h1->SetMarkerStyle(22);
    //h1->DrawCopy("P same");

    c->cd(1);
    gPad->SetGridx();
    file.GetObject("Data/Energy/Integral/SigmaFlux", h1);
    h1->SetMaximum(5);
    h1->SetMarkerStyle(kFullDotLarge);
    h1->DrawCopy("P");
    f0.SetParameter(0, 1);
    f0.DrawCopy("same");

    c->cd(3);
    gPad->SetGridx();
    file.GetObject("Data/Energy/Integral/BackgroundI", h1);
    h1->SetTitle("Integral Signal (black), Background (red) and Excess (blue)");
    h1->Scale(5);
    h1->SetMaximum(30);
    h1->SetMarkerColor(kRed);
    h1->SetLineColor(kRed);
    h1->DrawCopy("P");
    file.GetObject("Data/Energy/Integral/SignalI", h1);
    h1->DrawCopy("P same");
    file.GetObject("Data/Energy/Integral/ExcessI", h1);
    h1->SetMarkerColor(kBlue);
    h1->SetLineColor(kBlue);
    h1->DrawCopy("P same");

    f0.SetParameter(0, 10);
    f0.DrawCopy("same");

    // --------------------------------

    c = new TCanvas("Differential Spectrum", "Differential Spectrum");
    c->Divide(2,2);
    c->cd(4);
    gPad->SetLogy();
    file.GetObject("Data/Energy/Differential/Spectrum", h1);
    if (h1->GetMinimum()<=0)
    {
        h1->SetMinimum(h1->GetMinimum(0)*0.1);
        cout << "WARNING: Differential Spectrum contains negative flux points!\n" << endl;
    }
    h1->DrawCopy("P");

    f.SetParameters(h1->GetMaximum(), 2.4);
    h1->Fit(&f, "N0EM", "");
    f.DrawCopy("same");

    cout << endl;
    cout << "ChiSq " << f.GetChisquare() << " / " << f.GetNDF() << endl;
    cout << "Prob. " << f.GetProb() << endl;
    cout << endl;

    file.GetObject("Data/Energy/Differential/RolkeUL", h1);
    h1->SetMarkerStyle(23);
    h1->DrawCopy("P same");

    //file.GetObject("Data/Energy/Differential/RolkeLL", h1);
    //h1->SetMarkerStyle(22);
    //h1->DrawCopy("P same");

    //file.GetObject("Data/Energy/Differential/FeldmanCousins", h1);
    //h1->SetMarkerColor(kBlue);
    //h1->SetMarkerStyle(23);
    //h1->DrawCopy("P same");

    c->cd(1);
    gPad->SetGridx();
    file.GetObject("Data/Energy/Differential/SigmaFlux", h1);
    h1->SetMaximum(5);
    h1->SetMarkerStyle(kFullDotLarge);
    h1->DrawCopy("P");
    f0.SetParameter(0, 1);
    f0.DrawCopy("same");

    c->cd(3);
    gPad->SetGridx();
    file.GetObject("Data/Energy/Differential/Background", h1);
    h1->SetTitle("Differential Signal (black), Background (red) and Excess (blue)");
    h1->Scale(5);
    h1->SetMaximum(30);
    h1->SetMarkerColor(kRed);
    h1->SetLineColor(kRed);
    h1->DrawCopy("P");
    file.GetObject("Data/Energy/Differential/Signal", h1);
    h1->DrawCopy("P same");
    file.GetObject("Data/Energy/Differential/Excess", h1);
    h1->SetMarkerColor(kBlue);
    h1->SetLineColor(kBlue);
    h1->DrawCopy("P same");

    f0.SetParameter(0, 10);
    f0.DrawCopy("same");
}
