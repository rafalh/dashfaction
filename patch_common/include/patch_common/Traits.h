#pragma once

// For generic types that are functors, delegate to its 'operator()'
template<typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{
};

// for pointers to member function
template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...) const>
{
    // enum { arity = sizeof...(Args) };
    typedef ReturnType f_type(Args...);
};

// for pointers to member function
template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType (ClassType::*)(Args...)>
{
    typedef ReturnType f_type(Args...);
};

// for cdecl function pointers
template<typename ReturnType, typename... Args>
struct function_traits<ReturnType (__cdecl*)(Args...)>
{
    typedef ReturnType __cdecl f_type(Args...);
};

// for fastcall function pointers
template<typename ReturnType, typename... Args>
struct function_traits<ReturnType (__fastcall*)(Args...)>
{
    typedef ReturnType __fastcall f_type(Args...);
};
