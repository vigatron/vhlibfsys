#pragma once

#include <stdint.h>
#include <string>

class VHFile
{

public:
    //
    VHFile();

    //
    VHFile(const std::string & filename);

    //
    ~VHFile();

    //
    void assign(const std::string & filename);

    //
    int flen();

    //
    bool read(void *ptr, int sz, int seekoffs = 0);

    //
    bool write(void *ptr, int sz);

private:
    //
    bool openfile();
    //
    bool createfile();
    //
    bool closefile();
    //
    void dmperr();
    //
    void dmpwerr();

    std::string fname;
    FILE *file;
};
