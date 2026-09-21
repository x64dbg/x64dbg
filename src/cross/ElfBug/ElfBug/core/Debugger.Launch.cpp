#include <ElfBug/core/Debugger.h>
#include <ElfBug/process/ProcessArch.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <vector>

namespace ElfBug
{
    bool Debugger::launchChild()
    {
        if(!mHasLaunchArgs)
        {
            cbInternalError("launchChild called without Init");
            return false;
        }

        int pipeFds[2];
        if(pipe2(pipeFds, O_CLOEXEC) == -1)
        {
            cbInternalError("pipe2() failed: " + std::string(strerror(errno)));
            return false;
        }

        const pid_t pid = fork();
        if(pid == -1)
        {
            close(pipeFds[0]);
            close(pipeFds[1]);
            cbInternalError("fork() failed: " + std::string(strerror(errno)));
            return false;
        }

        if(pid == 0)
        {
            close(pipeFds[0]);

            auto childError = [&](const char* msg)
            {
                write(pipeFds[1], msg, strlen(msg));
                _exit(1);
            };

            if(setpgid(0, 0) < 0)
                childError("setpgid failed");

            if(!mCwd.empty())
            {
                if(chdir(mCwd.c_str()) == -1)
                    childError("chdir failed");
            }

            const int persona = personality(0xffffffff);
            if(persona != -1)
                (void)personality(static_cast<unsigned long>(persona) | ADDR_NO_RANDOMIZE);

            if(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1)
                childError("PTRACE_TRACEME failed");

            std::vector<char*> argvPtrs;
            if(!mArgv.empty())
            {
                argvPtrs.reserve(mArgv.size() + 1);
                for(auto & s : mArgv)
                    argvPtrs.push_back(s.data());
                argvPtrs.push_back(nullptr);
                execv(mFilePath.c_str(), argvPtrs.data());
            }
            else
            {
                char* defaultArgv[] = { const_cast<char*>(mFilePath.c_str()), nullptr };
                execv(mFilePath.c_str(), defaultArgv);
            }

            childError("execv failed");
        }

        close(pipeFds[1]);

        char errBuf[256] = {};
        ssize_t n;
        do
        {
            n = read(pipeFds[0], errBuf, sizeof(errBuf) - 1);
        }
        while(n == -1 && errno == EINTR);
        close(pipeFds[0]);

        if(n == -1)
        {
            const std::string err = strerror(errno);
            waitpid(pid, nullptr, 0);
            cbInternalError("read() from child pipe failed: " + err);
            return false;
        }

        if(n > 0)
        {
            waitpid(pid, nullptr, 0);
            cbInternalError("child process failed: " + std::string(errBuf));
            return false;
        }

        mMainPid.store(pid, std::memory_order_release);
        return true;
    }

    bool Debugger::startLaunchedProcess()
    {
        if(!launchChild())
            return false;

        const pid_t mainPid = mMainPid.load(std::memory_order_relaxed);

        int status = 0;
        if(waitpid(mainPid, &status, __WALL) == -1)
        {
            cbInternalError("initial waitpid() failed: " + std::string(strerror(errno)));
            return false;
        }

        if(!WIFSTOPPED(status))
        {
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -WTERMSIG(status);
            cbInternalError("child exited before reaching first stop (code " + std::to_string(code) + ")");
            cbExitProcess(code);
            return false;
        }

        if(ptrace(PTRACE_SETOPTIONS, mainPid, nullptr, kLaunchPtraceOptions) == -1)
        {
            cbInternalError("PTRACE_SETOPTIONS failed: " + std::string(strerror(errno)));
            return false;
        }

        const Arch detectedArch = DetectArchFromProcExe(mainPid);
        if(detectedArch != Arch::X86_64)
        {
            cbInternalError("cannot debug " + mFilePath + ": " + ArchRejectMessage(detectedArch));
            kill(mainPid, SIGKILL);
            int killStatus = 0;
            waitpid(mainPid, &killStatus, __WALL);
            mMainPid.store(0, std::memory_order_release);
            const int exitCode = WIFEXITED(killStatus) ? WEXITSTATUS(killStatus)
                                 : -WTERMSIG(killStatus);
            cbExitProcess(exitCode);
            return false;
        }

        createProcessEvent(mainPid, detectedArch);
        return true;
    }
}
