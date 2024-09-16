#pragma once

#include <utility>

class DynamicLinkLibrary
{
public:
    DynamicLinkLibrary(const wchar_t* filename)
    {
        handle_ = LoadLibraryW(filename);
    }

    DynamicLinkLibrary(const DynamicLinkLibrary& other) = delete;

    DynamicLinkLibrary(DynamicLinkLibrary&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr))
    {}

    ~DynamicLinkLibrary()
    {
        if (handle_) {
            FreeLibrary(handle_);
        }
    }

    DynamicLinkLibrary& operator=(const DynamicLinkLibrary& other) = delete;

    DynamicLinkLibrary& operator=(DynamicLinkLibrary&& other) noexcept
    {
        std::swap(handle_, other.handle_);
        return *this;
    }

    operator bool() const
    {
        return handle_ != nullptr;
    }

    template<typename T>
    T get_proc_address(const char* name) const
    {
        static_assert(std::is_pointer_v<T>);
        // Note: double cast is needed to fix cast-function-type GCC warning
        return reinterpret_cast<T>(reinterpret_cast<void(*)()>(GetProcAddress(handle_, name)));
    }

private:
    HMODULE handle_;
};
