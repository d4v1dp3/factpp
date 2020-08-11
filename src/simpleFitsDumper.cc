#include <map>
#include <vector>
#include <iostream>
#include <fstream>

#include <CCfits/CCfits>

using namespace std;

void writeValuesFromFits(vector<int>& offsets,ofstream& targetFile, unsigned char* fitsBuffer, vector<string> dumpList, map<string, CCfits::Column*>& colMap)
{
   targetFile.precision(20);
   map<string, CCfits::Column*>::iterator it;
   for (it=colMap.begin(); it != colMap.end(); it++)
    {
        bool found = false;
        for (vector<string>::iterator jt=dumpList.begin(); jt != dumpList.end(); jt++)
        {
            if (it->first == *jt)
            {
                found = true;
                break;
            }
        }
        if (!found)
            continue;
       int offset = offsets[it->second->index()-1];
       const char* charSrc = reinterpret_cast<char*>(&fitsBuffer[offset]);
        unsigned char copyBuffer[30];//max size of a single variable
        for (int width = 0; width<it->second->width(); width++)
        {
            switch (it->second->type())
            {
            case CCfits::Tbyte:
                targetFile << *charSrc;
                charSrc += sizeof(char);
            break;
            case CCfits::Tushort:
                reverse_copy(charSrc, charSrc+sizeof(unsigned short), copyBuffer);
                targetFile << *reinterpret_cast<const unsigned short*>(copyBuffer);
                charSrc += sizeof(char);
            break;
            case CCfits::Tshort:
                reverse_copy(charSrc, charSrc+sizeof(short), copyBuffer);
                targetFile << *reinterpret_cast<const short*>(copyBuffer);
                charSrc += sizeof(char);
            break;
            case CCfits::Tuint:
                reverse_copy(charSrc, charSrc+sizeof(unsigned int), copyBuffer);
                //warning suppressed in gcc4.0.2
                targetFile << *reinterpret_cast<unsigned int*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tint:
                reverse_copy(charSrc, charSrc+sizeof(int), copyBuffer);
                targetFile << *reinterpret_cast<int*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tulong:
                reverse_copy(charSrc, charSrc+sizeof(unsigned long), copyBuffer);
                targetFile << *reinterpret_cast<unsigned long*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tlong:
                reverse_copy(charSrc, charSrc+sizeof(long), copyBuffer);
                targetFile << *reinterpret_cast<long*>(copyBuffer);
                charSrc += sizeof(int);
            break;
            case CCfits::Tlonglong:
                reverse_copy(charSrc, charSrc+sizeof(long long), copyBuffer);
                targetFile << *reinterpret_cast<long long*>(copyBuffer);
                charSrc += sizeof(long long);
            break;
            case CCfits::Tfloat:
                reverse_copy(charSrc, charSrc+sizeof(float), copyBuffer);
                targetFile << *reinterpret_cast<float*>(copyBuffer);
                charSrc += sizeof(float);
            break;
            case CCfits::Tdouble:
                reverse_copy(charSrc, charSrc+sizeof(double), copyBuffer);
                targetFile << *reinterpret_cast<double*>(copyBuffer);
                charSrc += sizeof(double);
            break;
            case CCfits::Tnull:
            case CCfits::Tbit:
            case CCfits::Tlogical:
            case CCfits::Tstring:
            case CCfits::Tcomplex:
            case CCfits::Tdblcomplex:
            case CCfits::VTbit:
            case CCfits::VTbyte:
            case CCfits::VTlogical:
            case CCfits::VTushort:
            case CCfits::VTshort:
            case CCfits::VTuint:
            case CCfits::VTint:
            case CCfits::VTulong:
            case CCfits::VTlong:
            case CCfits::VTlonglong:
            case CCfits::VTfloat:
            case CCfits::VTdouble:
            case CCfits::VTcomplex:
            case CCfits::VTdblcomplex:
                cout << "Data type not implemented yet." << endl;
                return;
            break;
            default:
                cout << "THIS SHOULD NEVER BE REACHED" << endl;
                return;
            }//switch
            targetFile << " ";
        }//width loop
    }//iterator over the columns
    targetFile << endl;
}

// --------------------------------------------------------------------------
//
//! Calculates the required buffer size for reading one row of the current table.
//! Also calculates the offsets to all the columns
//
vector<int> CalculateBufferSize(map<string, CCfits::Column*>& colMap)
{
    vector<int> result;
    map<int,int> sizes;
    int size = 0;

    for (map<string, CCfits::Column*>::iterator it=colMap.begin(); it != colMap.end(); it++)
    {
        int width = it->second->width();
        switch (it->second->type())
        {
        case CCfits::Tbyte:
        case CCfits::Tushort:
        case CCfits::Tshort:
            sizes[it->second->index()] =  sizeof(char)*width;
        break;
        case CCfits::Tuint:
        case CCfits::Tint:
            sizes[it->second->index()] =  sizeof(int)*width;
        break;
        case CCfits::Tulong:
        case CCfits::Tlong:
            sizes[it->second->index()] = sizeof(int)*width;
        break;
        case CCfits::Tlonglong:
            sizes[it->second->index()] =  sizeof(long long)*width;
        break;
        case CCfits::Tfloat:
            sizes[it->second->index()] =  sizeof(float)*width;
        break;
        case CCfits::Tdouble:
            sizes[it->second->index()] =  sizeof(double)*width;
        break;
        case CCfits::Tnull:
        case CCfits::Tbit:
        case CCfits::Tlogical:
        case CCfits::Tstring:
        case CCfits::Tcomplex:
        case CCfits::Tdblcomplex:
        case CCfits::VTbit:
        case CCfits::VTbyte:
        case CCfits::VTlogical:
        case CCfits::VTushort:
        case CCfits::VTshort:
        case CCfits::VTuint:
        case CCfits::VTint:
        case CCfits::VTulong:
        case CCfits::VTlong:
        case CCfits::VTlonglong:
        case CCfits::VTfloat:
        case CCfits::VTdouble:
        case CCfits::VTcomplex:
        case CCfits::VTdblcomplex:
            cout << "Data type not implemented yet." << endl;
            return vector<int>();
        break;
        default:
            cout << "THIS SHOULD NEVER BE REACHED" << endl;
            return vector<int>();
        }
    }
    //calculate the offsets in the vector.
    int checkIndex = 1;
    for (map<int,int>::iterator it=sizes.begin(); it != sizes.end(); it++)
    {
        result.push_back(size);
        size += it->second;
        if (it->first != checkIndex)
        {
            cout << "Expected index " << checkIndex << " found " << it->first << endl;
        }
        checkIndex++;
    }
    result.push_back(size);
    return result;
}

int main(int argc, const char** argv)
{
    //set the names of the file and table to be loaded
    string fileNameToLoad = "test.fits";
    string tableNameToLoad = "FACT-TIME_ETIENNE";
    //set the vector of columns to be dumped
    vector<string> columnsToDump;
    columnsToDump.push_back("Data0");
    columnsToDump.push_back("Data1");
    //set the name of the output text file
    string outputFile = "output.txt";

    //load the fits file
    CCfits::FITS* file = NULL;
    try
    {
        file = new CCfits::FITS(fileNameToLoad);
    }
    catch (CCfits::FitsException e)
    {
         cout << "Could not open FITS file " << fileNameToLoad << " reason: " << e.message() << endl;
         return -1;
    }
    //check if the selected table indeed exists in the loaded file. If so, load it. Otherwise display the existing tables
    CCfits::Table* table;
    const multimap< string, CCfits::ExtHDU * > extMap = file->extension();
    if (extMap.find(tableNameToLoad) == extMap.end())
    {
        cout << "Could not open table " << tableNameToLoad << ". Tables in file are: " << endl;
        for (std::multimap<string, CCfits::ExtHDU*>::const_iterator it=extMap.begin(); it != extMap.end(); it++)
            cout << it->first << " ";
        cout << endl;
        delete file;
        return -1;
    }
    else
        table = dynamic_cast<CCfits::Table*>(extMap.find(tableNameToLoad)->second);
    int numRows = table->rows();
    //check that the given column names are indeed part of that table
    map<string, CCfits::Column*> colMap = table->column();
    if (columnsToDump.size() != 0)
    {
        for (vector<string>::iterator it=columnsToDump.begin(); it!= columnsToDump.end(); it++)
        {
            if (colMap.find(*it) == colMap.end())
            {
                cout << "Config-given dump list contains invalid entry " << *it << endl;
                delete file;
                return -1;
            }
        }
    }
    //dump the requested columns
    table->makeThisCurrent();
    vector<int> offsets = CalculateBufferSize(colMap);
    int size = offsets[offsets.size()-1];
    offsets.pop_back();
    unsigned char* fitsBuffer = new unsigned char[size];

    ofstream targetFile(outputFile.c_str());
    int status = 0;

    for (int i=1;i<=table->rows(); i++)
    {
        fits_read_tblbytes(file->fitsPointer(), i, 1, size, fitsBuffer, &status);
        if (status)
        {
            cout << "An error occurred while reading fits row #" << i << " error code: " << status << endl;
            for (unsigned int j=0;j<offsets.size(); j++)
                cout << offsets[j] << " ";
            cout << endl;
        }
        writeValuesFromFits(offsets, targetFile, fitsBuffer, columnsToDump, colMap);
    }
    delete file;
    return 0;
}
