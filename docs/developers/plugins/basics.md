# The basics

This page covers the basic principles of plugin development for x64dbg. See the [plugin page](http://plugins.x64dbg.com) for example plugins and templates.

## Exports

A plugin has at least one export. This export must be called `pluginit`. See the `PLUG_INITSTRUCT` and the plugin headers for more information. The other valid exports are:

`plugstop` called when the plugin is about to be unloaded. Remove all registered commands and callbacks here. Also clean up plugin data.

`plugsetup` Called when the plugin initialization was successful, here you can register menus and other GUI-related things.

`CB*` Instead of calling `_plugin_registercallback`, you can create a `CDECL` export which has the name of the callback. For example when you create an export called `CBMENUENTRY`, this will be registered as your callback for the event `CB_MENUENTRY`. Notice that you should not use an underscore in the export name.

`CBALLEVENTS` An export with the name `CBALLEVENTS` will get every event registered to it. This is done prior to registering optional other export names.

## Definitions

Initialization exports.

```c++
extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct);
extern "C" __declspec(dllexport) bool plugstop();
extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT* setupStruct);
```

Callback exports. **Make sure to only export callbacks that you actually use!**

```c++
extern "C" __declspec(dllexport) void CBINITDEBUG(CBTYPE cbType, PLUG_CB_INITDEBUG* info);
extern "C" __declspec(dllexport) void CBSTOPDEBUG(CBTYPE cbType, PLUG_CB_STOPDEBUG* info);
extern "C" __declspec(dllexport) void CBEXCEPTION(CBTYPE cbType, PLUG_CB_EXCEPTION* info);
extern "C" __declspec(dllexport) void CBDEBUGEVENT(CBTYPE cbType, PLUG_CB_DEBUGEVENT* info);
extern "C" __declspec(dllexport) void CBMENUENTRY(CBTYPE cbType, PLUG_CB_MENUENTRY* info);
```
