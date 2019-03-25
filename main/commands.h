#pragma once

#include "rf.h"
#include <functional>
#include <Traits.h>

namespace rf
{
    struct DcCommand;
    struct Player;
}

class DcInvalidArgTypeError : public std::exception {};
class DcRequiredArgMissingError : public std::exception {};

inline bool ReadArgInternal(int TypeFlag, bool PreserveCase = false)
{
    rf::DcGetArg(rf::DC_ARG_ANY, PreserveCase);
    if (rf::g_DcArgType & TypeFlag)
        return true;
    if (!(rf::g_DcArgType & rf::DC_ARG_NONE))
        throw DcInvalidArgTypeError();
    return false;
}

template<typename T>
inline T DcReadArg()
{
    auto ValueOpt = DcReadArg<std::optional<T>>();
    if (!ValueOpt)
        throw DcRequiredArgMissingError();
    return ValueOpt.value();
}

template<>
inline std::optional<int> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_INT))
        return {};
    return std::optional{rf::g_iDcArg};
}

template<>
inline std::optional<float> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_FLOAT))
        return {};
    return std::optional{rf::g_fDcArg};
}

template<>
inline std::optional<bool> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_TRUE | rf::DC_ARG_FALSE))
        return {};
    bool Value = (rf::g_DcArgType & rf::DC_ARG_TRUE) == rf::DC_ARG_TRUE;
    return std::optional{Value};
}

template<>
inline std::optional<std::string> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_STR))
        return {};
    return std::optional{rf::g_pszDcArg};
}

template<typename... Args>
class DcCommand2;

template<typename... Args>
class DcCommand2<void(Args...)> : public rf::DcCommand
{
private:
    std::function<void(Args...)> m_handler_fun;
    const char *m_usage_text;

public:
    DcCommand2(const char *name, void(*handler_fun)(Args...),
        const char *description = nullptr, const char *usage_text = nullptr) :
        m_handler_fun(handler_fun), m_usage_text(usage_text)
    {
        pszCmd = name;
        pszDescr = description;
        pfnHandler = (void (*)()) StaticHandler;
    }

    void Register()
    {
        CommandRegister(this);
    }

private:
    static void __fastcall StaticHandler(DcCommand2 *cmd)
    {
        cmd->Handler();
    }

    void Handler()
    {
        if (rf::g_bDcRun)
            Run();
        else if (rf::g_bDcHelp)
            Help();
    }

    void Run()
    {
        try {
            m_handler_fun(DcReadArg<Args>()...);
        }
        catch (DcInvalidArgTypeError e) {
            rf::DcPrint("Invalid arg type!", nullptr);
        }
        catch (DcRequiredArgMissingError e) {
            rf::DcPrint("Required arg is missing!", nullptr);
        }
    }

    void Help() {
        if (m_usage_text) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     %s", m_usage_text);
        }
    }
};

// TODO: move to different header

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template <class T>
DcCommand2(const char *, T)
    -> DcCommand2<typename function_traits<T>::f_type>;
template <class T>
DcCommand2(const char *, T, const char *)
    -> DcCommand2<typename function_traits<T>::f_type>;
template <class T>
DcCommand2(const char *, T, const char *, const char *)
    -> DcCommand2<typename function_traits<T>::f_type>;
#endif

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::DcCommand *pCmd);
rf::Player *FindBestMatchingPlayer(const char *pszName);
void DebugRender3d();
void DebugRender2d();
