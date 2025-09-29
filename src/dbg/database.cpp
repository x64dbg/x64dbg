/**
@file database.cpp

@brief Implements runtime database saving and loading.
*/

#include <thread>
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
        ModCacheSave(root);
        WatchCacheSave(root);

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
        // Some plugins may wish to change this value so that all plugins after their plugin will save data into plugin-supplied storage instead of the system's.
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

        //store the file hash only if other data is saved in the database
        if(dbhash != 0 && json_object_size(root))
        {
            json_object_set_new(root, "hashAlgorithm", json_string("murmurhash"));
            json_object_set_new(root, "hash", json_hex(dbhash));
        }
    }

    auto wdbpath = StringUtils::Utf8ToUtf16(file);
    if(!dbfile)
        MoveFileExW(wdbpath.c_str(), (wdbpath + L".bak").c_str(), MOVEFILE_REPLACE_EXISTING); //move current file to the backup
    if(json_object_size(root))
    {
        auto json_dump_callback_func = [](const char* buffer, size_t size, void* data) -> int
        {
            return ((BufferedWriter*)data)->Write(buffer, size) ? 0 : -1;
        };
        auto dumpSuccess = false;
        HANDLE hFile;
        if(!disablecompression && !settingboolget("Engine", "DisableDatabaseCompression", false))
        {
            LZ4_STATUS status;
            // Create a pipe with random name, 2 max instances, 128KB buffer, 1s wait time
            WString pipeName = StringUtils::sprintf(L"\\\\.\\pipe\\x64dbg-DbSave-%d", rand());
            if((hFile = CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE, 2, 128 * 1024, 128 * 1024, 1000, NULL)) == INVALID_HANDLE_VALUE)
            {
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file !(GetLastError() = %s)\n"), error.c_str());
                return;
            }
            // Start the JSON writer. As JSON data is written, compressor will recieve the data through the pipe and save compressed data.
            std::thread jsonThread([&]()
            {
                BufferedWriter bufWriter(hFile);
                dumpSuccess = !json_dump_callback(root, json_dump_callback_func, &bufWriter, JSON_INDENT(1));
            });
            // Start the compressor to compress and save data
            status = LZ4_compress_fileW(pipeName.c_str(), wdbpath.c_str());
            // Wait for the compressor to finish
            jsonThread.join();
            // Check for error conditions
            if(!dumpSuccess)
            {
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file !(GetLastError() = %s)\n"), error.c_str());
                json_decref(root);
                return;
            }
            const char* errorText;
            switch(status)
            {
            case LZ4_SUCCESS:
                errorText = nullptr;
                break;
            case LZ4_FAILED_OPEN_INPUT:
                errorText = "LZ4_FAILED_OPEN_INPUT";
                break;
            case LZ4_FAILED_OPEN_OUTPUT:
                errorText = "LZ4_FAILED_OPEN_OUTPUT";
                break;
            case LZ4_NOT_ENOUGH_MEMORY:
                errorText = "LZ4_NOT_ENOUGH_MEMORY";
                break;
            case LZ4_INVALID_ARCHIVE:
                errorText = "LZ4_INVALID_ARCHIVE";
                break;
            case LZ4_CORRUPTED_ARCHIVE:
                errorText = "LZ4_CORRUPTED_ARCHIVE";
                break;
            default:
                errorText = "LZ4(UNKNOWN)";
                break;
            }
            if(errorText)
            {
                dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file !(LZ4_compress_fileW() = %s)\n"), errorText);
                return;
            }
        }
        else   // Uncompressed
        {
            hFile = CreateFileW(wdbpath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
            BufferedWriter bufWriter(hFile);
            dumpSuccess = !json_dump_callback(root, json_dump_callback_func, &bufWriter, JSON_INDENT(1));

            if(!dumpSuccess)
            {
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file !(GetLastError() = %s)\n"), error.c_str());
                json_decref(root);
                return;
            }
        }
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
    // If the file is "bak", load from database backup instead
    if(_stricmp(file, "bak") == 0)
    {
        String dbpath_backup(dbpath);
        dbpath_backup.append(".bak");
        DbLoad(loadType, dbpath_backup.c_str());
        return;
    }
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

    // Check if we should migrate the breakpoints
    bool migrateBreakpoints = false;
    {
        HANDLE hFile = CreateFileW(databasePathW.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, 0);
        if(hFile != INVALID_HANDLE_VALUE)
        {
            FILETIME written;
            if(GetFileTime(hFile, nullptr, nullptr, &written))
            {
                // On 2023-06-10 the default of the command condition was changed
                SYSTEMTIME smigration = {};
                smigration.wYear = 2023;
                smigration.wMonth = 6;
                smigration.wDay = 10;
                FILETIME migration = {};
                if(SystemTimeToFileTime(&smigration, &migration))
                {
                    if(CompareFileTime(&written, &migration) < 0)
                    {
                        dprintf(QT_TRANSLATE_NOOP("DBG", "(migrating breakpoints) "));
                        migrateBreakpoints = true;
                    }
                }
            }
            CloseHandle(hFile);
        }
    }

    JSON root;

    // Decompress the file into a pipe if compression was enabled
    bool useCompression = !settingboolget("Engine", "DisableDatabaseCompression", false);
    LZ4_STATUS lzmaStatus = LZ4_INVALID_ARCHIVE;
    if(useCompression)
    {
        HANDLE hFile;
        // Create a pipe with random name, 2 max instances, 128KB buffer, 1s wait time
        WString pipeNameW = StringUtils::sprintf(L"\\\\.\\pipe\\x64dbg-DbLoad-%d", rand());
        if((hFile = CreateNamedPipeW(pipeNameW.c_str(), PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE, 2, 128 * 1024, 128 * 1024, 1000, NULL)) == INVALID_HANDLE_VALUE)
        {
            String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
            dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to write database file !(GetLastError() = %s)\n"), error.c_str());
            return;
        }

        // Deserialize JSON and validate
        std::thread json_loader([&]()
        {
            root = json_load_callback([](void* buffer, size_t buflen, void* data) -> size_t
            {
                HANDLE hFile = (HANDLE)data;
                DWORD n;
                while(true)
                {
                    if(ReadFile(hFile, buffer, buflen, &n, NULL))
                    {
                        if(n > 0)
                            return (size_t)n;
                        else
                            continue; // The other end of the pipe wrote 0 bytes
                    }
                    else if(GetLastError() == ERROR_BROKEN_PIPE)
                    {
                        return 0;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }, hFile, 0, 0);
        });

        lzmaStatus = LZ4_decompress_fileW(databasePathW.c_str(), pipeNameW.c_str());

        // Check return code
        if(lzmaStatus != LZ4_SUCCESS && lzmaStatus != LZ4_INVALID_ARCHIVE)
        {
            dputs(QT_TRANSLATE_NOOP("DBG", "\nInvalid database file!"));
            json_loader.join();
            CloseHandle(hFile);
            return;
        }
        json_loader.join();
        CloseHandle(hFile);
    }
    else   // Uncompressed database can be mapped
    {
        // Map the database file
        FileMap<char> dbMap;
        if(!dbMap.Map(databasePathW.c_str()))
        {
            String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
            dprintf(QT_TRANSLATE_NOOP("DBG", "\nFailed to read database file !(GetLastError() = %s)\n"), error.c_str());
            return;
        }

        // Deserialize JSON and validate
        root = json_loadb(dbMap.Data(), dbMap.Size(), 0, 0);

        // Unmap the database file
        dbMap.Unmap();
    }

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
        BpCacheLoad(root, migrateBreakpoints);
        ModCacheLoad(root);
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
    ModCacheClear();
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
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
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
                String error = stringformatinline(StringUtils::sprintf("{winerror@%x}", GetLastError()));
                dprintf(QT_TRANSLATE_NOOP("DBG", "Cannot write to the program directory (GetLastError() = %s), try running x64dbg as admin...\n"), error.c_str());
                return false;
            }
            CloseHandle(hFile);
            DeleteFileW(testfile.c_str());
            return true;
        };

        if(settingboolget("Engine", "SaveDatabaseInProgramDirectory", false) && checkWritable(fileDir))
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