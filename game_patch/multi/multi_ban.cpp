#include <vector>
#include <fstream>
#include <charconv>
#include <sstream>
#include <winsock2.h>
#include <xlog/xlog.h>
#include <patch_common/FunHook.h>
#include <common/utils/string-utils.h>

#include "../rf/multi.h"
#include "multi.h"

constexpr unsigned FULL_MASK = 0xFFFFFFFF;

class IpRange
{
    unsigned ip_;
    unsigned mask_;

public:
    IpRange(unsigned ip, unsigned mask = FULL_MASK) :
        ip_{ip},
        mask_{mask}
    {}

    bool operator==(const IpRange& other)
    {
        return ip_ == other.ip_ && mask_ == other.mask_;
    }

    bool matches(unsigned ip)
    {
        return (ip & mask_) == ip_;
    }

    static IpRange parse(const std::string& s);

    std::string to_string()
    {
        std::ostringstream ss;
        unsigned zeros = 0;
        while (zeros < 32) {
            if (mask_ & (1 << zeros)) {
                break;
            }
            ++zeros;
        }
        unsigned ones = 32 - zeros;
        bool is_cidr = ones != 8 && ones != 16 && ones != 24 && ones != 32;

        for (unsigned i = 0; i < 4; ++i) {
            unsigned shift = (3 - i) * 8;
            if (is_cidr || (mask_ & (0xFF << shift))) {
                ss << ((ip_ >> shift) & 0xFF);
            } else {
                ss << '*';
            }
            if (i < 3) {
                ss << '.';
            }
        }

        if (is_cidr) {
            ss << '/' << ones;
        }

        return ss.str();
    }
};

IpRange IpRange::parse(const std::string& s)
{
    const char* cur_ptr = s.c_str();
    const char* end_ptr = s.c_str() + s.size();
    unsigned ip = 0;
    unsigned mask = 0;

    for (unsigned i = 0; i < 4; ++i) {
        if (cur_ptr >= end_ptr) {
            throw std::runtime_error("invalid ip");
        }
        if (*cur_ptr == '*' && i > 0) {
            break;
        } else {
            int n = 0;
            auto result = std::from_chars(cur_ptr, end_ptr, n);
            if (result.ec != std::errc{} || n < 0 || n > 255) {
                throw std::runtime_error("invalid ip");
            }
            cur_ptr = result.ptr;
            unsigned shift = ((3 - i) * 8);
            ip |= n << shift;
            mask |= 0xFF << shift;
            if (i < 3) {
                if (*cur_ptr != '.') {
                    throw std::runtime_error("invalid ip");
                }
                ++cur_ptr;
            } else {
                break;
            }
        }
    }

    if (cur_ptr < end_ptr && *cur_ptr == '/') {
        cur_ptr++;
        int cidr = 0;
        auto result = std::from_chars(cur_ptr, end_ptr, cidr);
        if (result.ec != std::errc{} || cidr <= 0 || cidr > 32) {
            throw std::runtime_error("bad cidr");
        }
        mask = FULL_MASK << (32 - cidr);
    }

    ip &= mask;
    return IpRange{ip, mask};
}

class Banlist
{
    std::vector<IpRange> ip_ranges_;

public:
    void load()
    {
        std::ifstream f("banlist.txt");
        std::string line;
        while (f) {
            std::getline(f, line);
            // Ignore empty lines and comments
            if (line.empty() || string_starts_with(line, "#") || string_starts_with(line, "//")) {
                continue;
            }
            add(line);
        }
    }

    void save()
    {
        std::ofstream f("banlist.txt");
        for (auto& r : ip_ranges_) {
            f << r.to_string() << '\n';
        }
    }

    bool is_banned(unsigned ip)
    {
        for (auto& r : ip_ranges_) {
            if (r.matches(ip)) {
                return true;
            }
        }
        return false;
    }

    bool add(const std::string& s)
    {
        try {
            IpRange r = IpRange::parse(s);
            ip_ranges_.push_back(r);
            return true;
        } catch (const std::exception& e) {
            xlog::error("Failed to parse banlist entry: %s", s.c_str());
            return false;
        }
    }

    void add(unsigned ip)
    {
        ip_ranges_.push_back(IpRange{ip});
    }

    std::optional<IpRange> unban_last()
    {
        if (ip_ranges_.empty()) {
            return {};
        }
        IpRange r = ip_ranges_.back();
        ip_ranges_.pop_back();
        return {r};
    }

    static Banlist& instance()
    {
        static Banlist instance;
        return instance;
    }
};

FunHook multi_ban_init_hook{
    0x0046D2C0,
    []() {
        Banlist::instance().load();
    },
};

FunHook multi_ban_shutdown_hook{
    0x0046D3B0,
    []() {}
};

FunHook multi_ban_is_banned_hook{
    0x0046D010,
    [](const rf::NetAddr& addr) {
        return Banlist::instance().is_banned(addr.ip_addr);
    },
};

FunHook multi_ban_validate_ip_hook{
    0x0046D180,
    [](const char* s) {
        try {
            IpRange::parse(s);
        } catch (const std::exception& e) {
            xlog::error("Invalid IP range: %s", s);
            return 0;
        }
        return 1;
    },
};

FunHook multi_ban_add_hook{
    0x0046D070,
    [](const char* s) {
        if (Banlist::instance().add(s)) {
            Banlist::instance().save();
        }
    },
};

FunHook multi_ban_add2_hook{
    0x0046D0F0,
    [](const rf::NetAddr& addr) {
        Banlist::instance().add(addr.ip_addr);
        Banlist::instance().save();
    },
};

std::optional<std::string> multi_ban_unban_last()
{
    auto opt = Banlist::instance().unban_last();
    if (opt) {
        Banlist::instance().save();
        return {opt.value().to_string()};
    }
    return {};
}

#ifndef NDEBUG

#define ok(expr) if (!(expr)) xlog::error("Test failed: %s", #expr)

static void test_parsing()
{
    try {
        ok(IpRange::parse("192.168.17.3") == IpRange(0xC0A81103, 0xFFFFFFFF));
        ok(IpRange::parse("192.168.17.*") == IpRange(0xC0A81100, 0xFFFFFF00));
        ok(IpRange::parse("192.168.*") == IpRange(0xC0A80000, 0xFFFF0000));
        ok(IpRange::parse("192.*") == IpRange(0xC0000000, 0xFF000000));
        ok(IpRange::parse("192.168.17.17/28") == IpRange(0xC0A81110, 0xFFFFFFF0));
        ok(IpRange::parse("192.168.17.3").to_string() == "192.168.17.3");
        ok(IpRange::parse("192.168.17.*").to_string() == "192.168.17.*");
        ok(IpRange::parse("192.168.*.*").to_string() == "192.168.*.*");
        ok(IpRange::parse("192.168.17.17/28").to_string() == "192.168.17.16/28"); // normalized
    } catch (const std::exception& e) {
        xlog::error("banlist test failed: %s", e.what());
    }
}

#endif

void multi_ban_apply_patch()
{
    multi_ban_init_hook.install();
    multi_ban_shutdown_hook.install();
    multi_ban_is_banned_hook.install();
    multi_ban_validate_ip_hook.install();
    multi_ban_add_hook.install();
    multi_ban_add2_hook.install();

#ifdef DEBUG
    test_parsing();
#endif
}
