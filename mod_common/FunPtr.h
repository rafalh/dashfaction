#pragma once

template<uintptr_t DefaultAddr, typename RetType, typename... ArgTypes>
class FunPtr
{
public:
    RetType operator()(ArgTypes... args)
    {
        return (reinterpret_cast<RetType(*)(ArgTypes...)>(m_addr))(args...);
    }

    void setAddr(uintptr_t addr)
    {
        m_addr = addr;
    }

    uintptr_t getAddr() const
    {
        return m_addr;
    }

protected:
    uintptr_t m_addr = DefaultAddr;
};

template<uintptr_t DefaultAddr, typename... ArgTypes>
class FunPtr<DefaultAddr, void, ArgTypes...>
{
public:
    void operator()(ArgTypes... args)
    {
        (reinterpret_cast<void(*)(ArgTypes...)>(m_addr))(args...);
    }

    void setAddr(uintptr_t addr)
    {
        m_addr = addr;
    }

    uintptr_t getAddr() const
    {
        return m_addr;
    }

protected:
    uintptr_t m_addr = DefaultAddr;
};
