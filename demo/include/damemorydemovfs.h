/* Based on demovfs.h (https://www.sqlite.org/src/doc/trunk/src/test_demovfs.c) */

/*
** 2010 April 7
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** -----------------------------------------------------------------------------
**
**   The following VFS features are omitted:
**
**     1. File locking. The user must ensure that there is at most one
**        connection to each database when using this VFS. Multiple
**        connections to a single shared-cache count as a single connection
**        for the purposes of the previous statement.
**
**     2. The loading of dynamic extensions (shared libraries).
**
**     3. Temporary files. The user must configure SQLite to use in-memory
**        temp files when using this VFS. The easiest way to do this is to
**        compile with:
**
**          -DSQLITE_TEMP_STORE=3
**
**     4. File truncation. As of version 3.6.24, SQLite may run without
**        a working xTruncate() call, providing the user does not configure
**        SQLite to use "journal_mode=truncate", or use both
**        "journal_mode=persist" and ATTACHed databases.
*/

#if !defined(SQLITE_TEST) || SQLITE_OS_UNIX

#include "sqlite3.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <damemory/damemory.h>

#define DBFILESIZE (0x10UL * 0x200000UL)

/*
** The maximum pathname length supported by this VFS.
*/
#define MAXPATHNAME 128

#if 0
#include <stdio.h>
#define DEBUG(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG(fmt, ...) (void)(0)
#endif

/*
** When using this VFS, the sqlite3_file* handles that SQLite uses are
** actually pointers to instances of type DemoFile.
*/
typedef struct DemoFile DemoFile;
struct DemoFile
{
    sqlite3_file base; /* Base class. Must be first. */

    damemory_id id;
    damemory_handle handle;
};

/*
** Close a file.
*/
static int demoClose(sqlite3_file* pFile)
{
    DemoFile* p = (DemoFile*)pFile;

    DEBUG("close: %llu\n", p->id);

    size_t filesize = damemory_handle_size(p->handle);
    if (damemory_persist(p->handle, 0, filesize))
        return SQLITE_IOERR_CLOSE;

    return damemory_unmap(p->handle) ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

static int test_access_valid(DemoFile* p, int iAmt, sqlite_int64 iOfst)
{
    size_t filesize = damemory_handle_size(p->handle);
    return !(filesize < (size_t)iAmt || (size_t)iOfst > filesize - (size_t)iAmt);
}

static size_t get_size(damemory_handle handle)
{
    size_t size = 0;
    memcpy(&size, damemory_handle_addr(handle), sizeof(size));
    return size;
}

static void set_size(damemory_handle handle, size_t size)
{
    memcpy(damemory_handle_addr(handle), &size, sizeof(size));
}

static char* get_filecontent_addr(damemory_handle handle, sqlite_int64 iOfst)
{
    char* addr = (char*)damemory_handle_addr(handle);
    return addr + sizeof(size_t) + iOfst + 8;
}

/*
** Read data from a file.
*/
static int demoRead(sqlite3_file* pFile, void* zBuf, int iAmt, sqlite_int64 iOfst)
{
    DemoFile* p = (DemoFile*)pFile;

    if (!test_access_valid(p, iAmt, iOfst))
        return SQLITE_IOERR_READ;

    DEBUG("read: %llu, %d, %lld, %lu\n", p->id, iAmt, iOfst, get_size(p->handle));
    memcpy(zBuf, get_filecontent_addr(p->handle, iOfst), (size_t)iAmt);

    return SQLITE_OK;
}

/*
** Write data to a crash-file.
*/
static int demoWrite(sqlite3_file* pFile, const void* zBuf, int iAmt, sqlite_int64 iOfst)
{
    DemoFile* p = (DemoFile*)pFile;

    if (!test_access_valid(p, iAmt, iOfst))
        return SQLITE_IOERR_WRITE;

    size_t size = get_size(p->handle);

    DEBUG("write: %llu, %d, %lld, %lu\n", p->id, iAmt, iOfst, size);
    memcpy(get_filecontent_addr(p->handle, iOfst), zBuf, (size_t)iAmt);

    set_size(p->handle, MAX(size, (size_t)(iAmt + iOfst)));

    return SQLITE_OK;
}

/*
** Truncate a file. This is a no-op for this VFS (see header comments at
** the top of the file).
*/
static int demoTruncate(sqlite3_file*, sqlite_int64)
{
#if 0
      if( ftruncate(((DemoFile *)pFile)->fd, size) ) return SQLITE_IOERR_TRUNCATE;
#endif
    return SQLITE_OK;
}

/*
** Sync the contents of the file to the persistent media.
*/
static int demoSync(sqlite3_file* pFile, int)
{
    DemoFile* p = (DemoFile*)pFile;

    DEBUG("sync: %llu\n", p->id);

    size_t filesize = damemory_handle_size(p->handle);
    return damemory_persist(p->handle, 0, filesize) ? SQLITE_IOERR_FSYNC : SQLITE_OK;
}

/*
** Write the size of the file in bytes to *pSize.
*/
static int demoFileSize(sqlite3_file* pFile, sqlite_int64* pSize)
{
    DemoFile* p = (DemoFile*)pFile;

    *pSize = get_size(p->handle);
    return SQLITE_OK;
}

/*
** Locking functions. The xLock() and xUnlock() methods are both no-ops.
** The xCheckReservedLock() always indicates that no other process holds
** a reserved lock on the database file. This ensures that if a hot-journal
** file is found in the file-system it is rolled back.
*/
static int demoLock(sqlite3_file*, int)
{
    return SQLITE_OK;
}
static int demoUnlock(sqlite3_file*, int)
{
    return SQLITE_OK;
}
static int demoCheckReservedLock(sqlite3_file*, int* pResOut)
{
    *pResOut = 0;
    return SQLITE_OK;
}

/*
** No xFileControl() verbs are implemented by this VFS.
*/
static int demoFileControl(sqlite3_file*, int, void*)
{
    return SQLITE_NOTFOUND;
}

/*
** The xSectorSize() and xDeviceCharacteristics() methods. These two
** may return special values allowing SQLite to optimize file-system
** access to some extent. But it is also safe to simply return 0.
*/
static int demoSectorSize(sqlite3_file*)
{
    return 0;
}
static int demoDeviceCharacteristics(sqlite3_file*)
{
    return 0;
}

static damemory_id path_to_id(const char* zPath)
{
    damemory_id id = 0;
    memccpy(&id, zPath, 0, sizeof(id));
    DEBUG("  id = %llu\n", id);
    return id;
}

/*
** Open a file handle.
*/
static int demoOpen(sqlite3_vfs*,        /* VFS */
                    const char* zName,   /* File to open, or 0 for a temp file */
                    sqlite3_file* pFile, /* Pointer to DemoFile struct to populate */
                    int flags,           /* Input SQLITE_OPEN_XXX flags */
                    int* pOutFlags       /* Output SQLITE_OPEN_XXX flags (or NULL) */
)
{
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    static const sqlite3_io_methods demoio = {
        1,                        /* iVersion */
        demoClose,                /* xClose */
        demoRead,                 /* xRead */
        demoWrite,                /* xWrite */
        demoTruncate,             /* xTruncate */
        demoSync,                 /* xSync */
        demoFileSize,             /* xFileSize */
        demoLock,                 /* xLock */
        demoUnlock,               /* xUnlock */
        demoCheckReservedLock,    /* xCheckReservedLock */
        demoFileControl,          /* xFileControl */
        demoSectorSize,           /* xSectorSize */
        demoDeviceCharacteristics /* xDeviceCharacteristics */
    };
#pragma GCC diagnostic pop

    DemoFile* p = (DemoFile*)pFile; /* Populate this structure */

    if (zName == 0) {
        return SQLITE_IOERR;
    }

    damemory_id id = path_to_id(zName);
    DEBUG("open: %s (%llu)\n", zName, id);

    int created = 0;

    damemory_handle handle;
    if (!(flags & SQLITE_OPEN_EXCLUSIVE)) {
        handle = damemory_map(id);
        if (!damemory_handle_valid(handle) && !(flags & SQLITE_OPEN_CREATE))
            return SQLITE_CANTOPEN;
    }

    if ((flags & SQLITE_OPEN_EXCLUSIVE) || !damemory_handle_valid(handle)) {
        damemory_properties properties = {
            .access_pattern = DAMEMORY_ACCESSPATTERN_RANDOM,
            .bandwidth      = DAMEMORY_BANDWIDTH_VERYLOW,
            .granularity    = DAMEMORY_GRANULARITY_SMALL,
            .avg_latency    = DAMEMORY_AVGLATENCY_VERYLOW,
            .tail_latency   = DAMEMORY_TAILLATENCY_VERYLOW,
            .persistency    = DAMEMORY_PERSISTENCY_PERSISTENT,

        };
        handle = damemory_alloc(id, DBFILESIZE, &properties);

        if (!damemory_handle_valid(handle))
            return SQLITE_CANTOPEN;

        created = 1;
    }

    if (damemory_anchor(id)) {
        damemory_unmap(handle);
        return SQLITE_CANTOPEN;
    }

    memset(p, 0, sizeof(DemoFile));
    p->id     = id;
    p->handle = handle;

    if (created)
        set_size(handle, 0);

    DEBUG("  size: %ld\n", get_size(handle));

    if (pOutFlags) {
        *pOutFlags = flags;
    }
    p->base.pMethods = &demoio;
    return SQLITE_OK;
}

/*
** Delete the file identified by argument zPath. If the dirSync parameter
** is non-zero, then ensure the file-system modification to delete the
** file has been synced to disk before returning.
*/
static int demoDelete(sqlite3_vfs*, const char* zPath, int)
{
    DEBUG("delete: %s %d\n", zPath, sync);

    damemory_id id = path_to_id(zPath);
    return damemory_unanchor(id) ? SQLITE_IOERR_DELETE : SQLITE_OK;
}

/*
** Query the file-system to see if the named file exists, is readable or
** is both readable and writable.
*/
static int demoAccess(sqlite3_vfs*, const char* zPath, int flags, int* pResOut)
{
    assert(flags == SQLITE_ACCESS_EXISTS       /* access(zPath, F_OK) */
           || flags == SQLITE_ACCESS_READ      /* access(zPath, R_OK) */
           || flags == SQLITE_ACCESS_READWRITE /* access(zPath, R_OK|W_OK) */
    );

    damemory_id id = path_to_id(zPath);
    *pResOut       = damemory_get_size(id) != 0;

    DEBUG("access: %s, %d\n", zPath, *pResOut);

    return SQLITE_OK;
}

/*
** Argument zPath points to a nul-terminated string containing a file path.
** If zPath is an absolute path, then it is copied as is into the output
** buffer. Otherwise, if it is a relative path, then the equivalent full
** path is written to the output buffer.
**
** This function assumes that paths are UNIX style. Specifically, that:
**
**   1. Path components are separated by a '/'. and
**   2. Full paths begin with a '/' character.
*/
static int demoFullPathname(sqlite3_vfs*,      /* VFS */
                            const char* zPath, /* Input path (possibly a relative path) */
                            int nPathOut,      /* Size of output buffer in bytes */
                            char* zPathOut     /* Pointer to output buffer */
)
{
    DEBUG("path: %s]\n", zPath);

    memset(zPathOut, 0, nPathOut);
    memccpy(zPathOut, zPath, 0, nPathOut);
    zPathOut[nPathOut - 1] = '\0';

    DEBUG("full path: %s]\n", zPathOut);

    return SQLITE_OK;
}

/*
** The following four VFS methods:
**
**   xDlOpen
**   xDlError
**   xDlSym
**   xDlClose
**
** are supposed to implement the functionality needed by SQLite to load
** extensions compiled as shared objects. This simple VFS does not support
** this functionality, so the following functions are no-ops.
*/
static void* demoDlOpen(sqlite3_vfs*, const char*)
{
    return 0;
}
static void demoDlError(sqlite3_vfs*, int nByte, char* zErrMsg)
{
    sqlite3_snprintf(nByte, zErrMsg, "Loadable extensions are not supported");
    zErrMsg[nByte - 1] = '\0';
}
static void (*demoDlSym(sqlite3_vfs*, void*, const char*))(void)
{
    return 0;
}
static void demoDlClose(sqlite3_vfs*, void*)
{
    return;
}

/*
** Parameter zByte points to a buffer nByte bytes in size. Populate this
** buffer with pseudo-random data.
*/
static int demoRandomness(sqlite3_vfs*, int, char*)
{
    return SQLITE_OK;
}

/*
** Sleep for at least nMicro microseconds. Return the (approximate) number
** of microseconds slept for.
*/
static int demoSleep(sqlite3_vfs*, int nMicro)
{
    sleep(nMicro / 1000000);
    usleep(nMicro % 1000000);
    return nMicro;
}

/*
** Set *pTime to the current UTC time expressed as a Julian day. Return
** SQLITE_OK if successful, or an error code otherwise.
**
**   http://en.wikipedia.org/wiki/Julian_day
**
** This implementation is not very good. The current time is rounded to
** an integer number of seconds. Also, assuming time_t is a signed 32-bit
** value, it will stop working some time in the year 2038 AD (the so-called
** "year 2038" problem that afflicts systems that store time this way).
*/
static int demoCurrentTime(sqlite3_vfs*, double* pTime)
{
    time_t t = time(0);
    *pTime   = t / 86400.0 + 2440587.5;
    return SQLITE_OK;
}

/*
** This function returns a pointer to the VFS implemented in this file.
** To make the VFS available to SQLite:
**
**   sqlite3_vfs_register(sqlite3_demovfs(), 0);
*/
sqlite3_vfs* sqlite3_demovfs(void)
{
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    static sqlite3_vfs demovfs = {
        1,                /* iVersion */
        sizeof(DemoFile), /* szOsFile */
        MAXPATHNAME,      /* mxPathname */
        0,                /* pNext */
        "damemorydemo",   /* zName */
        0,                /* pAppData */
        demoOpen,         /* xOpen */
        demoDelete,       /* xDelete */
        demoAccess,       /* xAccess */
        demoFullPathname, /* xFullPathname */
        demoDlOpen,       /* xDlOpen */
        demoDlError,      /* xDlError */
        demoDlSym,        /* xDlSym */
        demoDlClose,      /* xDlClose */
        demoRandomness,   /* xRandomness */
        demoSleep,        /* xSleep */
        demoCurrentTime,  /* xCurrentTime */
    };
#pragma GCC diagnostic pop
    return &demovfs;
}

#endif /* !defined(SQLITE_TEST) || SQLITE_OS_UNIX */

#ifdef SQLITE_TEST

#if defined(INCLUDE_SQLITE_TCL_H)
#include "sqlite_tcl.h"
#else
#include "tcl.h"
#ifndef SQLITE_TCLAPI
#define SQLITE_TCLAPI
#endif
#endif

#if SQLITE_OS_UNIX
static int SQLITE_TCLAPI register_demovfs(
    ClientData clientData, /* Pointer to sqlite3_enable_XXX function */
    Tcl_Interp* interp,    /* The TCL interpreter that invoked this command */
    int objc,              /* Number of arguments */
    Tcl_Obj* CONST objv[]  /* Command arguments */
)
{
    sqlite3_vfs_register(sqlite3_demovfs(), 1);
    return TCL_OK;
}
static int SQLITE_TCLAPI unregister_demovfs(
    ClientData clientData, /* Pointer to sqlite3_enable_XXX function */
    Tcl_Interp* interp,    /* The TCL interpreter that invoked this command */
    int objc,              /* Number of arguments */
    Tcl_Obj* CONST objv[]  /* Command arguments */
)
{
    sqlite3_vfs_unregister(sqlite3_demovfs());
    return TCL_OK;
}

/*
** Register commands with the TCL interpreter.
*/
int Sqlitetest_demovfs_Init(Tcl_Interp* interp)
{
    Tcl_CreateObjCommand(interp, "register_demovfs", register_demovfs, 0, 0);
    Tcl_CreateObjCommand(interp, "unregister_demovfs", unregister_demovfs, 0, 0);
    return TCL_OK;
}

#else
int Sqlitetest_demovfs_Init(Tcl_Interp* interp)
{
    return TCL_OK;
}
#endif

#endif /* SQLITE_TEST */
