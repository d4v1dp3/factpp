/*
 * fitsHacker.cc
 *
 *  Created on: Sep 8, 2011
 *      Author: lyard
 */

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cstring>

using namespace std;
/*
 * Usage: fitsHacker <nameOfFileToHack> <numberOfBytesToSkip> <WhichCharactersToPutAfterShift>(optionnal)
 *
 *
 *
 */

enum ModeT {seekingHDU,
            foundHDU,
            fixedEND,
            reachedHeaderEnd};

int main(int argc, char** argv)
{
    if (argc < 2)
        return 0;

/* ENDfixer */
    fstream file(argv[1]);

    char c[81];
    c[80] = 0;
    int seeking=0;

    ModeT mode = seekingHDU;

    bool reallyFixedEnd = false;
    int endAddress = 0;

    while (mode != fixedEND)
    {
        file.read(c, 80);
        if (!file.good()) {
            cout << 0;
            return 0;
        }
        string str(c);
//        cout << c << endl;
        if (str.substr(0, 9) == "XTENSION=")
            mode = foundHDU;

        if (mode == foundHDU && str=="END                                                                             ")
        {
            mode = fixedEND;
            endAddress = seeking;
//            cout << "found END at " << endAddress << endl;
        }
        if (mode == foundHDU && str =="                                                                                ")
        {
            file.seekp(seeking);
            file.put('E');
            file.put('N');
            file.put('D');
            mode = fixedEND;
            reallyFixedEnd = true;
            endAddress = seeking;
//            cout << "added END at " << endAddress << endl;
        }

        seeking+=80;
    }

    file.seekp(seeking-1);
    while (mode != reachedHeaderEnd)
    {
        file.read(c, 80);
        if (!file.good()) {
            cout << 0;
            return 0;
        }
        string str(c);

        if (str =="                                                                                ")
            seeking+=80;
        else
            mode = reachedHeaderEnd;
    }

    file.close();

    if (seeking % 2880 != 0)
    {
        cout << "Error: header length not acceptable" << endl;
        return 0;
    }

    if (((seeking - endAddress)/80) > 35)
    {
        cout << "Error: too much header space after END keyword" << endl;
        return 0;
    }

    cout << seeking;

    return seeking;

/* FITS HACKER
    file.get(data, shift);

    for (int i=0;i<shift;i++)
    {
        if (i%80 == 0)
            cout << "||| " << endl;
        cout << data[i];
    }
    cout << endl;
    if (argc < 4)
        return 0;

    int length = strlen(argv[3]);


    file.seekp(shift-1);
    for (int i=0;i<length;i++)
        file.put(argv[3][i]);

    file.close();

    delete[] data;*/
}
