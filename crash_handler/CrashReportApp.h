#pragma once

#include <wxx_wincore.h>

class CommandLineInfo;

class CrashReportApp : public CWinApp
{
public:
    virtual BOOL InitInstance() override;
    std::string GetArchivedReportFilePath() const;

private:
    void PrepareReport(const CommandLineInfo& cmd_line_info);
    void SendReport();
    int Message(HWND hwnd, const char *pszText, const char *pszTitle, int Flags);
};

inline CrashReportApp* GetCrashReportApp()
{
    return static_cast<CrashReportApp*>(GetApp());
}
