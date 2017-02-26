/*
 * This file is part of uGB
 * Copyright (C) 2017  Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
