#pragma once

#include <utility>
#include <cstdint>
#include <windows.h>
#include <common/error/Win32Error.h>

template<typename T>
bool is_valid_enum_value(int value);

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

    bool read_value(const char* name, uint32_t* value)
    {
        if (!m_open)
            return false;
        DWORD temp_value, type, cb = sizeof(temp_value);
        LONG error = RegQueryValueExA(m_key, name, nullptr, &type, reinterpret_cast<BYTE*>(&temp_value), &cb);
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
        LONG error = RegQueryValueExA(m_key, name, nullptr, &type, nullptr, &cb);
        if (error != ERROR_SUCCESS)
            return false;
        if (type != REG_SZ && type != REG_EXPAND_SZ)
            return false;

        // Note: value can but not have to be terminated with null character, therefore additional null character
        // is added here by resizing string to bigger buffer (it is initialized to zeros because it is cleared first)
        value->clear();
        value->resize(cb + 1);

        error = RegQueryValueExA(m_key, name, nullptr, &type, reinterpret_cast<BYTE*>(value->data()), &cb);
        if (error != ERROR_SUCCESS) {
            return false;
        }
        value->resize(value->find('\0'));
        return true;
    }

    bool read_value(const char* name, bool* value)
    {
        uint32_t temp;
        bool result = read_value(name, &temp);
        if (result)
            *value = (temp != 0);
        return result;
    }

    bool read_value(const char* name, int* value)
    {
        uint32_t temp;
        bool result = read_value(name, &temp);
        if (result)
            *value = static_cast<int>(temp);
        return result;
    }

    bool read_value(const char* name, float* value)
    {
        static_assert(sizeof(float) == sizeof(uint32_t));
        union {
            float f;
            uint32_t u32;
        } u;
        bool result = read_value(name, &u.u32);
        if (result)
            *value = u.f;
        return result;
    }

    template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    bool read_value(const char* name, T* value)
    {
        int temp;
        bool result = read_value(name, &temp) && is_valid_enum_value<T>(temp);
        if (result)
            *value = static_cast<T>(temp);
        return result;
    }

    void write_value(const char* name, uint32_t value)
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

    void write_value(const char* name, int value)
    {
        write_value(name, static_cast<uint32_t>(value));
    }

    void write_value(const char* name, float value)
    {
        static_assert(sizeof(float) == sizeof(uint32_t));
        union {
            float f;
            uint32_t u32;
        } u;
        u.f = value;
        write_value(name, u.u32);
    }

    template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    void write_value(const char* name, T value)
    {
        write_value(name, static_cast<int>(value));
    }

    [[nodiscard]] bool is_open() const
    {
        return m_open;
    }

private:
    HKEY m_key = nullptr;
    bool m_open = false;
};
