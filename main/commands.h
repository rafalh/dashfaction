#pragma once

#include "rf.h"
#include <Traits.h>
#include <functional>

namespace rf
{
struct DcCommand;
struct Player;
} // namespace rf

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::DcCommand* cmd);
rf::Player* FindBestMatchingPlayer(const char* name);
void DebugRender3d();
void DebugRender2d();

class DcInvalidArgTypeError : public std::exception
{};

class DcRequiredArgMissingError : public std::exception
{};

inline bool ReadArgInternal(int type_flag, bool preserve_case = false)
{
    rf::DcGetArg(rf::DC_ARG_ANY, preserve_case);
    if (rf::g_DcArgType & type_flag)
        return true;
    if (!(rf::g_DcArgType & rf::DC_ARG_NONE))
        throw DcInvalidArgTypeError();
    return false;
}

template<typename T>
inline T DcReadArg()
{
    auto value_opt = DcReadArg<std::optional<T>>();
    if (!value_opt)
        throw DcRequiredArgMissingError();
    return value_opt.value();
}

template<>
inline std::optional<int> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_INT))
        return {};
    return std::optional{rf::g_DcIntArg};
}

template<>
inline std::optional<float> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_FLOAT))
        return {};
    return std::optional{rf::g_DcFloatArg};
}

template<>
inline std::optional<bool> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_TRUE | rf::DC_ARG_FALSE))
        return {};
    bool value = (rf::g_DcArgType & rf::DC_ARG_TRUE) == rf::DC_ARG_TRUE;
    return std::optional{value};
}

template<>
inline std::optional<std::string> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_STR))
        return {};
    return std::optional{rf::g_DcStrArg};
}

class BaseCommand : public rf::DcCommand
{
protected:
    BaseCommand(const char* name, const char* description = nullptr)
    {
        this->cmd_name = name;
        this->descr = description;
        this->func = reinterpret_cast<void (*)()>(StaticHandler);
    }

    static void __fastcall StaticHandler(rf::DcCommand* cmd)
    {
        BaseCommand* cmd2 = static_cast<BaseCommand*>(cmd);
        cmd2->Handler();
    }

    virtual void Handler() = 0;

public:
    void Register()
    {
        CommandRegister(this);
    }
};

template<typename... Args>
class DcCommand2;

template<typename... Args>
class DcCommand2<void(Args...)> : public BaseCommand
{
private:
    std::function<void(Args...)> m_handler_fun;
    const char* m_usage_text;

public:
    DcCommand2(const char* name, void (*handler_fun)(Args...), const char* description = nullptr,
               const char* usage_text = nullptr) :
        BaseCommand(name, description),
        m_handler_fun(handler_fun), m_usage_text(usage_text)
    {}

private:
    void Handler() override
    {
        if (rf::g_DcRun)
            Run();
        else if (rf::g_DcHelp)
            Help();
    }

    void Run()
    {
        try {
            m_handler_fun(DcReadArg<Args>()...);
        }
        catch (const DcInvalidArgTypeError& e) {
            rf::DcPrint("Invalid arg type!", nullptr);
        }
        catch (const DcRequiredArgMissingError& e) {
            rf::DcPrint("Required arg is missing!", nullptr);
        }
    }

    void Help()
    {
        if (m_usage_text) {
            rf::DcPrint(rf::strings::usage, nullptr);
            rf::DcPrintf("     %s", m_usage_text);
        }
    }
};

#ifdef __cpp_deduction_guides
// deduction guide for lambda functions
template<class T>
DcCommand2(const char*, T)->DcCommand2<typename function_traits<T>::f_type>;
template<class T>
DcCommand2(const char*, T, const char*)->DcCommand2<typename function_traits<T>::f_type>;
template<class T>
DcCommand2(const char*, T, const char*, const char*)->DcCommand2<typename function_traits<T>::f_type>;
#endif

class DcCommandAlias : public BaseCommand
{
private:
    rf::DcCommand& m_target_cmd;

public:
    DcCommandAlias(const char* name, rf::DcCommand& target_cmd) :
        BaseCommand(name), m_target_cmd(target_cmd)
    {}

private:
    void Handler() override
    {
        auto handler_fun = reinterpret_cast<void __fastcall (*)(rf::DcCommand*)>(m_target_cmd.func);
        handler_fun(&m_target_cmd);
    }
};
