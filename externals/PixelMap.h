#ifndef FACT_PixelMap
#define FACT_PixelMap

#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#ifdef DEBUG
#include <iostream>  // cerr -- to be removed?
#endif

#ifdef __EXCEPTIONS
#include <stdexcept>
#endif

#ifdef __MARS__
#include "MLog.h"
#include "MLogManip.h"
#endif

// FIXME: Replace 416 by BIAS::kNumChannels

struct PixelMapEntry
{
    int   index;               /// Software index
    int   cbpx;                /// Hardware index as CBPX
    int   gapd;                /// gAPD index
//    float Vgapd;               /// gAPD Bias voltage
    int   hv_board;            /// Bias suppply board
    int   hv_channel;

    PixelMapEntry() : index(-1) { } /// Bias supply channel

    int crate() const { return cbpx/1000; }
    int board() const { return (cbpx/100)%10; }
    int patch() const { return (cbpx/10)%10; }
    int pixel() const { return cbpx%10; }
    int hw() const    { return pixel()+patch()*9+board()*36+crate()*360; }
    int group() const { return pixel()>3; }
    int count() const { return pixel()>3 ? 5 : 4; }
    int hv() const    { return hv_channel+hv_board*32; }

    operator bool() const { return index>=0; }

    static const PixelMapEntry &empty() { const static PixelMapEntry e; return e; }
};

class PixelMap : public std::vector<PixelMapEntry>
{
public:
    PixelMap() : std::vector<PixelMapEntry>(1440)
    {
    }

    bool Read(const std::string &fname)
    {
        std::ifstream fin(fname);

        int l = 0;

        std::string buf;
        while (getline(fin, buf, '\n'))
        {
            if (l>1439)
                break;

            buf.erase(buf.find_last_not_of(' ')+1);         //surfixing spaces
            buf.erase(0, buf.find_first_not_of(' '));       //prefixing spaces
 
            if (buf.empty() || buf[0]=='#')
                continue;

            std::stringstream str(buf);

            int     idummy;
            float   fdummy;

            PixelMapEntry entry;

            str >> entry.index;
            str >> entry.cbpx;
            str >> idummy;
            str >> idummy;
            str >> entry.gapd;
            str >> fdummy; //entry.Vgapd;
            str >> entry.hv_board;
            str >> entry.hv_channel;
            //str >> fdummy;
            //str >> fdummy;
            //str >> fdummy;

            if (entry.hv_channel+32*entry.hv_board>=416/*BIAS::kNumChannels*/)
            {
#ifdef DEBUG
                cerr << "Invalid board/channel read from FACTmapV5.txt." << endl;
#endif
                return false;
            }

            (*this)[l++] = entry;
        }

        return l==1440;
    }

    const PixelMapEntry &index(int idx) const
    {
        for (std::vector<PixelMapEntry>::const_iterator it=begin(); it!=end(); it++)
            if (it->index==idx)
                return *it;
#ifdef DEBUG
        std::cerr << "PixelMap: index " << idx << " not found" << std::endl;
#endif
        return PixelMapEntry::empty();
    }

    const PixelMapEntry &cbpx(int c) const
    {
        for (std::vector<PixelMapEntry>::const_iterator it=begin(); it!=end(); it++)
            if (it->cbpx==c)
                return *it;
#ifdef DEBUG
        std::cerr << "PixelMap: cbpx " << c << " not found" << std::endl;
#endif
        return PixelMapEntry::empty();
    }

    const PixelMapEntry &cbpx(int c, int b, int p, int px) const
    {
        return cbpx(px + p*10 + b*100 + c*1000);
    }

    const PixelMapEntry &hw(int idx) const
    {
        return cbpx(idx/360, (idx/36)%10, (idx/9)%4, idx%9);
    }

    const PixelMapEntry &hv(int board, int channel) const
    {
        for (std::vector<PixelMapEntry>::const_iterator it=begin(); it!=end(); it++)
            if (it->hv_board==board && it->hv_channel==channel)
                return *it;
#ifdef DEBUG
        std::cerr << "PixelMap: hv " << board << "/" << channel << " not found" << std::endl;
#endif
        return PixelMapEntry::empty();
    }

    const PixelMapEntry &hv(int idx) const
    {
        return hv(idx/32, idx%32);
    }

    /*
    float Vgapd(int board, int channel) const
    {
        float avg = 0;
        int   num = 0;

        for (std::vector<PixelMapEntry>::const_iterator it=begin(); it!=end(); it++)
            if (it->hv_board==board && it->hv_channel==channel)
            {
                avg += it->Vgapd;
                num ++;
            }

        return num==0 ? 0 : avg/num;
    }

    float Vgapd(int idx) const
    {
        return Vgapd(idx/32, idx%32);
    }

    std::vector<float> Vgapd() const
    {
        std::vector<float> avg(416);
        std::vector<int>   num(416);

        for (std::vector<PixelMapEntry>::const_iterator it=begin(); it!=end(); it++)
        {
            const int ch = it->hv_board*32 + it->hv_channel;

            avg[ch] += it->Vgapd;
            num[ch] ++;
        }

        for (int ch=0; ch<416; ch++)
        {
            if (num[ch])
                avg[ch] /= num[ch];
        }

        return avg;
    }*/
};

struct BiasMapEntry
{
    int   hv_board;            /// Bias suppply board
    int   hv_channel;          /// Bias supply channel
    float Vnom;                /// Channel bias voltage nominal
    float Voff;                /// Channel bias voltage offset [V]
    float Vslope;              /// Channel bias voltage slope  [Ohm]

    BiasMapEntry() : hv_board(-1), Voff(0), Vslope(90000) { }

    int hv() const { return hv_channel+hv_board*32; }

    operator bool() const { return hv_board>=0; }

    static const BiasMapEntry &empty() { const static BiasMapEntry e; return e; }
};

class BiasMap : public std::vector<BiasMapEntry>
{
public:
    BiasMap() : std::vector<BiasMapEntry>(416)
    {
    }

#ifndef __MARS__
    void Retrieve(const std::string &database);
#endif
    bool Read(const std::string &fname)
    {
        std::ifstream fin(fname);

        int l = 0;

        std::string buf;
        while (getline(fin, buf, '\n'))
        {
            if (l>416)
                break;

            buf.erase(buf.find_last_not_of(' ')+1);         //surfixing spaces
            buf.erase(0, buf.find_first_not_of(' '));       //prefixing spaces

            if (buf.empty() || buf[0]=='#')
                continue;

            std::stringstream str(buf);

            BiasMapEntry entry;

            str >> entry.hv_board;
            str >> entry.hv_channel;
            str >> entry.Vnom;
            str >> entry.Voff;
            str >> entry.Vslope;

#ifdef __EXCEPTIONS
            if (entry.hv_channel+32*entry.hv_board>=416)
                throw std::runtime_error("Invalid board/channel read from "+fname+".");
#endif
#ifdef __MARS__
            if (entry.hv_channel+32*entry.hv_board>=416)
            {
                gLog << err << "Invalid board/channel read from " << fname << "." << std::endl;
                return false;
            }
#endif

            (*this)[entry.hv()] = entry;

            l++;
        }

#ifdef __EXCEPTIONS
        if (l!=416)
	  throw std::runtime_error("Number of lines ("+std::to_string(static_cast<long long>(l))+") read from "+fname+" does not match 416.");

        if (size()!=416)
            throw std::runtime_error("Number of entries read from "+fname+" does not match 416.");
#endif

#ifdef __MARS__
        if (l!=416)
        {
            gLog << err  << "Number of lines read from " << fname << " does not match 416." << std::endl;
            return false;
        }

        if (size()!=416)
        {
            gLog << "Number of entries read from " << fname << " does not match 416." << std::endl;
            return false;
        }
#endif

        return true;
    }

    const BiasMapEntry &hv(int board, int channel) const
    {
        for (std::vector<BiasMapEntry>::const_iterator it=begin(); it!=end(); it++)
            if (it->hv_board==board && it->hv_channel==channel)
                return *it;
#ifdef DEBUG
        std::cerr << "PixelMap: hv " << board << "/" << channel << " not found" << std::endl;
#endif
        return BiasMapEntry::empty();
    }

    const BiasMapEntry &hv(int idx) const
    {
        return hv(idx/32, idx%32);
    }

    const BiasMapEntry &hv(const PixelMapEntry &p) const
    {
        return hv(p.hv_board, p.hv_channel);
    }

    /*
    float Vgapd(int board, int channel) const
    {
        const BiasMapEntry &entry = hv(board, channel);
        return entry.Vnom - entry.Voff; // use this with GAPDmap_20111126.txt
    }

    float Vgapd(int idx) const
    {
        return Vgapd(idx/32, idx%32);
    }*/

    std::vector<float> Vgapd() const
    {
        std::vector<float> volt(416);

        for (std::vector<BiasMapEntry>::const_iterator it=begin(); it!=end(); it++)
        {
            const int ch = it->hv_board*32 + it->hv_channel;
	    volt[ch] = it->Vnom;
        }

        return volt;
    }
    std::vector<float> Voffset() const
    {
        std::vector<float> volt(416);

        for (std::vector<BiasMapEntry>::const_iterator it=begin(); it!=end(); it++)
        {
            const int ch = it->hv_board*32 + it->hv_channel;
	    volt[ch] = it->Voff;
        }

        return volt;
    }

    std::vector<float> Vslope() const
    {
        std::vector<float> slope(416);

        for (std::vector<BiasMapEntry>::const_iterator it=begin(); it!=end(); it++)
        {
            const int ch = it->hv_board*32 + it->hv_channel;
            slope[ch] = it->Vslope;
        }

        return slope;
    }
};

#endif
