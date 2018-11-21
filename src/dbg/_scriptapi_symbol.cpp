#include "_scriptapi_symbol.h"
#include "_scriptapi_label.h"
#include "symbolinfo.h"

using namespace Script::Symbol;

struct cbSymbolEnumCtx
{
    const SYMBOLMODULEINFO* module;
    std::vector<SymbolInfo>* symbols;
};

static bool cbSymbolEnum(const SYMBOLPTR* ptr, void* user)
{
    auto ctx = (cbSymbolEnumCtx*)user;
    SYMBOLINFO info;
    DbgGetSymbolInfo(ptr, &info);

    const bool hasUndecoratedName = (0 != info.undecoratedSymbol) && (0 != info.undecoratedSymbol[0]);
    const char* symbolName = hasUndecoratedName ? info.undecoratedSymbol : info.decoratedSymbol;

    SymbolInfo symbol = {};
    strncpy_s(symbol.mod, sizeof(symbol.mod), ctx->module->name, sizeof(symbol.mod) - 1);
    symbol.rva = info.addr - ctx->module->base;
    strncpy_s(symbol.name, sizeof(symbol.name), symbolName, sizeof(symbol.name) - 1);
    symbol.manual = false;
    switch(info.type)
    {
    case sym_import:
        symbol.type = Import;
        break;
    case sym_export:
        symbol.type = Export;
        break;
    case sym_symbol:
        symbol.type = Function;
        break;
    default:
        __debugbreak();
    }

    ctx->symbols->push_back(symbol);
    return true;
}

SCRIPT_EXPORT bool Script::Symbol::GetList(ListOf(SymbolInfo) list)
{
    BridgeList<Label::LabelInfo> labels;
    if(!Label::GetList(&labels))
        return false;
    std::vector<SymbolInfo> symbols;
    symbols.reserve(labels.Count());
    for(auto i = 0; i < labels.Count(); i++)
    {
        const auto & label = labels[i];
        SymbolInfo symbol;
        strcpy_s(symbol.mod, label.mod);
        symbol.rva = label.rva;
        strcpy_s(symbol.name, label.text);
        symbol.manual = label.manual;
        symbol.type = Function;
        symbols.push_back(symbol);
    }

    std::vector<SYMBOLMODULEINFO> modules;
    SymGetModuleList(&modules);
    cbSymbolEnumCtx ctx;
    ctx.symbols = &symbols;

    for(const auto & mod : modules)
    {
        ctx.module = &mod;
        DbgSymbolEnumFromCache(mod.base, cbSymbolEnum, &ctx);
    }

    //TODO: enumerate actual symbols + virtual symbols (sub_XXXXXX) + imports + exports in addition to user-defined labels.
    return BridgeList<SymbolInfo>::CopyData(list, symbols);
}
