#pragma once

#include <Windows.h>
#include <stdexcept>
#include "exception.h"

class RegKey
{
public:
    RegKey(HKEY parentKey, const char *subKey, REGSAM accessRights)
    {
        LONG error = RegCreateKeyExA(parentKey, subKey, 0, NULL, 0, accessRights, NULL, &m_key, NULL);
        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegCreateKeyExA failed: win32 error %lu", error);
    }

    RegKey(RegKey parentKey, const char *subKey, REGSAM accessRights) :
        RegKey(parentKey.m_key, subKey, accessRights) {}

    ~RegKey()
    {
        RegCloseKey(m_key);
    }

    bool readValue(const char *name, unsigned *value)
    {
        DWORD tempValue, type, cb = sizeof(tempValue);
        LONG error = RegQueryValueEx(m_key, name, NULL, &type, (LPBYTE)&tempValue, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_DWORD)
            return false;
        *value = tempValue;
        return true;
    }

    bool readValue(const char *name, std::string *value)
    {
        DWORD type, cb = 0;
        LONG error = RegQueryValueEx(m_key, name, NULL, &type, NULL, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_SZ && type != REG_EXPAND_SZ)
            return false;

        value->clear();
        value->resize(cb + 1);
        error = RegQueryValueEx(m_key, name, NULL, &type, (LPBYTE)value->data(), &cb);
        if (error != ERROR_SUCCESS)
            return false;
        ((LPBYTE)value->data())[cb] = 0;
        return true;
    }

    bool readValue(const char *name, bool *value)
    {
        unsigned temp;
        bool result = readValue(name, &temp);
        if (result)
            *value = (temp != 0);
        return result;
    }

    void writeValue(const char *name, unsigned value)
    {
        DWORD temp = value;
        LONG error = RegSetValueExA(m_key, name, NULL, REG_DWORD, (BYTE*)&temp, sizeof(temp));
        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegSetValueExA failed: win32 error %lu", error);
    }

    void writeValue(const char *name, const std::string &value)
    {
        LONG error = RegSetValueExA(m_key, name, NULL, REG_SZ, (BYTE*)value.c_str(), value.size() + 1);
        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegSetValueExA failed: win32 error %lu", error);
    }

    void writeValue(const char *name, bool value)
    {
        writeValue(name, value ? 1u : 0u);
    }

private:
    HKEY m_key;
};