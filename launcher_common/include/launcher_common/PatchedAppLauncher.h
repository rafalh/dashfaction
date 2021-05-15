#pragma once

#include <common/config/GameConfig.h>
#include <stdexcept>
#include <string>
#include <optional>
#include <vector>

struct _STARTUPINFOA; // NOLINT(bugprone-reserved-identifier)

class FileNotFoundException : public std::runtime_error
{
public:
    FileNotFoundException(std::string file_name) :
        std::runtime_error("file not found"), m_file_name(std::move(file_name))
    {}

    [[nodiscard]] const std::string& get_file_name() const
    {
        return m_file_name;
    }

private:
    std::string m_file_name;
};

class FileHashVerificationException : public std::runtime_error
{
public:
    FileHashVerificationException(std::string file_name, std::string sha1) :
        std::runtime_error("file hash sum verification failed"), m_file_name(std::move(file_name)), m_sha1(std::move(sha1))
    {}

    [[nodiscard]] const std::string& get_file_name() const
    {
        return m_file_name;
    }

    [[nodiscard]] const std::string& get_sha1() const
    {
        return m_sha1;
    }

private:
    std::string m_file_name;
    std::string m_sha1;
};

class PrivilegeElevationRequiredException : public std::runtime_error
{
public:
    PrivilegeElevationRequiredException() : std::runtime_error("privilage elevation required") {}
};

class LauncherError : public std::runtime_error
{
public:
    LauncherError(const char* message) : std::runtime_error(message) {}
};

class PatchedAppLauncher
{
public:
    PatchedAppLauncher(const char* patch_dll_name) :
        m_patch_dll_name(patch_dll_name)
    {}
    void check_installation();
    void launch();

    void set_app_exe_path(const std::string& app_exe_path)
    {
        m_forced_app_exe_path = {app_exe_path};
    }

    void set_args(const std::vector<std::string>& args)
    {
        m_args = args;
    }

    void set_mod(const std::string& mod_name)
    {
        m_mod_name = mod_name;
    }

protected:
    std::string get_patch_dll_path();
    std::string get_app_path();
    virtual std::string get_default_app_path() = 0;
    virtual bool check_app_hash(const std::string&) = 0;

private:
    void verify_before_launch();
    static void setup_startup_info(_STARTUPINFOA& startup_info);
    std::string build_cmd_line(const std::string& app_path);

    std::optional<std::string> m_forced_app_exe_path;
    std::string m_patch_dll_name;
    std::vector<std::string> m_args;
    std::string m_mod_name;
};

class GameLauncher : public PatchedAppLauncher
{
public:
    GameLauncher();
    std::string get_default_app_path() override;
    bool check_app_hash(const std::string& sha1) override;

private:
    GameConfig m_conf;
};

class EditorLauncher : public PatchedAppLauncher
{
public:
    EditorLauncher();
    std::string get_default_app_path() override;
    bool check_app_hash(const std::string& sha1) override;

private:
    GameConfig m_conf;
};


inline std::string get_dir_from_path(const std::string& path)
{
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos)
        return ".";
    return path.substr(0, pos);
}
