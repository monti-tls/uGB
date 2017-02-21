#include "errno.h"

static const char* _ugb_errstrs[-UGB_ERR_NERRNO] = {
    #define DEF_ERRNO(value, id, string) [value] = string,
};

const char* ugb_strerror(int errno)
{
    errno = -errno;
    if (errno < 0 || errno >= -UGB_ERR_NERRNO)
        return 0;

    return _ugb_errstrs[errno];
}
