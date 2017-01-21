#pragma once

#include "GameConfig.h"

class PrivilegeElevationRequiredException : public std::exception
{
};

class IntegrityCheckFailedException : public std::exception
{
public:
    IntegrityCheckFailedException(uint32_t crc) : m_crc(crc) {}

    uint32_t getCrc32()
    {
        return m_crc;
    }

private:
    uint32_t m_crc;
};


class ModdedAppLauncher
{
public:
    ModdedAppLauncher(const char *modDllName, uint32_t expectedCrc) :
        m_modDllName(modDllName), m_expectedCrc(expectedCrc) {}
    void launch();

protected:
    std::string getModDllPath();
    void injectDLL(HANDLE hProcess, const TCHAR *pszPath);
    virtual std::string getAppPath() = 0;

    std::string m_modDllName;
    uint32_t m_expectedCrc;
};

class GameStarter : public ModdedAppLauncher
{
public:
    GameStarter();
    virtual std::string getAppPath();

private:
    GameConfig m_conf;
};