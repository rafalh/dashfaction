#pragma once

#include <stdexcept>
#include <windows.h>
#include <common/Win32Error.h>

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
            throw Win32Error(error, "RegCreateKeyExA failed");
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

    bool read_value(const char* name, unsigned* value)
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

    bool read_value(const char* name, std::string* value)
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

    bool read_value(const char* name, bool* value)
    {
        unsigned temp;
        bool result = read_value(name, &temp);
        if (result)
            *value = (temp != 0);
        return result;
    }

    void write_value(const char* name, unsigned value)
    {
        DWORD temp = value;
        LONG error = RegSetValueExA(m_key, name, 0, REG_DWORD, (BYTE*)&temp, sizeof(temp));
        if (error != ERROR_SUCCESS)
            throw Win32Error(error, "RegSetValueExA failed");
    }

    void write_value(const char* name, const std::string& value)
    {
        LONG error = RegSetValueExA(m_key, name, 0, REG_SZ, (BYTE*)value.c_str(), value.size() + 1);
        if (error != ERROR_SUCCESS)
            throw Win32Error(error, "RegSetValueExA failed");
    }

    void write_value(const char* name, bool value)
    {
        write_value(name, value ? 1u : 0u);
    }

    bool is_open() const
    {
        return m_open;
    }

private:
    HKEY m_key = NULL;
    bool m_open = false;
};
