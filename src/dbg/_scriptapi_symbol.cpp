#include "_scriptapi_symbol.h"
#include "_scriptapi_label.h"

bool Script::Symbol::GetList(ListOf(SymbolInfo) list)
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
    //TODO: enumerate actual symbols + virtual symbols (sub_XXXXXX) + imports + exports in addition to user-defined labels.
    return BridgeList<SymbolInfo>::CopyData(list, symbols);
}
