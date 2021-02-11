#pragma once

#include <functional>

template<typename T>
class CfgVar
{
private:
    T value_;
    std::function<bool(T)> validator_;
    bool dirty_ = true;

public:
    CfgVar(T default_value, std::function<bool(T)> validator = default_validator) :
        value_(default_value), validator_(validator)
    {}

    operator T() const
    {
        return value_;
    }

    T operator=(T&& value)
    {
        assign(value);
        return value_;
    }

    T operator=(const T& value)
    {
        assign(value);
        return value_;
    }

    const T* operator->() const
    {
        return &value_;
    }

    const T* operator&() const
    {
        return &value_;
    }

    const T& value() const
    {
        return value_;
    }

    bool assign(const T& value)
    {
        if (!validator_(value)) {
            return false;
        }
        if (value_ != value) {
            value_ = value;
            dirty_ = true;
        }
        return true;
    }

    bool assign(T&& value)
    {
        if (!validator_(value)) {
            return false;
        }
        if (value_ != value) {
            value_ = value;
            dirty_ = true;
        }
        return true;
    }

    bool is_dirty() const
    {
        return dirty_;
    }

    void set_dirty(bool dirty)
    {
        dirty_ = dirty;
    }

private:
    static bool default_validator(T)
    {
        return true;
    }
};
