#include "_scriptapi_symbol.h"
#include "_scriptapi_label.h"
#include "symbolinfo.h"

using namespace Script::Symbol;

struct cbSymbolEnumCtx
{
    const SYMBOLMODULEINFO* module;
    std::vector<SymbolInfo>* symbols;
};

static void cbSymbolEnum(SYMBOLINFO* info, void* user)
{
    auto ctx = (cbSymbolEnumCtx*)user;

    SymbolInfo symbol = {};
    strncpy_s(symbol.mod, sizeof(symbol.mod), ctx->module->name, sizeof(symbol.mod) - 1);
    symbol.rva = info->addr - ctx->module->base;
    strncpy_s(symbol.name, sizeof(symbol.name), info->undecoratedSymbol ? info->undecoratedSymbol : info->decoratedSymbol, sizeof(symbol.name) - 1);
    symbol.manual = false;
    symbol.type = info->isImported ? Import : Export;
    ctx->symbols->push_back(symbol);
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
