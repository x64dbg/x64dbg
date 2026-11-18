#pragma once

#include <Bridge.h>
#include <ElfBug/api/elfbug_api.h>
#include <QHash>
#include <QThread>
#include <QVector>
#include <RegisterContext.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

Q_DECLARE_METATYPE(REGDUMP)

struct DbgThreadInfo
{
    pid_t tid = 0;
    uint32_t number = 0;
    duint rip = 0;
    duint fsBase = 0;
    uint64_t userTimeMs = 0;
    uint64_t kernelTimeMs = 0;
    uint64_t startTimeMs = 0;
    int32_t nice = 0;
    int32_t policy = -1;
    int32_t rtPriority = 0;
    uint32_t suspendCount = 0;
    QString name;
    QString waitReason;
};

Q_DECLARE_METATYPE(QVector<DbgThreadInfo>)

struct DbgMemoryPage
{
    duint base = 0;
    duint size = 0;
    duint moduleBase = 0;
    ElfBugRegionType type = ElfBugRegionType_Private;
    ElfBugParty party = ElfBugParty_User;
    QString perms;
    QString section;
    QString info;
};

Q_DECLARE_METATYPE(QVector<DbgMemoryPage>)

class QTimer;

class DbgAdapter : public QObject, public MemoryProvider
{
    Q_OBJECT

public:
    explicit DbgAdapter(QObject* parent = nullptr);
    ~DbgAdapter() override;

    bool read(duint addr, void* dest, duint size) override;
    bool write(duint addr, const void* src, duint size) override;
    bool getRange(duint addr, duint & base, duint & size) override;
    bool isCodePtr(duint addr) override;
    bool isValidPtr(duint addr) override;
    bool writeRegister(const char* name, duint value) override;
    bool modBaseFromAddr(duint addr, duint & base) override;
    bool modNameFromAddr(duint addr, char* buf, duint bufSize, bool extension) override;

    bool loadEngine();
    bool launch(const char* path);
    bool attach(pid_t pid);
    bool detach();
    [[nodiscard]] static std::vector<ElfBugProcessInfo> enumProcesses();
    void start();
    void run();
    void stepInto();
    void stepOver();
    void pause();
    bool stop();

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isPaused() const;
    [[nodiscard]] bool isEngineLoaded() const { return mDebugger != nullptr; }

    bool toggleBreakpoint(duint addr);
    [[nodiscard]] bool hasBreakpoint(duint addr) const;
    void setThreadListVisible(bool visible);
    bool switchThread(pid_t tid);

    // Empty name removes the label and shows comm again.
    void setThreadName(pid_t tid, const QString & name);
    bool setThreadSuspended(pid_t tid, bool suspended);
    void setAllThreadsSuspended(bool suspended);
    // Call from the GUI thread; while visible, the map also refreshes on a timer when the target runs.
    void setMemoryMapVisible(bool visible);
    void setMemoryMapSectionView(bool sectionView);
    void scheduleMemoryMapRefresh();
    // Writes [start, start + size) to path on the worker, unreadable pages as zeros like Windows savedata; the outcome goes to the log.
    void dumpMemory(duint start, duint size, const QString & path);
    void cancelDump();

signals:
    void processCreated(duint entryPoint);
    void processExited(int exitCode);
    void processDetached();
    // Raised alongside processExited and processDetached; MainWindow resets the debuggee views on it.
    void sessionEnded();
    void registersUpdated(const REGDUMP & regs);
    void logMessage(const QString & msg);
    void errorMessage(const QString & error);
    void stopped(duint rip, const QString & reason);
    void threadsUpdated(const QVector<DbgThreadInfo> & threads, pid_t currentTid);
    void memoryMapUpdated(const QVector<DbgMemoryPage> & pages);
    void dumpProgress(duint done, duint total);
    void dumpFinished();

private:
    static BPXTYPE queryBreakpoint(duint addr);
    static std::atomic<DbgAdapter*> sInstance;

    static void onCreateProcess(pid_t pid, uint64_t entryPoint, void* userdata);
    static void onExitProcess(int exitCode, void* userdata);
    static void onCreateThread(pid_t tid, void* userdata);
    static void onExitThread(pid_t tid, void* userdata);
    static void onSystemBreakpoint(void* userdata);
    static void onAttachBreakpoint(void* userdata);
    static void onDetach(void* userdata);
    static void onExec(void* userdata);
    static void onBreakpoint(uint64_t address, void* userdata);
    static void onStep(void* userdata);
    static void onPaused(void* userdata);
    static void onException(int signal, uint64_t address, void* userdata);
    static void onError(const char* error, void* userdata);
    static void onDebugString(const char* text, void* userdata);

    [[nodiscard]] QString threadSuffix() const;
    [[nodiscard]] REGDUMP readRegisters() const;
    void emitStoppedState(const QString & reason);
    void emitStoppedState(const QString & reason, const REGDUMP & dump);

    // A worker refresh that runs only while its view is visible, and is queued at most once.
    struct RefreshGate
    {
        std::atomic<bool> visible{false};
        std::atomic<bool> queued{false};
    };

    void scheduleRefresh(RefreshGate & gate, void (DbgAdapter::*refresh)());
    void refreshThreads();
    void scheduleThreadRefresh();

    struct DumpJob;

    void refreshMemoryMap();
    void dumpChunk(const std::shared_ptr<DumpJob> & job);

    ElfBugDebugger* mDebugger = nullptr;
    duint mEntryPoint = 0;
    RefreshGate mThreadListGate;
    RefreshGate mMemoryMapGate;
    std::atomic<bool> mMemoryMapSectionView{false};
    QTimer* mRunningMemoryMapTimer = nullptr;
    std::atomic<bool> mDumpCancelled{false};
    QThread mWorkerThread;
    QObject mWorker;

    std::mutex mThreadNameMutex;
    QHash<pid_t, QString> mThreadNames;
};
