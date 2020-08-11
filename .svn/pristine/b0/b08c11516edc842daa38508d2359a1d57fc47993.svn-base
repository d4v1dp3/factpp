#include "Writer.h"

#include <iostream> // cout
#include <fstream>  // ofstream

#include <TVector2.h>

#include <stdio.h>    // FILE
#include <png.h>

#include "MTime.h"

ClassImp(Writer);

using namespace std;

void Writer::Png(const char *fname, const byte *buf,
                 struct timeval *date, const TVector2 xy)
{
    MTime t(*date);
    TString mjd;
    mjd += t.GetMjd()-52000;
    mjd = mjd.Strip(TString::kBoth);
    if (mjd.Length()<10)
        mjd.Append('0', 10-mjd.Length());

    TString pos;
    pos += xy.X();
    pos = pos.Strip(TString::kBoth);
    pos +="_";
    TString posy;
    posy += xy.Y();
    posy = posy.Strip(TString::kBoth);
    pos +=posy;

    TString name = fname;
    name += "_";
    name += mjd;
    name += "_";
    name += pos;
    name += ".png";

    cout << "Writing PNG '" << name << "'" << endl;

    //
    // open file
    //
    FILE *fd = fopen(name, "w");
    if (!fd)
    {
        cout << "Warning: Cannot open file for writing." << endl;
        return;
    }

    //
    // allocate memory
    //
    png_structp fPng = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               NULL, NULL, NULL);

    if (!fPng)
    {
        cout << "Warning: Unable to create PNG structure" << endl;
        fclose(fd);
        return;
    }


    png_infop fInfo = png_create_info_struct(fPng);

    if (!fInfo)
    {
        cout << "Warning: Unable to create PNG info structure" << endl;
        png_destroy_write_struct (&fPng, NULL);
        fclose(fd);
        return;
    }

    fInfo->width      = 768;
    fInfo->height     = 576;
    fInfo->bit_depth  = 8;
    fInfo->color_type = PNG_COLOR_TYPE_GRAY;

    //
    // set jump-back point in case of errors
    //
    if (setjmp(fPng->jmpbuf))
    {
        cout << "longjmp Warning: PNG encounterd an error!" << endl;
        png_destroy_write_struct (&fPng, &fInfo);
        fclose(fd);
        return;
    }

    //
    // connect file to PNG-Structure
    //
    png_init_io(fPng, fd);

    // png_set_compression_level (fPng, Z_BEST_COMPRESSION);

    //
    // Write header
    //
    png_write_info (fPng, fInfo);

    //
    // Write Time Chunks
    //
    /*
    if (date)
    {
        char text[36];

        Timer timet(date);
        sprintf(text, "*** %s ***", timet.GetTimeStr());
        png_write_chunk(fPng, (png_byte*)"UTC", (png_byte*)text, strlen(text));
        sprintf(text,"*** %s %s %.1f %i ***", tzname[0], tzname[1], 1.0/3600*timezone, daylight);
        png_write_chunk(fPng, (png_byte*)"ZONE", (png_byte*)text, strlen(text));
        }
        */

    //
    // Write bitmap data
    //
    for (unsigned int y=0; y<768*576; y+=768)
	png_write_row (fPng, (png_byte*)buf+y);

    //
    // Write footer
    //
    png_write_end (fPng, fInfo);

    //
    // free memory
    //
    png_destroy_write_struct (&fPng, &fInfo);

    fclose(fd);
}

void Writer::Ppm(const char *fname, const byte *img, struct timeval *date, const TVector2 xy)
{
    TString name = fname;

    MTime t(*date);

    TString pos;
    pos += xy.X();
    pos = pos.Strip(TString::kBoth);
    pos +="_";
    TString posy;
    posy += xy.Y();
    posy = posy.Strip(TString::kBoth);
    pos +=posy;

    name += "_";
    name += t.GetMjd()-52000;
    name += "_";
    name += pos;  
    name += ".ppm";

    cout << "Writing PPM '" << name << "'" << endl;

    //
    // open file for writing
    //
    ofstream fout(name);
    if (!fout)
    {
        cout << "Warning: Cannot open file for writing." << endl;
        return;
    }

    //
    // write buffer to file
    //
    fout << "P6\n768 576\n255\n";
    for (byte const *buf = img; buf < img+768*576; buf++)
        fout << *buf << *buf << *buf;
}
