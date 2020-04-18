#pragma once

// Custom exception codes
namespace custom_exceptions
{
    constexpr DWORD abort             = 0xE0000001;
    constexpr DWORD invalid_parameter = 0xE0000002;
    constexpr DWORD unresponsive      = 0xE0000003;
}
