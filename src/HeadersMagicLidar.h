#ifndef FACT_HeadersMagicLidar
#define FACT_HeadersMagicLidar

namespace MagicLidar
{
    namespace State
    {
        enum states_t
        {
            kDisconnected = 1,
            kConnected,
            kReceiving,
        };
    }

    struct DimLidar
    {
        DimLidar() { memset(this, 0, sizeof(DimLidar)); }

        float fZd;
        float fAz;
        //float fCHE;
        //float fCOT;
        //float fPBL;

        float fT3;
        float fT6;
        float fT9;
        float fT12;

        float fCloudBaseHeight;

    } __attribute__((__packed__));
}

#endif
