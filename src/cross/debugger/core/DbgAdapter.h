#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <QHash>
#include <QVector>
#include <ElfBug/api/elfbug_api.h>
#include "RegisterContext.h"
#include "Bridge.h"

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
    bool launch(const char* path) const;
    void Start() const;
    void Continue() const;
    void StepInto() const;
    void StepOver() const;
    void Pause() const;
    bool Stop() const; //discardable

    [[nodiscard]] bool isActive() const;
    [[nodiscard]] bool isEngineLoaded() const { return mDebugger != nullptr; }
    [[nodiscard]] duint entryPoint() const { return mEntryPoint; }

    [[nodiscard]] bool toggleBreakpoint(duint addr) const;
    [[nodiscard]] bool hasBreakpoint(duint addr) const;
    void refreshThreads();
    bool switchThread(pid_t tid);

    // Empty name removes the label and shows comm again.
    void setThreadName(pid_t tid, const QString & name);
    bool setThreadSuspended(pid_t tid, bool suspended);
    void setAllThreadsSuspended(bool suspended);

signals:
    void processCreated(duint entryPoint);
    void processExited(int exitCode);
    void registersUpdated(const REGDUMP & regs);
    void logMessage(const QString & msg);
    void stopped(duint rip, const QString & reason);
    void threadsUpdated(const QVector<DbgThreadInfo> & threads, pid_t currentTid);

private:
    static BPXTYPE queryBreakpoint(duint addr);
    static std::atomic<DbgAdapter*> sInstance;

    static void onCreateProcess(pid_t pid, uint64_t entryPoint, void* userdata);
    static void onExitProcess(int exitCode, void* userdata);
    static void onCreateThread(pid_t tid, void* userdata);
    static void onExitThread(pid_t tid, void* userdata);
    static void onSystemBreakpoint(void* userdata);
    static void onBreakpoint(uint64_t address, void* userdata);
    static void onStep(void* userdata);
    static void onPaused(void* userdata);
    static void onException(int signal, uint64_t address, void* userdata);
    static void onError(const char* error, void* userdata);
    static void onDebugString(const char* text, void* userdata);

    [[nodiscard]] QString threadSuffix() const;
    [[nodiscard]] REGDUMP readRegisters() const;
    [[nodiscard]] std::vector<ElfBugThreadInfo> readThreadList() const;
    void emitStoppedState(const QString & reason);
    void emitStoppedState(const QString & reason, const REGDUMP & dump);

    ElfBugDebugger* mDebugger = nullptr;
    duint mEntryPoint = 0;

    std::mutex mThreadNameMutex;
    QHash<pid_t, QString> mThreadNames;
};
