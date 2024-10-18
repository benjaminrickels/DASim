#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sqlite3.h>

#include <damemory/damemory.h>

#if 0
#define SQLITE_VFS "demo"
#include <demovfs.h>
#else
#define SQLITE_VFS "damemorydemo"
#include <damemorydemovfs.h>
#endif

int main()
{
    static char query[512];
    memset(query, 0, sizeof(query));

    damemory_init();

    bool vfsFound = false;
    if (sqlite3_vfs_find(SQLITE_VFS)) {
        printf("VFS found\n");
        vfsFound = true;
    }
    else {
        printf("VFS not found\n");
    }

    if (!vfsFound && sqlite3_vfs_register(sqlite3_demovfs(), 0) != SQLITE_OK) {
        printf("Could not register VFS\n");
        return EXIT_FAILURE;
    }
    else {
        printf("VFS registered\n");
    }

    if (!sqlite3_vfs_find(SQLITE_VFS)) {
        printf("VFS not found\n");
        return EXIT_FAILURE;
    }

    sqlite3* db;

    if (sqlite3_open_v2("tdb", &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, SQLITE_VFS)
        != SQLITE_OK) {
        printf("Could not open DB\n");
        sqlite3_close_v2(db);
        return EXIT_FAILURE;
    }

    char* err;

    snprintf(query, sizeof(query), "CREATE TABLE testtable (col1);");
    if (sqlite3_exec(db, query, NULL, NULL, &err) != SQLITE_OK) {
        printf("\"%s\" failed: %s (%d)\n", query, sqlite3_errmsg(db), sqlite3_errcode(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    snprintf(query, sizeof(query), "BEGIN TRANSACTION;");
    if (sqlite3_exec(db, query, NULL, NULL, &err) != SQLITE_OK) {
        printf("\"%s\" failed: %s (%d)\n", query, sqlite3_errmsg(db), sqlite3_errcode(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    snprintf(query, sizeof(query), "INSERT INTO testtable VALUES (?);");
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, NULL) != SQLITE_OK) {
        printf("Preparing \"%s\" failed: %s (%d)\n", query, sqlite3_errmsg(db),
               sqlite3_errcode(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < 10000; i++) {
        int val = rand() % 256;
        printf("Inserting %d\n", val);
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, val);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            printf("ERROR inserting %d: %s (%d)\n", i, sqlite3_errmsg(db), sqlite3_errcode(db));
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return EXIT_FAILURE;
        }
    }

    sqlite3_finalize(stmt);

    snprintf(query, sizeof(query), "COMMIT;");
    if (sqlite3_exec(db, query, NULL, NULL, &err) != SQLITE_OK) {
        printf("\"%s\" failed: %s (%d)\n", query, sqlite3_errmsg(db), sqlite3_errcode(db));
        sqlite3_close(db);
        return EXIT_FAILURE;
    }

    sqlite3_free(err);
    sqlite3_close_v2(db);
    return 0;
}
