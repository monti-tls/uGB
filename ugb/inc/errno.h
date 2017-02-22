#ifndef __UGB_ERRNO_H__
#define __UGB_ERRNO_H__

enum
{
    #define DEF_ERRNO(value, id, string) UGB_ERR_ ## id = (value),
    #include "errno.def"
};

const char* ugb_strerror(int err);

#endif // __UGB_ERRNO_H__
