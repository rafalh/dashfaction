#pragma once

template<typename T>
class ComPtr
{
private:
    T *m_ptr = nullptr;

public:
    ComPtr() = default;

    ComPtr(const ComPtr& other) :
        m_ptr(other.m_ptr)
    {
        if (m_ptr)
            m_ptr->AddRef();
    }

    ~ComPtr()
    {
        release();
    }

    ComPtr& operator=(const ComPtr& other)
    {
        if (&other != this) {
            release();
            m_ptr = other.m_ptr;
            if (m_ptr)
                m_ptr->AddRef();
        }
        return *this;
    }

    T** operator&()
    {
        release();
        return &m_ptr;
    }

    operator T*()
    {
        return m_ptr;
    }

    T* operator->()
    {
        return m_ptr;
    }

    void release()
    {
        if (m_ptr)
            m_ptr->Release();
        m_ptr = nullptr;
    }
};
