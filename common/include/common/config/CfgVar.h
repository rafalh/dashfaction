#pragma once

template<typename T>
class CfgVar
{
private:
    T value_;
    bool dirty_ = true;

public:
    CfgVar(T default_value) : value_(default_value)
    {}

    operator T() const
    {
        return value_;
    }

    T operator=(T&& value)
    {
        if (value_ != value) {
            value_ = value;
            dirty_ = true;
        }
        return value_;
    }

    T operator=(const T& value)
    {
        if (value_ != value) {
            value_ = value;
            dirty_ = true;
        }
        return value_;
    }

    T* operator->()
    {
        return &value_;
    }

    T* operator&()
    {
        return &value_;
    }

    T& value()
    {
        return value_;
    }

    bool is_dirty() const
    {
        return dirty_;
    }

    void set_dirty(bool dirty)
    {
        dirty_ = dirty;
    }
};
