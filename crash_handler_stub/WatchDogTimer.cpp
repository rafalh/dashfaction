#include <windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <log/Logger.h>
#include "crash_handler_stub/custom_exceptions.h"
#include "crash_handler_stub.h"
#include "crash_handler_stub/WatchDogTimer.h"

class WatchDogTimer::Impl
{
    unsigned m_interval_ms;
    HANDLE m_observed_thread_handle;
    DWORD m_observed_thread_id;
    DWORD m_ticks;
    std::thread m_checker_thread;
    std::condition_variable m_cond_var;
    std::mutex m_mutex;
    std::atomic<bool> m_exiting;
    bool m_running = false;

public:
    Impl(int interval_ms) : m_interval_ms(interval_ms)
    {}

    void Start()
    {
        if (m_running) {
            ERR("Trying to start a running watch-dog timer");
            return;
        }

        if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &m_observed_thread_handle, 0,
            FALSE, DUPLICATE_SAME_ACCESS)) {
            WARN("DuplicateHandle failed");
        }
        m_observed_thread_id = GetCurrentThreadId();

        m_ticks = GetTickCount();
        m_exiting = false;

        m_checker_thread = std::thread{&WatchDogTimer::Impl::CheckerThreadProc, this};
        m_running = true;
        INFO("Watchdog timer started");
    }

    void Stop()
    {
        if (!m_running) {
            ERR("Trying to stop a watch-dog timer that is not running");
            return;
        }
        m_exiting = true;
        m_cond_var.notify_all();
        m_checker_thread.join();
        m_running = false;
        INFO("Watchdog timer stopped");
    }

    void Restart()
    {
        m_ticks = GetTickCount();
    }

    bool IsRunning()
    {
        return m_running;
    }

private:
    void CheckerThreadProc()
    {
        while (!m_exiting) {
            if (CheckForTimeOut()) {
                HandleTimeOut();
            }
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cond_var.wait_for(lk, std::chrono::milliseconds{m_interval_ms/2});
        }
    }

    bool CheckForTimeOut()
    {
        return GetTickCount() - m_ticks >= m_interval_ms;
    }

    void HandleTimeOut()
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
        CrashHandlerStubProcessException(&exc_ptrs, m_observed_thread_id);
        ExitProcess(0);
    }
};

WatchDogTimer::WatchDogTimer(int interval_ms) : m_impl(new WatchDogTimer::Impl(interval_ms))
{
}

WatchDogTimer::~WatchDogTimer()
{
}

void WatchDogTimer::Start()
{
    m_impl->Start();
}

void WatchDogTimer::Stop()
{
    m_impl->Stop();
}

void WatchDogTimer::Restart()
{
    m_impl->Restart();
}

bool WatchDogTimer::IsRunning()
{
    return m_impl->IsRunning();
}
