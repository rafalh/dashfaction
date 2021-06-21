#pragma once

#include <wxx_wincore.h>
#include <crash_handler_stub.h>

class CommandLineInfo;

class CrashReportApp : public CWinApp
{
public:
    virtual int Run() override;
    std::string GetArchivedReportFilePath() const;

private:
    void PrepareReport(const CommandLineInfo& cmd_line_info);
    void ArchiveReport(const char* crash_dump_filename, const char* exc_info_filename);
    void SendReport() const;
    static int Message(HWND hwnd, const char *pszText, const char *pszTitle, int Flags);

    CrashHandlerConfig m_config;
};

inline CrashReportApp* GetCrashReportApp()
{
    return static_cast<CrashReportApp*>(GetApp());
}
