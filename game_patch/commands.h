#pragma once

#include "rf.h"
#include <patch_common/Traits.h>
#include <functional>
#include <optional>

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
    if (rf::dc_arg_type & type_flag)
        return true;
    if (!(rf::dc_arg_type & rf::DC_ARG_NONE))
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
    return std::optional{rf::dc_int_arg};
}

template<>
inline std::optional<float> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_FLOAT))
        return {};
    return std::optional{rf::dc_float_arg};
}

template<>
inline std::optional<bool> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_TRUE | rf::DC_ARG_FALSE))
        return {};
    bool value = (rf::dc_arg_type & rf::DC_ARG_TRUE) == rf::DC_ARG_TRUE;
    return std::optional{value};
}

template<>
inline std::optional<std::string> DcReadArg()
{
    if (!ReadArgInternal(rf::DC_ARG_STR))
        return {};
    return std::optional{rf::dc_str_arg};
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

    static void __thiscall StaticHandler(rf::DcCommand* cmd)
    {
        // Note: this cast actually changes the offset taking into account the vtbl existance
        auto cmd2 = static_cast<BaseCommand*>(cmd);
        cmd2->Handler();
    }

    virtual void Handler() = 0;

public:
    void Register()
    {
        CommandRegister(this);
    }
};

template<typename T>
class DcCommand2 : public BaseCommand
{
private:
    template<typename V>
    class HandlerWrapper;

    template<typename ReturnType, typename... Args>
    class HandlerWrapper<ReturnType(Args...)>
    {
        T m_handler;

    public:
        HandlerWrapper(T handler) : m_handler(handler)
        {}

        void operator()()
        {
            m_handler(DcReadArg<Args>()...);
        }
    };

    HandlerWrapper<typename function_traits<T>::f_type> m_handler_wrapper;
    const char* m_usage_text;

public:
    DcCommand2(const char* name, T handler, const char* description = nullptr,
               const char* usage_text = nullptr) :
        BaseCommand(name, description),
        m_handler_wrapper(handler), m_usage_text(usage_text)
    {}

private:
    void Handler() override
    {
        if (rf::dc_run)
            Run();
        else if (rf::dc_help)
            Help();
    }

    void Run()
    {
        try {
            m_handler_wrapper();
        }
        catch (const DcInvalidArgTypeError&) {
            rf::DcPrint("Invalid arg type!", nullptr);
        }
        catch (const DcRequiredArgMissingError&) {
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

class DcCommandAlias : public BaseCommand
{
private:
    rf::DcCommand& m_target_cmd;

public:
    DcCommandAlias(const char* name, rf::DcCommand& target_cmd) : BaseCommand(name), m_target_cmd(target_cmd) {}

private:
    void Handler() override
    {
        auto handler_fun = reinterpret_cast<void(__thiscall *)(rf::DcCommand*)>(m_target_cmd.func);
        handler_fun(&m_target_cmd);
    }
};
