#include "vhfile.hpp"

/**
 *
 */
VHFile::VHFile() : file(nullptr) {}

/**
 *
 */
VHFile::VHFile(const std::string &filename)
{
    assign(filename);
}

/**
 *
 */
VHFile::~VHFile()
{
    closefile();
}

/**
 *
 */
void VHFile::assign(const std::string &filename)
{
    fname = filename;
}

/**
 *
 */
int VHFile::flen()
{
    int r = -1;
    if (!openfile())
    {
        dmperr();
        return r;
    }
    if (!fseek(file, 0, SEEK_END))
    {
        r = ftell(file);
    }
    closefile();
    return r;
}

/**
 *
 */
bool VHFile::read(void *ptr, int sz, int seekoffs)
{
    if (!openfile())
    {
        dmperr();
        return false;
    }
    if (seekoffs)
    {
        if (fseek(file, seekoffs, SEEK_SET) != 0)
        {
            closefile();
            return false;
        }
    }
    int result = fread(ptr, sz, 1, file);
    closefile();
    return (result < 1) ? false : true;
}

/**
 *
 */
bool VHFile::write(void *ptr, int sz)
{
    if (!createfile())
    {
        dmpwerr();
        return false;
    }
    int result = fwrite(ptr, sz, 1, file);
    closefile();
    return result == 1;
}

/**
 *
 */
bool VHFile::openfile()
{
    file = fopen(fname.c_str(), "r");
    return file != NULL;
}

/**
 *
 */
bool VHFile::createfile()
{
    file = fopen(fname.c_str(), "w");
    return file != NULL;
}

/**
 *
 */
bool VHFile::closefile()
{
    bool v = (file != NULL);
    if (v)
    {
        fclose(file);
        file = NULL;
    }
    return v;
}

/**
 *
 */
void VHFile::dmperr()
{
    printf("ERROR: File (%s) cannot be opened.\n", fname.c_str());
}

/**
 *
 */
void VHFile::dmpwerr()
{
    printf("ERROR: File (%s) cannot be created.\n", fname.c_str());
}
