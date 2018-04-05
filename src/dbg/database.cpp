/**
@file database.cpp

@brief Implements runtime database saving and loading.
*/

#include "lz4/lz4file.h"
#include "console.h"
#include "breakpoint.h"
#include "patches.h"
#include "comment.h"
#include "label.h"
#include "bookmark.h"
#include "function.h"
#include "loop.h"
#include "watch.h"
#include "commandline.h"
#include "database.h"
#include "threading.h"
#include "filehelper.h"
#include "xrefs.h"
#include "TraceRecord.h"
#include "encodemap.h"
#include "plugin_loader.h"
#include "argument.h"
#include "filemap.h"
#include "debugger.h"
#include "stringformat.h"

/**
\brief Directory where program databases are stored (usually in \db). UTF-8 encoding.
*/
char dbbasepath[deflen];

/**
\brief The hash of the debuggee stored in the database
*/
duint dbhash = 0;

/**
\brief Path of the current program database. UTF-8 encoding.
*/
char dbpath[deflen];

void DbSave(DbLoadSaveType saveType, const char* dbfile, bool disablecompression)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    auto file = dbfile ? dbfile : dbpath;
    auto filename = strrchr(file, '\\');
    auto cmdlinepath = filename ? StringUtils::sprintf("%s%s.cmdline", dbbasepath, filename) : file + String(".cmdline");
    dprintf(QT_TRANSLATE_NOOP("DBG", "Saving database to %s "), file);
    DWORD ticks = GetTickCount();
    JSON root = json_object();

    // Save only command line
    if(saveType == DbLoadSaveType::CommandLine || saveType == DbLoadSaveType::All)
    {
        CmdLineCacheSave(root, cmdlinepath);
    }

    if(saveType == DbLoadSaveType::DebugData || saveType == DbLoadSaveType::All)
    {
        CommentCacheSave(root);
        LabelCacheSave(root);
        BookmarkCacheSave(root);
        FunctionCacheSave(root);
        ArgumentCacheSave(root);
        LoopCacheSave(root);
        XrefCacheSave(root);
        EncodeMapCacheSave(root);
        TraceRecord.saveToDb(root);
        BpCacheSave(root);
        WatchCacheSave(root);
        if(dbhash != 0)
        {
            json_object_set_new(root, "hashAlgorithm", json_string("murmurhash"));
            json_object_set_new(root, "hash", json_hex(dbhash));
        }

        //save notes
        char* text = nullptr;
        GuiGetDebuggeeNotes(&text);
        if(text)
        {
            json_object_set_new(root, "notes", json_string(text));
            BridgeFree(text);
        }

        //save initialization script
        const char* initscript = dbggetdebuggeeinitscript();
        if(initscript[0] != 0)
        {
            json_object_set_new(root, "initscript", json_string(initscript));
        }

        //plugin data
        PLUG_CB_LOADSAVEDB pluginSaveDb;
        // Some plugins may wish to change this value so that all plugins after his or her plugin will save data into plugin-supplied storage instead of the system's.
        // We back up this value so that the debugger is not fooled by such plugins.
        JSON pluginRoot = json_object();
        pluginSaveDb.root = pluginRoot;
        switch(saveType)
        {
        case DbLoadSaveType::DebugData:
            pluginSaveDb.loadSaveType = PLUG_DB_LOADSAVE_DATA;
            break;
        case DbLoadSaveType::All:
            pluginSaveDb.loadSaveType = PLUG_DB_LOADSAVE_ALL;
            break;
        default:
            pluginSaveDb.loadSaveType = 0;
            break;
        }
        plugincbcall(CBTYPE::CB_SAVEDB, &pluginSaveDb);
        if(json_object_size(pluginRoot))
            json_object_set(root, "plugins", pluginRoot);
        json_decref(pluginRoot);
    }

    auto wdbpath = StringUtils::Utf8ToUtf16(file);
    if(!dbfile)
        CopyFileW(wdbpath.c_str(), (wdbpath + L".bak").c_str(), FALSE); //make a backup
    if(json_object_size(root))
    {
        auto dumpSuccess = false;
        auto hFile = CreateFileW(wdbpath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            BufferedWriter bufWriter(hFile);
            dumpSuccess = !json_dump_callback(root, [](const char* buffer, size_t size, void* data) -> int
            {
                return ((BufferedWriter*)data)->Write(buffer, size) ? 0 : -1;
            }, &bufWriter, JSON_INDENT(1));
        }

        if(!dumpSuccess)
        {
            String error = stringformatinline(StringUtils::sprintf("{winerror@%d}", GetLastError()));
            dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file!(GetLastError() = %s)\n"), error.c_str());
            json_decref(root);
            return;
        }

        if(!disablecompression && !settingboolget("Engine", "DisableDatabaseCompression"))
            LZ4_compress_fileW(wdbpath.c_str(), wdbpath.c_str());
    }
    else //remove database when nothing is in there
    {
        DeleteFileW(wdbpath.c_str());
        DeleteFileW(StringUtils::Utf8ToUtf16(cmdlinepath).c_str());
    }

    dprintf(QT_TRANSLATE_NOOP("DBG", "%ums\n"), GetTickCount() - ticks);
    json_decref(root); //free root
}

void DbLoad(DbLoadSaveType loadType, const char* dbfile)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    auto file = dbfile ? dbfile : dbpath;
    // If the file doesn't exist, there is no DB to load
    if(!FileExists(file))
        return;

    if(loadType == DbLoadSaveType::CommandLine)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "Loading commandline..."));
        String content;
        if(FileHelper::ReadAllText(file + String(".cmdline"), content))
        {
            copyCommandLine(content.c_str());
            return;
        }
    }
    else
        dprintf(QT_TRANSLATE_NOOP("DBG", "Loading database from %s "), file);
    DWORD ticks = GetTickCount();

    // Multi-byte (UTF8) file path converted to UTF16
    WString databasePathW = StringUtils::Utf8ToUtf16(file);

    // Decompress the file if compression was enabled
    bool useCompression = !settingboolget("Engine", "DisableDatabaseCompression");
    LZ4_STATUS lzmaStatus = LZ4_INVALID_ARCHIVE;
    {
        lzmaStatus = LZ4_decompress_fileW(databasePathW.c_str(), databasePathW.c_str());

        // Check return code
        if(useCompression && lzmaStatus != LZ4_SUCCESS && lzmaStatus != LZ4_INVALID_ARCHIVE)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "\nInvalid database file!"));
            return;
        }
    }

    // Map the database file
    FileMap<char> dbMap;
    if(!dbMap.Map(databasePathW.c_str()))
    {
        String error = stringformatinline(StringUtils::sprintf("{winerror@%d}", GetLastError()));
        dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to read database file!(GetLastError() = %s)\n"), error.c_str());
        return;
    }

    // Deserialize JSON and validate
    JSON root = json_loadb(dbMap.Data(), dbMap.Size(), 0, 0);

    // Unmap the database file
    dbMap.Unmap();

    // Restore the old, compressed file
    if(lzmaStatus != LZ4_INVALID_ARCHIVE && useCompression)
        LZ4_compress_fileW(databasePathW.c_str(), databasePathW.c_str());

    if(!root)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "\nInvalid database file (JSON)!"));
        return;
    }

    // Load only command line
    if(loadType == DbLoadSaveType::CommandLine || loadType == DbLoadSaveType::All)
    {
        CmdLineCacheLoad(root);
    }

    if(loadType == DbLoadSaveType::DebugData || loadType == DbLoadSaveType::All)
    {
        auto hashalgo = json_string_value(json_object_get(root, "hashAlgorithm"));
        if(hashalgo && strcmp(hashalgo, "murmurhash") == 0) //Checking checksum of the debuggee.
            dbhash = duint(json_hex_value(json_object_get(root, "hash")));
        else
            dbhash = 0;

        // Finally load all structures
        CommentCacheLoad(root);
        LabelCacheLoad(root);
        BookmarkCacheLoad(root);
        FunctionCacheLoad(root);
        ArgumentCacheLoad(root);
        LoopCacheLoad(root);
        XrefCacheLoad(root);
        EncodeMapCacheLoad(root);
        TraceRecord.loadFromDb(root);
        BpCacheLoad(root);
        WatchCacheLoad(root);

        // Load notes
        const char* text = json_string_value(json_object_get(root, "notes"));
        GuiSetDebuggeeNotes(text);

        // Initialization script
        text = json_string_value(json_object_get(root, "initscript"));
        dbgsetdebuggeeinitscript(text);

        // Plugins
        JSON pluginRoot = json_object_get(root, "plugins");
        if(pluginRoot)
        {
            PLUG_CB_LOADSAVEDB pluginLoadDb;
            pluginLoadDb.root = pluginRoot;
            switch(loadType)
            {
            case DbLoadSaveType::DebugData:
                pluginLoadDb.loadSaveType = PLUG_DB_LOADSAVE_DATA;
                break;
            case DbLoadSaveType::All:
                pluginLoadDb.loadSaveType = PLUG_DB_LOADSAVE_ALL;
                break;
            default:
                pluginLoadDb.loadSaveType = 0;
                break;
            }
            plugincbcall(CB_LOADDB, &pluginLoadDb);
        }
    }

    // Free root
    json_decref(root);

    if(loadType != DbLoadSaveType::CommandLine)
        dprintf(QT_TRANSLATE_NOOP("DBG", "%ums\n"), GetTickCount() - ticks);
}

void DbClose()
{
    DbSave(DbLoadSaveType::All);
    DbClear(true);
}

void DbClear(bool terminating)
{
    CommentClear();
    LabelClear();
    BookmarkClear();
    FunctionClear();
    ArgumentClear();
    LoopClear();
    XrefClear();
    EncodeMapClear();
    TraceRecord.clear();
    BpClear();
    WatchClear();
    GuiSetDebuggeeNotes("");

    if(terminating)
    {
        PatchClear();
        dbhash = 0;
    }
}

void DbSetPath(const char* Directory, const char* ModulePath)
{
    EXCLUSIVE_ACQUIRE(LockDatabase);

    // Initialize directory only if it was supplied
    if(Directory)
    {
        ASSERT_TRUE(strlen(Directory) > 0);

        // Copy to global
        strcpy_s(dbbasepath, Directory);

        // Create directory
        if(!CreateDirectoryW(StringUtils::Utf8ToUtf16(Directory).c_str(), nullptr))
        {
            if(GetLastError() != ERROR_ALREADY_EXISTS)
            {
                String error = stringformatinline(StringUtils::sprintf("{winerror@%d}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "Warning: Failed to create database folder '%s'. GetLastError() = %s\n"), Directory, error.c_str());
            }
        }
    }

    // The database file path may be relative (dbbasepath) or a full path
    if(ModulePath)
    {
        ASSERT_TRUE(strlen(ModulePath) > 0);

#ifdef _WIN64
        const char* dbType = "dd64";
#else
        const char* dbType = "dd32";
#endif // _WIN64

        // Get the module name and directory
        char dbName[deflen];
        char fileDir[deflen];
        {
            // Dir <- file path
            strcpy_s(fileDir, ModulePath);

            // Find the last instance of a path delimiter (slash)
            char* fileStart = strrchr(fileDir, '\\');

            if(fileStart)
            {
                strcpy_s(dbName, fileStart + 1);
                fileStart[0] = '\0';
            }
            else
            {
                // Directory or file with no extension
                strcpy_s(dbName, fileDir);
            }
        }

        auto checkWritable = [](const char* fileDir)
        {
            auto testfile = StringUtils::Utf8ToUtf16(StringUtils::sprintf("%s\\%X.x64dbg", fileDir, GetTickCount()));
            auto hFile = CreateFileW(testfile.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
            if(hFile == INVALID_HANDLE_VALUE)
            {
                String error = stringformatinline(StringUtils::sprintf("{winerror@%d}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "Cannot write to the program directory(GetLastError() = %s), try running x64dbg as admin...\n"), error.c_str());
                return false;
            }
            CloseHandle(hFile);
            DeleteFileW(testfile.c_str());
            return true;
        };

        if(settingboolget("Engine", "SaveDatabaseInProgramDirectory") && checkWritable(fileDir))
        {
            // Absolute path in the program directory
            sprintf_s(dbpath, "%s\\%s.%s", fileDir, dbName, dbType);
        }
        else
        {
            // Relative path in debugger directory
            sprintf_s(dbpath, "%s\\%s.%s", dbbasepath, dbName, dbType);
        }

        dprintf(QT_TRANSLATE_NOOP("DBG", "Database file: %s\n"), dbpath);
    }
}

/**
\brief Warn the user if the hash in the database and the executable mismatch.
*/
bool DbCheckHash(duint currentHash)
{
    if(dbhash != 0 && currentHash != 0 && dbhash != currentHash)
    {
        dputs(QT_TRANSLATE_NOOP("DBG", "WARNING: The database has a checksum that is different from the module you are debugging. It is possible that your debuggee has been modified since last session. The content of this database may be incorrect."));
        dbhash = currentHash;
        return false;
    }
    else
    {
        dbhash = currentHash;
        return true;
    }
}

duint DbGetHash()
{
    return dbhash;
}