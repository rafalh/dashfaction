#pragma once

#include <memory>

class WatchDogTimer
{
    class Impl;
    std::unique_ptr<Impl> m_impl;

public:
    WatchDogTimer(unsigned timeout_ms = 5000);
    ~WatchDogTimer();
    void start();
    void stop();
    void restart();
    bool is_running();

    class ScopedStartStop
    {
        WatchDogTimer& m_timer;

    public:
        ScopedStartStop(WatchDogTimer& timer) : m_timer(timer)
        {
            m_timer.start();
        }

        ~ScopedStartStop()
        {
            m_timer.stop();
        }
    };

    class ScopedPause
    {
        WatchDogTimer& m_timer;
        bool m_running;

    public:
        ScopedPause(WatchDogTimer& timer) : m_timer(timer)
        {
            m_running = m_timer.is_running();
            if (m_running) {
                m_timer.stop();
            }
        }

        ~ScopedPause()
        {
            if (m_running) {
                m_timer.start();
            }
        }
    };
};
