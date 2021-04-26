#include "eb_file.h"
#include "basic.h"

int eb_file_exists(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return 1;
}

int eb_is_directory(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return S_ISDIR(st.st_mode);
}

int eb_is_file(const char *name)
{
    struct stat st;
    if (_stat(name, &st))
        return 0;
    else
        return S_ISREG(st.st_mode);
}
