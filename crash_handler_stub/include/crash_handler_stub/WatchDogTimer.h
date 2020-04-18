#pragma once

#include <memory>

class WatchDogTimer
{
    class Impl;
    std::unique_ptr<Impl> m_impl;

public:
    WatchDogTimer(int interval_ms = 3000);
    ~WatchDogTimer();
    void Start();
    void Stop();
    void Restart();
    bool IsRunning();

    class ScopedStartStop
    {
        WatchDogTimer& m_timer;

    public:
        ScopedStartStop(WatchDogTimer& timer) : m_timer(timer)
        {
            m_timer.Start();
        }

        ~ScopedStartStop()
        {
            m_timer.Stop();
        }
    };

    class ScopedPause
    {
        WatchDogTimer& m_timer;
        bool m_running;

    public:
        ScopedPause(WatchDogTimer& timer) : m_timer(timer)
        {
            m_running = m_timer.IsRunning();
            if (m_running) {
                m_timer.Stop();
            }
        }

        ~ScopedPause()
        {
            if (m_running) {
                m_timer.Start();
            }
        }
    };
};
