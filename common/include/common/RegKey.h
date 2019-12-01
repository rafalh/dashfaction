#pragma once

#include <stdexcept>
#include <windows.h>
#include <memory>
#include <common/Win32Error.h>

class RegKey
{
public:
    RegKey(RegKey const&) = delete;
    RegKey& operator=(RegKey const&) = delete;

    RegKey(RegKey&& other) noexcept :
        m_key(std::exchange(other.m_key, nullptr)), m_open(std::exchange(other.m_open, false))
    {
    }

    RegKey& operator=(RegKey&& other) noexcept
    {
        std::swap(m_key, other.m_key);
        std::swap(m_open, other.m_open);
        return *this;
    }

    RegKey(HKEY parent_key, const char* sub_key, REGSAM access_rights, bool create = false)
    {
        if (!parent_key)
            return; // empty parent

        LONG error;
        if (create)
            error = RegCreateKeyExA(parent_key, sub_key, 0, nullptr, 0, access_rights, nullptr, &m_key, nullptr);
        else {
            error = RegOpenKeyExA(parent_key, sub_key, 0, access_rights, &m_key);
            if (error == ERROR_FILE_NOT_FOUND)
                return; // dont throw exception in this case
        }

        if (error != ERROR_SUCCESS)
            throw Win32Error(error, "RegCreateKeyExA failed");
        m_open = true;
    }

    RegKey(const RegKey& parent_key, const char* sub_key, REGSAM access_rights, bool create = false) :
        RegKey(parent_key.m_key, sub_key, access_rights, create)
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
        DWORD temp_value, type, cb = sizeof(temp_value);
        LONG error = RegQueryValueEx(m_key, name, nullptr, &type, reinterpret_cast<BYTE*>(&temp_value), &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_DWORD)
            return false;
        *value = temp_value;
        return true;
    }

    bool read_value(const char* name, std::string* value)
    {
        if (!m_open)
            return false;
        DWORD type, cb = 0;
        LONG error = RegQueryValueEx(m_key, name, nullptr, &type, nullptr, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_SZ && type != REG_EXPAND_SZ)
            return false;

        auto buf = std::make_unique<char[]>(cb + 1);
        error = RegQueryValueEx(m_key, name, nullptr, &type, reinterpret_cast<BYTE*>(buf.get()), &cb);
        if (error != ERROR_SUCCESS) {
            return false;
        }
        buf[cb] = '\0';
        value->assign(buf.get());
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
        LONG error = RegSetValueExA(m_key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&temp), sizeof(temp));
        if (error != ERROR_SUCCESS)
            throw Win32Error(error, "RegSetValueExA failed");
    }

    void write_value(const char* name, const std::string& value)
    {
        LONG error = RegSetValueExA(m_key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), value.size() + 1);
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
    HKEY m_key = nullptr;
    bool m_open = false;
};
