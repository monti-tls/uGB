#include "errno.h"

static const char* _ugb_errstrs[-UGB_ERR_NERRNO+1] = {
    #define DEF_ERRNO(value, id, string) [-value] = string,
    #include "errno.def"
};

const char* ugb_strerror(int err)
{
    if (err > 0 || err <= UGB_ERR_NERRNO)
        return 0;

    return _ugb_errstrs[-err];
}
