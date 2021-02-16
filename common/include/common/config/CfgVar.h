#pragma once

#include <functional>

template<typename T>
class CfgVar
{
private:
    T value_;
    std::function<T(T)> assign_callback_;
    bool dirty_ = true;

public:
    CfgVar(T default_value, std::function<T(T)> assign_callback = default_assign_callback) :
        value_(default_value), assign_callback_(assign_callback)
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
        T corrected_value = assign_callback_(value);
        if (value_ != corrected_value) {
            value_ = corrected_value;
            dirty_ = true;
        }
        return true;
    }

    bool assign(T&& value)
    {
        T corrected_value = assign_callback_(value);
        if (value_ != corrected_value) {
            value_ = corrected_value;
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
    static T default_assign_callback(T value)
    {
        return value;
    }
};
