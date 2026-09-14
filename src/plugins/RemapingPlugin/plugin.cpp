// 让软件断点能下到 SEC_NO_CHANGE 保护的节里。
//
// SEC_NO_CHANGE 的节页保护锁死，VirtualProtectEx 一律失败。而 x64dbg 下软件断点
// 第一步就是改页保护（TitanEngine.Breakpoints.cpp:249），失败就直接 return false，
// INT3 写不进去。本插件在 x64dbg 动手之前先把这块内存变成可写，再交还控制权。

#include <Windows.h>

#include <atomic>
#include <mutex>
#include <unordered_set>

// 插件 SDK：CBTYPE、PLUG_CB_* 结构体、_plugin_* 函数都在这里
#include "_plugins.h"
// 桥接层：所有 Dbg* 开头的函数，插件靠它访问调试器
#include "bridgemain.h"

// 匿名 namespace：里面的符号只在本文件可见，不会混进 DLL 导出表
namespace
{
    // 日志前缀，也是插件列表里显示的名字
    const char* PluginName = "RemapingPlugin";

    // x64dbg 在 pluginit 时分配给本插件的编号，注册回调和操作菜单时必须带上
    int gPluginHandle = 0;

    // 菜单项编号，由插件自己定，同一插件内不重复即可（-1 是保留值）
    enum MenuEntry
    {
        MenuToggle,   // 启用解锁（可勾选）
    };

    // 插件总开关，和菜单上那个勾一一对应。
    // 勾上 = 拦截下断点并解锁；不勾 = 什么都不做，断点走 x64dbg 原本的流程。
    std::atomic<bool> gEnabled{ true };

    // 本次会话已解锁过的区域基址。
    // 必须去重：CB_BEFORE_SETBPX 每下一个断点触发一次，同一个节里下十个断点就会
    // 触发十次，没有缓存就会重复解锁十遍。
    std::mutex gUnlockedMutex;
    std::unordered_set<duint> gUnlocked;

    // 这个区域之前解锁过了吗
    bool alreadyUnlocked(duint base)
    {
        std::lock_guard<std::mutex> lock(gUnlockedMutex);
        return gUnlocked.find(base) != gUnlocked.end();
    }

    // 记下这个区域已经解锁
    void markUnlocked(duint base)
    {
        std::lock_guard<std::mutex> lock(gUnlockedMutex);
        gUnlocked.insert(base);
    }

    // 清空全部记录，换调试会话时用
    void forgetUnlocked()
    {
        std::lock_guard<std::mutex> lock(gUnlockedMutex);
        gUnlocked.clear();
    }

    // 这块内存的页保护改不改得动？
    // 用和 SetBPX 完全相同的调用来试探，所以结论等价于"这个断点会不会失败"。
    // 普通区域会成功，随即还原，插件对它没有任何副作用。
    bool needsUnlock(HANDLE hProcess, duint addr)
    {
        DWORD oldProtect = 0;
        if(VirtualProtectEx(hProcess, (LPVOID)addr, 1, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            VirtualProtectEx(hProcess, (LPVOID)addr, 1, oldProtect, &oldProtect);
            return false;
        }
        return true;
    }

    // TODO: 在这里实现你的解锁。
    //   hProcess  靶机进程句柄
    //   base/size 被锁住的整个内存区域
    //   返回 false 会让 x64dbg 放弃这个断点
    // 调用时靶机所有线程已挂起。
    //
    // 两个坑：
    // 1. SuspendThread 是异步的，返回时线程可能还在跑（thread.cpp:317 没做同步
    //    等待）。对此敏感的话自己补一次 dummy GetThreadContext。
    // 2. 不要事后还原成只读。断点每命中一次还要再写两次这块内存
    //    （DebugLoop.cpp:633 写回原字节、:784 贴回 INT3），不可写就会跑飞。
    bool unlockSection(HANDLE hProcess, duint base, duint size)
    {
        (void)hProcess;
        (void)base;
        (void)size;
        return false;
    }

    // 【回调】x64dbg 让调试引擎下软件断点之前触发。
    // 此刻地址处还是原始字节，INT3 未写入，且没有持有任何断点锁。
    //
    // 所有 x64dbg 回调都是这个签名：第一个参数是回调类型（一个函数只处理一种回调
    // 时用不上），第二个参数要按回调类型强转成对应的 PLUG_CB_* 结构体。
    void cbBeforeSetBPX(CBTYPE, void* callbackInfo)
    {
        auto info = (PLUG_CB_BEFORE_SETBPX*)callbackInfo;
        if(info == nullptr)
            return;

        // 菜单里关掉了就什么都不做，断点走 x64dbg 原本的流程。
        // 这个检查必须在最前面：没勾选时后面一律不执行。
        if(!gEnabled.load())
            return;

        // 靶机的进程句柄，没有调试会话时为空
        auto hProcess = DbgGetProcessHandle();
        if(hProcess == nullptr)
            return;

        // 先做便宜的探测：普通区域到这里就返回了，连线程都不会挂起
        if(!needsUnlock(hProcess, info->addr))
            return;

        // 取这个地址所在内存区域的基址和大小
        duint size = 0;
        auto base = DbgMemFindBaseAddr(info->addr, &size);
        if(base == 0)
        {
            // _plugin_logprintf 打到 x64dbg 的日志窗口
            _plugin_logprintf("[%s] 找不到 %llX 所在的内存区域\n",
                              PluginName, (unsigned long long)info->addr);
            info->cancel = true;
            return;
        }

        if(alreadyUnlocked(base))
            return;

        // bp 命令不要求靶机已暂停（command.cpp:307 只查了有没有调试会话），
        // 所以此刻它可能正在全速运行，改映射前先冻住所有线程。
        //
        // DbgCmdExecDirect 同步执行一条 x64dbg 命令。这两条等价于 GUI 线程列表里
        // 右键的 Suspend / Resume All Threads（ThreadView.cpp:31）。
        // 必须严格配对，漏掉 resume 靶机就再也起不来。
        auto suspended = DbgCmdExecDirect("suspendallthreads");
        auto unlocked = unlockSection(hProcess, base, size);
        if(suspended)
            DbgCmdExecDirect("resumeallthreads");

        if(!unlocked)
        {
            _plugin_logprintf("[%s] 解锁 %llX[%llX] 失败，放弃 %llX 处的断点\n",
                              PluginName,
                              (unsigned long long)base,
                              (unsigned long long)size,
                              (unsigned long long)info->addr);
            // cancel 置 true，x64dbg 当作 SetBPX 失败处理：
            // cmd-breakpoint-control.cpp:135 会 BpDelete 撤销刚入库的记录。
            info->cancel = true;
            return;
        }

        markUnlocked(base);
        _plugin_logprintf("[%s] 已解锁 %llX[%llX]\n",
                          PluginName,
                          (unsigned long long)base,
                          (unsigned long long)size);
    }

    // 【回调】调试引擎处理完之后触发，成功失败都触发。
    // 前面置了 cancel 的话引擎压根没被调用，这里也不会触发。
    // 走到这里还 success == false，说明失败另有原因，记一笔方便排查。
    void cbAfterSetBPX(CBTYPE, void* callbackInfo)
    {
        auto info = (PLUG_CB_AFTER_SETBPX*)callbackInfo;
        if(info == nullptr || info->success)
            return;

        _plugin_logprintf("[%s] 引擎仍然拒绝了 %llX 处的断点\n",
                          PluginName, (unsigned long long)info->addr);
    }

    // 【回调】新的调试会话开始，地址空间换了，解锁记录全部作废
    void cbInitDebug(CBTYPE, void*)
    {
        forgetUnlocked();
    }

    // 【回调】菜单项被点击。info->hEntry 就是 plugsetup 里登记的那个编号。
    void cbMenuEntry(CBTYPE, void* callbackInfo)
    {
        auto info = (PLUG_CB_MENUENTRY*)callbackInfo;
        if(info == nullptr || info->hEntry != MenuToggle)
            return;

        // 翻转开关，再把菜单上的勾同步成新状态
        auto enabled = !gEnabled.load();
        gEnabled.store(enabled);
        _plugin_menuentrysetchecked(gPluginHandle, MenuToggle, enabled);
        _plugin_logprintf("[%s] %s\n", PluginName, enabled ? "已启用" : "已禁用");
    }
}

// 下面三个是 x64dbg 插件必须导出的函数，少一个就加载失败。

// 插件初始化，加载时第一个被调用。返回 false 则放弃加载。
extern "C" __declspec(dllexport) bool pluginit(PLUG_INITSTRUCT* initStruct)
{
    initStruct->pluginVersion = 1;
    // 必须和 x64dbg 的 PLUG_SDKVERSION 一致，不然 plugin_loader.cpp:380 直接拒绝
    initStruct->sdkVersion = PLUG_SDKVERSION;
    strncpy_s(initStruct->pluginName, sizeof(initStruct->pluginName), PluginName, _TRUNCATE);
    gPluginHandle = initStruct->pluginHandle;

    // 把回调挂上去，三个参数分别是：插件编号、回调类型、处理函数
    _plugin_registercallback(gPluginHandle, CB_INITDEBUG, cbInitDebug);
    _plugin_registercallback(gPluginHandle, CB_BEFORE_SETBPX, cbBeforeSetBPX);
    _plugin_registercallback(gPluginHandle, CB_AFTER_SETBPX, cbAfterSetBPX);
    _plugin_registercallback(gPluginHandle, CB_MENUENTRY, cbMenuEntry);
    return true;
}

// 卸载时调用（plugunload 命令或退出 x64dbg）。
// 回调和菜单由 x64dbg 自动注销，这里只清理自己申请的资源。
extern "C" __declspec(dllexport) void plugstop()
{
}

// GUI 就绪后调用，在这里往菜单里加东西。
// setupStruct->hMenu 是 x64dbg 分配给本插件的菜单句柄，加进去的条目会出现在
// 主菜单 Plugins -> RemapingPlugin 下面。
extern "C" __declspec(dllexport) void plugsetup(PLUG_SETUPSTRUCT* setupStruct)
{
    _plugin_menuaddentry(setupStruct->hMenu, MenuToggle, "启用解锁");
    // setchecked 会顺带把菜单项变成可勾选的（MainWindow.cpp:1960），
    // 所以这一行既是设初值，也是让勾能显示出来的前提。
    _plugin_menuentrysetchecked(gPluginHandle, MenuToggle, gEnabled.load());
}
