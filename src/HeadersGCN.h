#ifndef FACT_HeadersGCN
#define FACT_HeadersGCN

namespace GCN
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected    = 2,
            kValid        = 3,
        };

    }

    struct PaketType_t
    {
        std::string instrument;
        std::string name;
        std::string description;
    };

    typedef std::map<uint16_t, PaketType_t> PaketMap;
    typedef std::map<uint16_t, PaketType_t>::const_iterator PaketPtr;

    // probably from https://gcn.gsfc.nasa.gov/filtering.html
    static const std::map<uint16_t, PaketType_t> PaketTypes =
    {   // inactive, ACTIVE[1], T-Only[-], in-work[+], private[p]
        { {   1 }, { /*   */ "BATSE",    "GRB_COORDS",	             "BATSE Trigger coords (the first GCN Notice Type)" } },
        { {   2 }, { /* 1 */ "",         "TEST_COORDS",	             "Test coords" } },
        { {   3 }, { /* 1 */ "",         "IM_ALIVE",	             "I'm alive socket packet every 60 sec" } },
        { {   4 }, { /* 1 */ "",         "KILL_SOCKET",	             "Kill a socket connection" } },
        { {  11 }, { /*   */ "",         "MAXBC",	             "MAXC1/BC" } },
        { {  21 }, { /*   */ "",         "BRAD_COORDS",	             "Special Test coords packet for BRADFORD" } },
        { {  22 }, { /*   */ "BATSE",    "GRB_FINAL",	             "Final BATSE coords" } },
        { {  24 }, { /*   */ "HUNTS",    "HUNTS_SRC",	             "Huntsville LOCBURST GRB coords (HitL)" } },
        { {  25 }, { /*   */ "ALEXIS",   "ALEXIS_SRC",	             "ALEXIS Transient coords" } },
        { {  26 }, { /*   */ "PCA",      "XTE_PCA_ALERT",            "XTE-PCA ToO Observation Scheduled" } },
        { {  27 }, { /*   */ "PCA",      "XTE_PCA_SRC",	             "XTE-PCA GRB coords" } },
        { {  28 }, { /*   */ "ASM",      "XTE_ASM_ALERT",            "XTE-ASM Alert" } },
        { {  29 }, { /*   */ "ASM",      "XTE_ASM_SRC",	             "XTE-ASM GRB coords" } },
        { {  30 }, { /*   */ "COMPTEL",  "COMPTEL_SRC",	             "COMPTEL GRB coords" } },
        { {  31 }, { /*   */ "IPN",      "IPN_RAW",	             "IPN_RAW GRB annulus coords (position is center of Annulus)" } },
        { {  32 }, { /*   */ "IPN",      "IPN_SEG",	             "IPN+POS GRB annulus seg (kind of a cheat to allow error filter)" } },
        { {  33 }, { /*   */ "SAX",      "SAX_WFC_ALERT",            "SAX-WFC Alert" } },
        { {  34 }, { /*   */ "SAX",      "SAX_WFC_SRC",	             "SAX-WFC GRB coords" } },
        { {  35 }, { /*   */ "SAX",      "SAX_NFI_ALERT",            "SAX-NFI Alert" } },
        { {  36 }, { /*   */ "SAX",      "SAX_NFI_SRC",	             "SAX-NFI GRB coords" } },
        { {  37 }, { /*   */ "ASM",      "XTE_ASM_TRANS",            "XTE-ASM TRANSIENT coords" } },
        { {  38 }, { /* 1 */ "",         "spare38",	             "(spare; used for s/w development testing)" } },
        { {  39 }, { /* 1 */ "IPN",      "IPN_POS",	             "IPN Position coords" } },
        { {  40 }, { /*   */ "HETE",     "HETE_ALERT_SRC",           "HETE Trigger Alert" } },
        { {  41 }, { /*   */ "HETE",     "HETE_UPDATE_SRC",          "HETE Update position (multiples)" } },
        { {  42 }, { /*   */ "HETE",     "HETE_FINAL_SRC",           "HETE Last/Final position" } },
        { {  43 }, { /*   */ "HETE",     "HETE_GNDANA_SRC",          "HETE position from Ground Analysis (HitL)" } },
        { {  44 }, { /* 1 */ "HETE",     "HETE_TEST",	             "HETE TEST" } },
        { {  45 }, { /* 1 */ "SOURCE",   "GRB_CNTRPART",             "GRB Counterpart coordinates" } },
        { {  46 }, { /* 1 */ "SWIFT",    "SWIFT_TOO_FOM",            "SWIFT TOO-form of the FOM" } },
        { {  47 }, { /* 1 */ "SWIFT",    "SWIFT_TOO_SC_SLEW",        "SWIFT TOO-form of the SC_SLEW" } },
        { {  48 }, { /* - */ "",         "DOW_TOD",	             "Day-of-Week Time-of-Day end2end testing" } },
        { {  50 }, { /* 1 */ "",         "spare50",	             "(spare; not yet assigned)" } },
        { {  51 }, { /* 1 */ "INTEGRAL", "INTEGRAL_POINTDIR",        "INTEGRAL Pointing Direction" } },
        { {  52 }, { /* 1 */ "INTEGRAL", "INTEGRAL_SPIACS",          "INTEGRAL SPIACS" } },
        { {  53 }, { /* 1 */ "INTEGRAL", "INTEGRAL_WAKEUP",          "INTEGRAL Wakeup" } },
        { {  54 }, { /* 1 */ "INTEGRAL", "INTEGRAL_REFINED",         "INTEGRAL Refined" } },
        { {  55 }, { /* 1 */ "INTEGRAL", "INTEGRAL_OFFLINE",         "INTEGRAL Offline (HitL)" } },
        { {  56 }, { /* 1 */ "INTEGRAL", "INTEGRAL_WEAK",            "INTEGRAL Weak" } },
        { {  57 }, { /* + */ "AAVSO",    "AAVSO",	             "AAVSO" } },
        { {  58 }, { /*   */ "MILAGRO",  "MILAGRO_POS",	             "MILAGRO Position" } },
        { {  59 }, { /* 1 */ "KONUS",    "KONUS_LC",	             "KONUS Lightcurve" } },
        { {  60 }, { /* 1 */ "BAT",      "SWIFT_BAT_GRB_ALERT",      "BAT ALERT. Never transmitted by the s/c." } },
        { {  61 }, { /* 1 */ "BAT",      "SWIFT_BAT_GRB_POS_ACK",    "BAT GRB Position Acknowledge" } },
        { {  62 }, { /* 1 */ "BAT",      "SWIFT_BAT_GRB_POS_NACK",   "BAT GRB Position NOT_Ack (pos not found)." } },
        { {  63 }, { /* 1 */ "BAT",      "SWIFT_BAT_GRB_LC",	     "BAT GRB Lightcurve" } },
        { {  64 }, { /* - */ "BAT",      "SWIFT_BAT_SCALEDMAP",      "BAT Scaled Map" } },
        { {  65 }, { /* 1 */ "SWIFT",    "SWIFT_FOM_OBS",	     "BAT FOM to Observe (FOM_2OBSAT)" } },
        { {  66 }, { /* 1 */ "SWIFT",    "SWIFT_SC_SLEW",	     "BAT S/C to Slew (FOSC_2OBSAT)" } },
        { {  67 }, { /* 1 */ "XRT",      "SWIFT_XRT_POSITION",       "XRT Position" } },
        { {  68 }, { /* - */ "XRT",      "SWIFT_XRT_SPECTRUM",       "XRT Spectrum" } },
        { {  69 }, { /* 1 */ "XRT",      "SWIFT_XRT_IMAGE",	     "XRT Image (aka postage stamp)" } },
        { {  70 }, { /* - */ "XRT",      "SWIFT_XRT_LC",	     "XRT Lightcurve (aka Prompt)" } },
        { {  71 }, { /* 1 */ "XRT",      "SWIFT_XRT_CENTROID",       "XRT Centroid Error (Pos Nack)" } },
        { {  72 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_DBURST",	     "UVOT DarkBurst (aka Neighbor, aka GeNie)" } },
        { {  73 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_FCHART",	     "UVOT Finding Chart" } },
        { {  76 }, { /* + */ "BAT",      "SWIFT_BAT_GRB_LC_PROC",    "BAT GRB Lightcurve processed" } },
        { {  77 }, { /* - */ "XRT",      "SWIFT_XRT_SPECTRUM_PROC",  "XRT Spectrum processed" } },
        { {  78 }, { /* 1 */ "XRT",      "SWIFT_XRT_IMAGE_PROC",     "XRT Image processed" } },
        { {  79 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_DBURST_PROC",   "UVOT DarkBurst proc mesg (aka Neighbor)" } },
        { {  80 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_FCHART_PROC",   "UVOT Finding Chart processed" } },
        { {  81 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_POS",	     "UVOT Position" } },
        { {  82 }, { /* 1 */ "BAT",      "SWIFT_BAT_GRB_POS_TEST",   "BAT GRB Position Test" } },
        { {  83 }, { /* 1 */ "SWIFT",    "SWIFT_POINTDIR",	     "Pointing Direction" } },
        { {  84 }, { /* 1 */ "BAT",      "SWIFT_BAT_TRANS",	     "BAT Hard X-ray Transient coords" } },
        { {  85 }, { /* - */ "XRT",      "SWIFT_XRT_THRESHPIX",      "XRT Thresholded-Pixel-list" } },
        { {  86 }, { /* - */ "XRT",      "SWIFT_XRT_THRESHPIX_PROC", "XRT Thresholded-Pixel-list processed" } },
        { {  87 }, { /* - */ "XRT",      "SWIFT_XRT_SPER",	     "XRT Single-Pixel-Event-Report" } },
        { {  88 }, { /* - */ "XRT",      "SWIFT_XRT_SPER_PROC",      "XRT Single-Pixel-Event-Report processed" } },
        { {  89 }, { /* 1 */ "UVOT",     "SWIFT_UVOT_POS_NACK",      "UVOT Position Nack (contains BATs/XRTs position)" } },
        { {  90 }, { /* - */ "BAT",      "SWIFT_BAT_ALARM_SHORT",    "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  91 }, { /* - */ "BAT",      "SWIFT_BAT_ALARM_LONG",     "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  92 }, { /* - */ "UVOT",     "SWIFT_UVOT_EMERGENCY",     "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  93 }, { /* - */ "XRT",      "SWIFT_XRT_EMERGENCY",      "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  94 }, { /* - */ "SWIFT",    "SWIFT_FOM_PPT_ARG_ERR",    "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  95 }, { /* - */ "SWIFT",    "SWIFT_FOM_SAFE_POINT",     "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  96 }, { /* - */ "SWIFT",    "SWIFT_FOM_SLEW_ABORT",     "SWIFT Appendix_C non-public (Team Ops)" } },
        { {  97 }, { /* 1 */ "BAT",      "SWIFT_BAT_QL_POS",	     "BAT Quick Look Position (1-6 sec sooner)" } },
        { {  98 }, { /* 1 */ "BAT",      "SWIFT_BAT_SUB_THRESHOLD",  "BAT Sub-Threshold Position" } },
        { {  99 }, { /* 1 */ "BAT",      "SWIFT_BAT_SLEW_POS",       "BAT Burst/Trans Pos during slewing" } },
        { { 100 }, { /* 1 */ "AGILE",    "AGILE_GRB_WAKEUP",	     "AGILE GRB Wake-Up Position" } },
        { { 101 }, { /* 1 */ "AGILE",    "AGILE_GRB_GROUND",	     "AGILE GRB Prompt Position" } },
        { { 102 }, { /* 1 */ "AGILE",    "AGILE_GRB_REFINED",	     "AGILE GRB Refined Position" } },
        { { 103 }, { /* 1 */ "SWIFT",    "SWIFT_ACTUAL_POINTDIR",    "Actual Pointing Direction" } },
        { { 107 }, { /* 1 */ "AGILE",    "AGILE_POINTDIR",	     "AGILE Pointing Direction" } },
        { { 108 }, { /* + */ "AGILE",    "AGILE_TRANS",	             "AGILE Transient Position" } },
        { { 109 }, { /* 1 */ "AGILE",    "AGILE_GRB_POS_TEST",       "AGILE GRB Position Test" } },
        { { 110 }, { /* 1 */ "GBM",      "FERMI_GBM_ALERT",	     "GBM Alert" } },
        { { 111 }, { /* 1 */ "GBM",      "FERMI_GBM_FLT_POS",	     "GBM Flightt-calculated Position" } },
        { { 112 }, { /* 1 */ "GBM",      "FERMI_GBM_GND_POS",	     "GBM Ground-calculated Position" } },
        { { 113 }, { /* + */ "GBM",      "FERMI_GBM_LC",	     "GBM Lightcurve" } },
        { { 114 }, { /* - */ "GBM",      "FERMI_GBM_GND_INTERNAL",   "GBM Gnd-calc Internal (beyond 112)" } },
        { { 115 }, { /* 1 */ "GBM",      "FERMI_GBM_FIN_POS",	     "GBM Final Position HitL or Offline" } },
        { { 118 }, { /* + */ "GBM",      "FERMI_GBM_TRANS",	     "GBM Transient Position" } },
        { { 119 }, { /* 1 */ "GBM",      "FERMI_GBM_POS_TEST",       "GBM Position Test" } },
        { { 120 }, { /* - */ "LAT",      "FERMI_LAT_POS_INI",	     "LAT Position Initial" } },
        { { 121 }, { /* 1 */ "LAT",      "FERMI_LAT_POS_UPD",	     "LAT Position Update" } },
        { { 122 }, { /* - */ "LAT",      "FERMI_LAT_POS_DIAG",       "LAT Position Diagnostic" } },
        { { 123 }, { /* + */ "LAT",      "FERMI_LAT_TRANS",	     "LAT Transient Position (previously unknown source)" } },
        { { 124 }, { /* 1 */ "LAT",      "FERMI_LAT_POS_TEST",       "LAT Position Test (like UPD only)" } },
        { { 125 }, { /* + */ "LAT",      "FERMI_LAT_MONITOR",	     "LAT Monitor (eg Blazar, AGN, etc)" } },
        { { 126 }, { /* 1 */ "FERMI",    "FERMI_SC_SLEW",	     "Spcecraft Slew" } },
        { { 127 }, { /* 1 */ "LAT",      "FERMI_LAT_GND",	     "LAT Ground-analysis refined Pos" } },
        { { 128 }, { /* + */ "LAT",      "FERMI_LAT_OFFLINE",	     "LAT Ground-analysis Trigger Pos, Offline" } },
        { { 129 }, { /* 1 */ "FERMI",    "FERMI_POINTDIR",	     "Pointing Direction" } },
        { { 130 }, { /* 1 */ "",         "SIMBADNED",	             "SIMBAD/NED Search Results" } },
        { { 131 }, { /* + */ "PIOTS",    "PIOTS_OT_POS",	     "Pi-Of-The-Sky Optical Transient Pos" } },
        { { 132 }, { /* + */ "KAIT",     "KAIT_SN",                  "KAIT SuperNova" } },
        { { 133 }, { /* 1 */ "KAIT",     "SWIFT_BAT_MONITOR",	     "Swift BAT Transient Monitor LC page event" } },
        { { 134 }, { /* 1 */ "MAXI",     "MAXI_UNKNOWN",	     "MAXI previously Unknown source transient (GRBs or other x-ray trans)" } },
        { { 135 }, { /* 1 */ "MAXI",     "MAXI_KNOWN",	             "MAXI previously Known source transient (already in some catalog)" } },
        { { 136 }, { /* 1 */ "MAXI",     "MAXI_TEST",	             "MAXI Test notice (for the Unknown type)" } },
        { { 137 }, { /* + */ "OGLE",     "OGLE",	             "OGLE lensing event (Inten, yes; but not Signif)" } },
        { { 138 }, { /* + */ "CBAT",     "CBAT",	             "CBAT" } },
        { { 139 }, { /* + */ "MOA",      "MOA",	                     "MOA lensing event (turn off inten for now!!!)" } },
        { { 140 }, { /* 1 */ "BAT",      "SWIFT_BAT_SUBSUB",	     "BAT SubSubThreshold trigger" } },
        { { 141 }, { /* 1 */ "BAT",      "SWIFT_BAT_KNOWN_SRC",      "Known source detected in ach BAT image" } },
        { { 142 }, { /* 1 */ "",         "VOE_1.1_IM_ALIVE",	     "I'm alive socket packet sent every 60 sec" } },
        { { 143 }, { /* 1 */ "",         "VOE_2.0_IM_ALIVE",	     "I'm alive socket packet sent every 60 sec" } },
        { { 145 }, { /* + */ "",         "COINCIDENCE",              "Temporal/Spatial coinc between mission-instruments" } },
        { { 148 }, { /* 1 */ "SUZAKU",   "SUZAKU_LC",	             "SUZAKU-WAM Lightcurve" } },
        { { 149 }, { /* 1 */ "SNEWS",    "SNEWS",                    "SNEWS Positions" } },
        { { 150 }, { /* p */ "LVC",      "LVC_PRELIM",               "LIGO/Virgo trigger alert (no position information)" } },
        { { 151 }, { /* p */ "LVC",      "LVC_INITIAL",              "LIGO/Virgo initial position (skymap)" } },
        { { 152 }, { /* p */ "LVC",      "LVC_UPDATE",               "LIGO/Virgo updates position (skymap)" } },
        { { 153 }, { /* + */ "LVC",      "LVC_TEST",                 "LIGO/Virgo test position (skymap)" } },
        { { 154 }, { /* p */ "LVC",      "LVC_CNTRPART",             "LVC multi-messenger Counterpart coordinates" } },
        { { 157 }, { /* 1 */ "AMON",     "AMON_ICECUBE_COINC",       "AMON ICECUBE temporal/spatial coincidence events" } },
        { { 158 }, { /* 1 */ "AMON",     "AMON_ICECUBE_HESE",        "AMON ICECUBE High Energy Single (neutrino) Event" } },
        //// reeived but missing
        { { 160 }, { /* 1 */ "CALET",    "CALET_GBM_FLT_LC",         "CALET-GBM Flight-produced Lightcurve" } },
        { { 161 }, { /* 1 */ "CALET",    "CALET_GBM_GND_LC",         "CALET-GBM Ground-produced Lightcurve" } },
        { { 164 }, { /* + */ "LVC",      "LVC_RETRACTION",           "LIGO/Virgo Retraction of a previous notice" } },
        { { 168 }, { /* p */ "GWHEN",    "GWHEN_COINC",              "Coincidence between LVC and ICECUBE High Energy Neutrino event" } },
        { { 169 }, { /* 1 */ "AMON",     "AMON_ICECUBE_EHE",         "AMON ICECUBE Extreme High Energy (neutrino) event" } },
        { { 171 }, { /* 1 */ "AMON",     "HAWC_BURST_MONITOR",       "HAWC GRBs" } },
        { { 173 }, { /* 1 */ "AMON",     "ICECUBE_GOLD",             "ICECUBE High Energy Single (neutrino) Event" } },
        { { 174 }, { /* 1 */ "AMON",     "ICECUBE_BRONZE",           "ICECUBE High Energy Single (neutrino) Event" } },
    };
}

#endif