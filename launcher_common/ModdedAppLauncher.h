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
    IntegrityCheckFailedException(uint32_t crc) : std::runtime_error("integrity check failed"), m_crc(crc) {}

    uint32_t getCrc32() const
    {
        return m_crc;
    }

private:
    uint32_t m_crc;
};

class ModdedAppLauncher
{
public:
    ModdedAppLauncher(const char* modDllName, uint32_t expectedCrc) :
        m_modDllName(modDllName), m_expectedCrc(expectedCrc)
    {}
    void launch();

protected:
    std::string getModDllPath();
    void injectDLL(HANDLE hProcess, const TCHAR* pszPath);
    virtual std::string getAppPath() = 0;

    std::string m_modDllName;
    uint32_t m_expectedCrc;
};

class GameLauncher : public ModdedAppLauncher
{
public:
    GameLauncher();
    std::string getAppPath() override;

private:
    GameConfig m_conf;
};

class EditorLauncher : public ModdedAppLauncher
{
public:
    EditorLauncher();
    std::string getAppPath() override;

private:
    GameConfig m_conf;
};
