#pragma once

#include <wxx_wincore.h>
#include <wxx_dialog.h>
#include <wxx_controls.h>
#include <functional>
#include <thread>
#include <optional>
#include <common/error/error-utils.h>
#include "res/resource.h"

class ProgressDlg : public CDialog
{
    std::string m_title;
    std::function<void()> m_callback;
    std::thread m_thread;
    std::optional<std::string> m_error;

public:
    ProgressDlg(const std::string title, std::function<void()> callback) :
        CDialog(IDD_PROGRESS), m_title(title), m_callback(callback)
    {}

    std::optional<std::string> GetError() const
    {
        return m_error;
    }

    void ThrowIfError() const
    {
        if (m_error)
            throw std::runtime_error(m_error.value());
    }

protected:
    BOOL OnInitDialog() override
    {
        SetDlgItemText(IDC_TITLE, m_title.c_str());

        auto progress_bar = GetDlgItem(IDC_PROGRESS_BAR);
        progress_bar.SetStyle(progress_bar.GetStyle() | PBS_MARQUEE);
        progress_bar.SendMessage(PBM_SETMARQUEE, 1, 0);
        RunCallback();
        return TRUE;
    }

    void OnClose() override
    {
        m_thread.join();
    }

private:
    void RunCallback()
    {
        m_thread = std::thread{&ProgressDlg::ThreadProc, this};
    }

    void ThreadProc()
    try {
        m_callback();
        PostMessage(WM_CLOSE, 0, 0);
    }
    catch (std::exception& e) {
        m_error = generate_message_for_exception(e);
        PostMessage(WM_CLOSE, 0, 0);
    }
};
