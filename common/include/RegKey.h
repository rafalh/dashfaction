#pragma once

#include "Exception.h"
#include <stdexcept>
#include <windows.h>

class RegKey
{
public:
    RegKey(RegKey const&) = delete;
    RegKey& operator=(RegKey const&) = delete;

    RegKey(HKEY parentKey, const char* subKey, REGSAM accessRights, bool create = false)
    {
        if (!parentKey)
            return; // empty parent

        LONG error;
        if (create)
            error = RegCreateKeyExA(parentKey, subKey, 0, NULL, 0, accessRights, NULL, &m_key, NULL);
        else {
            error = RegOpenKeyExA(parentKey, subKey, 0, accessRights, &m_key);
            if (error == ERROR_FILE_NOT_FOUND)
                return; // dont throw exception in this case
        }

        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegCreateKeyExA failed: win32 error %lu", error);
        m_open = true;
    }

    RegKey(const RegKey& parentKey, const char* subKey, REGSAM accessRights, bool create = false) :
        RegKey(parentKey.m_key, subKey, accessRights, create)
    {}

    ~RegKey()
    {
        if (m_open)
            RegCloseKey(m_key);
    }

    bool readValue(const char* name, unsigned* value)
    {
        if (!m_open)
            return false;
        DWORD tempValue, type, cb = sizeof(tempValue);
        LONG error = RegQueryValueEx(m_key, name, NULL, &type, (LPBYTE)&tempValue, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_DWORD)
            return false;
        *value = tempValue;
        return true;
    }

    bool readValue(const char* name, std::string* value)
    {
        if (!m_open)
            return false;
        DWORD type, cb = 0;
        LONG error = RegQueryValueEx(m_key, name, NULL, &type, NULL, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_SZ && type != REG_EXPAND_SZ)
            return false;

        char* buf = new char[cb + 1];
        error = RegQueryValueEx(m_key, name, NULL, &type, (LPBYTE)buf, &cb);
        if (error != ERROR_SUCCESS) {
            delete[] buf;
            return false;
        }
        buf[cb] = '\0';
        value->assign(buf);
        delete[] buf;
        return true;
    }

    bool readValue(const char* name, bool* value)
    {
        unsigned temp;
        bool result = readValue(name, &temp);
        if (result)
            *value = (temp != 0);
        return result;
    }

    void writeValue(const char* name, unsigned value)
    {
        DWORD temp = value;
        LONG error = RegSetValueExA(m_key, name, 0, REG_DWORD, (BYTE*)&temp, sizeof(temp));
        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegSetValueExA failed: win32 error %lu", error);
    }

    void writeValue(const char* name, const std::string& value)
    {
        LONG error = RegSetValueExA(m_key, name, 0, REG_SZ, (BYTE*)value.c_str(), value.size() + 1);
        if (error != ERROR_SUCCESS)
            THROW_EXCEPTION("RegSetValueExA failed: win32 error %lu", error);
    }

    void writeValue(const char* name, bool value)
    {
        writeValue(name, value ? 1u : 0u);
    }

    bool isOpen() const
    {
        return m_open;
    }

private:
    HKEY m_key = NULL;
    bool m_open = false;
};
