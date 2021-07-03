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
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    ComPtr(ComPtr&& other) :
        m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr;
    }

    ComPtr(T* ptr) :
        m_ptr(ptr)
    {
        if (m_ptr) {
            m_ptr->AddRef();
        }
    }

    ~ComPtr()
    {
        release();
    }

    ComPtr& operator=(const ComPtr& other)
    {
        return (*this = other.m_ptr);
    }

    ComPtr& operator=(ComPtr&& other)
    {
        if (other.m_ptr != m_ptr) {
            release();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    ComPtr& operator=(T* ptr)
    {
        if (m_ptr != ptr) {
            release();
            m_ptr = ptr;
            if (m_ptr) {
                m_ptr->AddRef();
            }
        }
        return *this;
    }

    T** operator&()
    {
        release();
        return &m_ptr;
    }

    operator T*() const
    {
        return m_ptr;
    }

    T* operator->() const
    {
        return m_ptr;
    }

    T* get() const
    {
        return m_ptr;
    }

    void release()
    {
        if (m_ptr) {
            m_ptr->Release();
        }
        m_ptr = nullptr;
    }
};
