#pragma once
#pragma once

#include <patch_common/Traits.h>
#include <functional>
#include <optional>
#include "../rf/os/console.h"
#include "../rf/localize.h"

// Forward declarations
namespace rf
{
    struct Player;
}

void console_init();
void console_apply_patches();
void console_register_command(rf::console::Command* cmd);
rf::Player* find_best_matching_player(const char* name);

class DcInvalidArgTypeError : public std::exception
{};

class DcRequiredArgMissingError : public std::exception
{};

inline bool console_read_arg_internal(unsigned type_flag, bool preserve_case = false)
{
    rf::console::get_arg(rf::console::ARG_ANY, preserve_case);
    if (rf::console::arg_type & type_flag)
        return true;
    if (!(rf::console::arg_type & rf::console::ARG_NONE))
        throw DcInvalidArgTypeError();
    return false;
}

template<typename T>
inline T console_read_arg()
{
    auto value_opt = console_read_arg<std::optional<T>>();
    if (!value_opt)
        throw DcRequiredArgMissingError();
    return value_opt.value();
}

template<>
inline std::optional<int> console_read_arg()
{
    if (!console_read_arg_internal(rf::console::ARG_INT))
        return {};
    return std::optional{rf::console::int_arg};
}

template<>
inline std::optional<float> console_read_arg()
{
    if (!console_read_arg_internal(rf::console::ARG_FLOAT))
        return {};
    return std::optional{rf::console::float_arg};
}

template<>
inline std::optional<bool> console_read_arg()
{
    if (!console_read_arg_internal(rf::console::ARG_TRUE | rf::console::ARG_FALSE))
        return {};
    bool value = (rf::console::arg_type & rf::console::ARG_TRUE) == rf::console::ARG_TRUE;
    return std::optional{value};
}

template<>
inline std::optional<std::string> console_read_arg()
{
    if (!console_read_arg_internal(rf::console::ARG_STR))
        return {};
    return std::optional{rf::console::str_arg};
}

class BaseCommand : public rf::console::Command
{
protected:
    BaseCommand(const char* name, const char* description = nullptr)
    {
        this->name = name;
        this->help = description;
        this->func = reinterpret_cast<void (*)()>(static_handler);
    }

    static void __thiscall static_handler(rf::console::Command* cmd)
    {
        // Note: this cast actually changes the offset taking into account the vtbl existance
        auto* cmd2 = static_cast<BaseCommand*>(cmd);
        cmd2->handler();
    }

    virtual void handler() = 0;

public:
    void register_cmd()
    {
        console_register_command(this);
    }
};

template<typename T>
class ConsoleCommand2 : public BaseCommand
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
            // Note: The order of evaluation of arguments is unspecified in C++ so we have to use a tuple here
            std::tuple<Args...> args = {console_read_arg<Args>()...};
            std::apply(m_handler, args);
        }
    };

    HandlerWrapper<typename function_traits<T>::f_type> m_handler_wrapper;
    const char* m_usage_text;

public:
    ConsoleCommand2(const char* name, T handler, const char* description = nullptr,
               const char* usage_text = nullptr) :
        BaseCommand(name, description),
        m_handler_wrapper(handler), m_usage_text(usage_text)
    {}

private:
    void handler() override
    {
        if (rf::console::run)
            run();
        else if (rf::console::help)
            help();
    }

    void run()
    {
        try {
            m_handler_wrapper();
        }
        catch (const DcInvalidArgTypeError&) {
            rf::console::output("Invalid arg type!", nullptr);
        }
        catch (const DcRequiredArgMissingError&) {
            rf::console::output("Required arg is missing!", nullptr);
        }
    }

    void help()
    {
        if (m_usage_text) {
            rf::console::output(rf::strings::usage, nullptr);
            rf::console::printf("     %s", m_usage_text);
        }
    }
};

class DcCommandAlias : public BaseCommand
{
private:
    rf::console::Command& m_target_cmd;

public:
    DcCommandAlias(const char* name, rf::console::Command& target_cmd) : BaseCommand(name), m_target_cmd(target_cmd) {}

private:
    void handler() override
    {
        auto handler_fun = reinterpret_cast<void(__thiscall *)(rf::console::Command*)>(m_target_cmd.func);
        handler_fun(&m_target_cmd);
    }
};

constexpr int CMD_LIMIT = 256;

extern rf::console::Command* g_commands_buffer[CMD_LIMIT];
