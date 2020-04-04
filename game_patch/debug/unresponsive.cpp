#include <windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <patch_common/FunHook.h>
#include "debug_internal.h"
#include "../console/console.h"
#include "crashhandler.h"

class UnresponsiveThreadDetector
{
    HANDLE m_observed_thread_handle;
    int m_ticks = 0;
    std::thread m_checker_thread;
    std::condition_variable m_cond_var;
    std::mutex m_mutex;
    bool m_exiting = false;

public:
    void ResetTicks()
    {
        m_ticks = GetTickCount();
    }

    void Start()
    {
        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &m_observed_thread_handle, 0,
            FALSE, DUPLICATE_SAME_ACCESS)) {
            WARN("DuplicateHandle failed");
        }
        m_checker_thread = std::thread{[this]() { CheckerThreadProc(); }};
    }

    void Stop()
    {
        m_exiting = true;
        m_cond_var.notify_all();
        m_checker_thread.join();
    }

    void CheckerThreadProc()
    {
        using namespace std::chrono_literals;
        while (!m_exiting) {
            if (CheckForUnresponsiveThread()) {
                HandleUnresponsiveThread();
            }
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond_var.wait_for(lk, 1000ms);
        }
    }

    bool CheckForUnresponsiveThread()
    {
        return m_ticks != 0 && GetTickCount() - m_ticks > 3000;
    }

    void HandleUnresponsiveThread()
    {
        if ((GetAsyncKeyState(VK_DELETE) & 0x8000)) {
            CrashObservedThread();
        }
        else {
            WARN("Application is not responding! Press DEL key to trigger a crash that will allow to debug the problem...");
        }
    }

    void CrashObservedThread()
    {
        SuspendThread(m_observed_thread_handle);
        CONTEXT ctx;
        ZeroMemory(&ctx, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_FULL;
        if (!GetThreadContext(m_observed_thread_handle, &ctx)) {
            WARN("GetThreadContext failed");
        }

        // Simulate exception
        EXCEPTION_POINTERS exc_ptrs;
        EXCEPTION_RECORD exc_rec;
        exc_ptrs.ContextRecord = &ctx;
        exc_ptrs.ExceptionRecord = &exc_rec;
        exc_rec.ExceptionCode = custom_exceptions::unresponsive;
        exc_rec.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
        exc_rec.ExceptionRecord = nullptr;
        exc_rec.ExceptionAddress = reinterpret_cast<void*>(ctx.Eip);
        exc_rec.NumberParameters = 0;
        CrashHandlerProcessException(&exc_ptrs, GetThreadId(m_observed_thread_handle));
        ExitProcess(0);
    }
};

UnresponsiveThreadDetector g_detector;

FunHook<void()> gr_flip_hook{
    0x0050CE20,
    []() {
        // Resetting detector is needed here to handle Bink videos properly
        g_detector.ResetTicks();
        gr_flip_hook.CallTarget();
    },
};

#ifndef NDEBUG
DcCommand2 hang_cmd{
    "d_hang",
    []() {
        int start = GetTickCount();
        while (GetTickCount() - start < 6000) {}
    },
};
#endif

void DebugUnresponsiveApplyPatches()
{
    gr_flip_hook.Install();
}

void DebugUnresponsiveInit()
{
    g_detector.Start();
#ifndef NDEBUG
    hang_cmd.Register();
#endif
}

void DebugUnresponsiveCleanup()
{
    g_detector.Stop();
}

void DebugUnresponsiveDoUpdate()
{
    g_detector.ResetTicks();
}
