#pragma once

#include "GameConfig.h"
#include <stdexcept>

class PrivilegeElevationRequiredException : public std::runtime_error
{
public:
    PrivilegeElevationRequiredException() : std::runtime_error("privilage elevation required") {}
};

class IntegrityCheckFailedException : public std::runtime_error
{
public:
    IntegrityCheckFailedException(uint32_t crc32) : std::runtime_error("integrity check failed"), m_crc32(crc32) {}

    uint32_t getCrc32() const
    {
        return m_crc32;
    }

private:
    uint32_t m_crc32;
};

class LauncherError : public std::runtime_error
{
public:
    LauncherError(const char* message) : std::runtime_error(message) {}
};

class Process;

class ModdedAppLauncher
{
public:
    ModdedAppLauncher(const char* mod_dll_name, uint32_t expected_crc32) :
        m_mod_dll_name(mod_dll_name), m_expected_crc32(expected_crc32)
    {}
    void launch(const char* mod_name = nullptr);

protected:
    std::string get_mod_dll_path();
    virtual std::string get_app_path() = 0;

    std::string m_mod_dll_name;
    uint32_t m_expected_crc32;
};

class GameLauncher : public ModdedAppLauncher
{
public:
    GameLauncher();
    std::string get_app_path() override;

private:
    GameConfig m_conf;
};

class EditorLauncher : public ModdedAppLauncher
{
public:
    EditorLauncher();
    std::string get_app_path() override;

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
