/**
 * Copyright (C) 2017  Sergey Morozov <sergey94morozov@gmail.com>
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

#define _LARGEFILE64_SOURCE    /* needed for off64_t type */
#define _GNU_SOURCE            /* needed for RTLD_NEXT */

#include <dlfcn.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <attr/xattr.h>

#include "defs.h"
#include "syms.h"
#include "queue.h"

/* enum of supported extended attributes */
enum xattr_enum {
        XATTRS(ENUMERIZE, COMMA),
};

/* declare array of extended attributes' keys */
static const char *xattr_str[] = {
        XATTRS(XATTR_KEY, COMMA),
};

/* pointer to the first priority download queue in shared memory */
static queue_t *queue = NULL;

/* functions for which this library has wrappers */
static symbols_t symbols = { 0 };

symbols_t *get_syms( void ) {
        return &symbols;
}

/* This function creates a table of pointers to glibc functions for all
 * of the io system calls so they can be called when needed
 */
void __attribute__ ((constructor)) init_syms(void) {
        symbols.open       = dlsym( RTLD_NEXT, "open"       );
        symbols.open64     = dlsym( RTLD_NEXT, "open64"     );
        symbols.openat     = dlsym( RTLD_NEXT, "openat"     );
        symbols.openat64   = dlsym( RTLD_NEXT, "openat64"   );
        symbols.truncate   = dlsym( RTLD_NEXT, "truncate"   );
        symbols.truncate64 = dlsym( RTLD_NEXT, "truncate64" );
        symbols.stat       = dlsym( RTLD_NEXT, "stat"       );
        symbols.stat64     = dlsym( RTLD_NEXT, "stat64"     );
        symbols.fstatat    = dlsym( RTLD_NEXT, "fstatat"    );
        symbols.fstatat64  = dlsym( RTLD_NEXT, "fstatat64"  );
        symbols.futimesat  = dlsym( RTLD_NEXT, "futimesat"  );
        symbols.utimes     = dlsym( RTLD_NEXT, "utimes"     );
        symbols.utime      = dlsym( RTLD_NEXT, "utime"      );
        symbols.fadvise    = dlsym( RTLD_NEXT, "fadvise"    );
        symbols.fadvise64  = dlsym( RTLD_NEXT, "fadvise64"  );

        symbols.fopen      = dlsym( RTLD_NEXT, "fopen"      );
        symbols.freopen    = dlsym( RTLD_NEXT, "freopen"    );
}


/**
 * @brief is_file_local Check a location of file (local or remote).
 *
 * @note Operation is atomic according to
 *       http://man7.org/linux/man-pages/man7/xattr.7.html.
 *
 * @param[in] path Path to file to check location.
 *
 * @return  1: if file is in local storage
 *          0: if file is in remote storage
 *         -1: error happen during an attempt to get extended attribute's value
 */
int is_file_local( const char *path ) {

        if ( getxattr( path, xattr_str[e_stub], NULL, 0 ) == -1 ) {
                if (    ( errno == ENOATTR )
                     || ( errno == ENOTSUP )
                     || ( errno == ERANGE ) ) {
                        /* if ENOTSUP then the file not in our target filesystem
                           and we not manage lifecycle of this file,
                           if ERANGE then e_stub attribute should have
                           a value which is not possible according to then
                           logic of this program and if ENOATTR then the file is
                           definitely local by our convention which means that
                           we can safetly report that file is local */
                        return 1;
                } else {
                        /* errors from stat(2) which should be handled
                           separately in each public libc wrapper function
                           so we return the error here */
                        return -1;
                }
        }

        /* e_stub atribute is set which means that file is remote */
        return 0;
}
