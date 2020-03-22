#pragma once

#include <common/GameConfig.h>
#include <stdexcept>

class FileNotFoundException : public std::runtime_error
{
public:
    FileNotFoundException(const std::string& file_name) :
        std::runtime_error("file not found"), m_file_name(file_name)
    {}

    const std::string& get_file_name() const
    {
        return m_file_name;
    }

private:
    std::string m_file_name;
};

class FileHashVerificationException : public std::runtime_error
{
public:
    FileHashVerificationException(const std::string& file_name, const std::string& sha1) :
        std::runtime_error("file hash sum verification failed"), m_file_name(file_name), m_sha1(sha1)
    {}

    const std::string& get_file_name() const
    {
        return m_file_name;
    }

    const std::string& get_sha1() const
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
    void launch(const char* mod_name = nullptr);

protected:
    std::string get_patch_dll_path();
    virtual std::string get_app_path() = 0;
    virtual bool check_app_hash(const std::string&) = 0;

    std::string m_patch_dll_name;
};

class GameLauncher : public PatchedAppLauncher
{
public:
    GameLauncher();
    std::string get_app_path() override;
    bool check_app_hash(const std::string& sha1) override;

private:
    GameConfig m_conf;
};

class EditorLauncher : public PatchedAppLauncher
{
public:
    EditorLauncher();
    std::string get_app_path() override;
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
