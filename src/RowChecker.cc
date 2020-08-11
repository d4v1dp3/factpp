/*
 * RowChecker.cc
 *
 *  Created on: Dec 20, 2011
 *      Author: lyard
 */

#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <sstream>

using namespace std;


//usage RowChecker <name of file> <size of header> <size of line> <mjdref> <givenLines>
int main(int argc, char** argv)
{

    if (argc < 6)
        return 0;

    fstream file(argv[1]);

    int headLen = atoi(argv[2]);
    int lineWidth = atoi(argv[3]);
    double mjdRef = atof(argv[4]);
    int numLines = atoi(argv[5]);

    int totalBytes = headLen;
    file.seekp(headLen);

    char* buf = new char[lineWidth];

    double currentTime = 0;
    char timeBuf[16];
    int realNumRows = 0;

    while (file.read(buf, lineWidth))
    {
        timeBuf[0] = buf[7];
        timeBuf[1] = buf[6];
        timeBuf[2] = buf[5];
        timeBuf[3] = buf[4];
        timeBuf[4] = buf[3];
        timeBuf[5] = buf[2];
        timeBuf[6] = buf[1];
        timeBuf[7] = buf[0];
        currentTime = reinterpret_cast<double*>(timeBuf)[0];

        if (realNumRows >= numLines)
        {
            if (currentTime + mjdRef > 60000 || currentTime + mjdRef < 10000)
                break;
            if (currentTime + mjdRef > 20000 && currentTime + mjdRef < 50000)
                break;
        }
//fix the time column if required.
        if (currentTime > 50000 && currentTime < 60000)
        {
            currentTime -= 40587;
            reinterpret_cast<double*>(timeBuf)[0] = currentTime;
            file.seekp(totalBytes);
            file.put(timeBuf[7]);
            file.put(timeBuf[6]);
            file.put(timeBuf[5]);
            file.put(timeBuf[4]);
            file.put(timeBuf[3]);
            file.put(timeBuf[2]);
            file.put(timeBuf[1]);
            file.put(timeBuf[0]);
            file.seekp(totalBytes + lineWidth);
        }

        realNumRows++;
        totalBytes += lineWidth;
    }
    //now update the number of lines of the file
    file.close();
    file.open(argv[1]);
    file.seekp(2880);
    delete[] buf;
    buf = new char[81];
    buf[80] = 0;
    bool changeDone = false;
    int seeked = 2880;
    if (realNumRows == numLines)
        changeDone = true;

    while (file.good() && !changeDone)
    {
        file.read(buf, 80);
        string str(buf);

        if (str.substr(0,9) == "NAXIS2  =")
        {
            ostringstream ss;
            ss << realNumRows;
            file.seekp(seeked + 30 - ss.str().size());
            for (int i=0;i<ss.str().size();i++)
                file.put(ss.str()[i]);
            changeDone = true;
            break;
        }
        seeked += 80;
    }
    if (!changeDone)
        cout << -1;
    else
        cout << realNumRows;
    file.close();
    return realNumRows;
}
