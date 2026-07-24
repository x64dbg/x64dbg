#include <cstdio>
#include <dlfcn.h>
#include <cstdlib>
#include <string>

#ifndef __clangd__
#include "idasymbols.h"
#endif // __clangd__

std::string idalib_path()
{
    auto fallback = "/Applications/IDA Professional 9.2.app/Contents/MacOS/libidalib.dylib";

    // Get path to config JSON
    std::string configJsonPath;
    auto home = getenv("HOME");
    if(home == nullptr)
    {
        fprintf(stderr, "Failed to get HOME");
        return fallback;
    }
    configJsonPath = home;
    if(configJsonPath.back() == '/')
    {
        configJsonPath.pop_back();
    }
    configJsonPath += "/.idapro/ida-config.json";

    // Read file
    auto fp = fopen(configJsonPath.c_str(), "r");
    if(fp == nullptr)
    {
        fprintf(stderr, "Failed to open ida-config.json\n");
        return fallback;
    }

    fseek(fp, 0, SEEK_END);
    auto len = ftello(fp);
    fseek(fp, 0, SEEK_SET);

    std::string configJson;
    configJson.resize(len);
    if(fread((char*)configJson.data(), 1, configJson.size(), fp) != len)
    {
        fprintf(stderr, "Failed to read ida-config.json\n");
        return fallback;
    }
    fclose(fp);

    // Extract IDA install path
    auto keyIdx = configJson.find(R"("ida-install-dir": ")");
    if(keyIdx == std::string::npos)
    {
        fprintf(stderr, "Could not find ida-install-dir key in ida-config.json\n");
        return fallback;
    }
    auto quoteIdx = configJson.find('\"', keyIdx + 20);
    if(quoteIdx == std::string::npos)
    {
        fprintf(stderr, "Could not find closing quote for ida-install-dir\n");
        return fallback;
    }
    auto idaInstallDir = configJson.substr(keyIdx + 20, quoteIdx - keyIdx - 20);
    if(idaInstallDir.back() == '/')
    {
        idaInstallDir.pop_back();
    }
    fprintf(stderr, "ida-install-dir: '%s'\n", idaInstallDir.c_str());

    return idaInstallDir + "/libidalib.dylib";
}

#ifndef __clangd__

#define DECLARE_STUB(name) \
void* fp_##name = nullptr;

#define DEFINE_STUB(name) \
__attribute__((naked)) void name() { \
        __asm__("adrp x16, _fp_" #name "@PAGE\n" \
                "ldr x16, [x16, _fp_" #name "@PAGEOFF]\n" \
                "br x16"); \
}

#define SYMBOL_ENTRY(name) {#name, &fp_##name}

extern "C"
{
struct SymbolEntry {
    const char* name;
    void** ptr;
};


// Declare all function pointers
#define X(name) DECLARE_STUB(name)
IDA_SYMBOLS
#undef X

static SymbolEntry g_symbols[] = {
#define X(name) SYMBOL_ENTRY(name),
    IDA_SYMBOLS
#undef X
    };

bool idalib_resolve()
{
    auto idalibPath = idalib_path();

    auto g_libidalib_handle = dlopen(idalibPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    fprintf(stderr, "ida_resolve(), libidalib.dylib: %p\n", g_libidalib_handle);

    if (!g_libidalib_handle)
    {
        fprintf(stderr, "Failed to load: %s\n", dlerror());
        return false;
    }

    // Resolve all symbols
    bool resolved = true;
    for (auto& entry : g_symbols) {
        *entry.ptr = dlsym(g_libidalib_handle, entry.name);
        if (!*entry.ptr) {
            fprintf(stderr, "Failed to resolve %s\n", entry.name);
            resolved = false;
        }
    }
    return resolved;
}

// Trampoline functions
// Define all trampolines
#define X(name) DEFINE_STUB(name)
IDA_SYMBOLS
#undef X

}

#endif // __clangd__
