#pragma once

bool KdOpEmuLoad();

bool OpCheckForEmulation(KD_CONTEXT *Context);
bool OpEmuSwapgs(KD_CONTEXT *Context);
bool OpEmuSyscall(KD_CONTEXT *Context);
bool OpEmuSysret(KD_CONTEXT *Context);
bool OpEmuSysenter(KD_CONTEXT *Context);
bool OpEmuSysexit(KD_CONTEXT *Context);