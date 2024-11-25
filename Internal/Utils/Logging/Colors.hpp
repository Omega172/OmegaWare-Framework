#pragma once

// Who the fuck defines stream functions for this stuff???
namespace colors {
    inline constexpr auto reset         = "\e[00m";
    inline constexpr auto bold          = "\e[1m";
    inline constexpr auto faint         = "\e[2m";
    inline constexpr auto italic        = "\e[3m";
    inline constexpr auto underline     = "\e[4m";
    inline constexpr auto blink         = "\e[5m";
    inline constexpr auto invisible     = "\e[6m";
    inline constexpr auto strikethrough = "\e[7m";
    // Foreground color.
    inline constexpr auto grey           = "\e[30m";
    inline constexpr auto red            = "\e[31m";
    inline constexpr auto green          = "\e[32m";
    inline constexpr auto yellow         = "\e[33m";
    inline constexpr auto blue           = "\e[34m";
    inline constexpr auto magenta        = "\e[35m";
    inline constexpr auto cyan           = "\e[36m";
    inline constexpr auto white          = "\e[37m";
    inline constexpr auto bright_grey    = "\e[90m";
    inline constexpr auto bright_red     = "\e[91m";
    inline constexpr auto bright_green   = "\e[92m";
    inline constexpr auto bright_yellow  = "\e[93m";
    inline constexpr auto bright_blue    = "\e[94m";
    inline constexpr auto bright_magenta = "\e[95m";
    inline constexpr auto bright_cyan    = "\e[96m";
    inline constexpr auto bright_white   = "\e[97m";
    // Background color
    inline constexpr auto on_grey           = "\e[40m";
    inline constexpr auto on_red            = "\e[41m";
    inline constexpr auto on_green          = "\e[42m";
    inline constexpr auto on_yellow         = "\e[43m";
    inline constexpr auto on_blue           = "\e[44m";
    inline constexpr auto on_magenta        = "\e[45m";
    inline constexpr auto on_cyan           = "\e[46m";
    inline constexpr auto on_white          = "\e[47m";
    inline constexpr auto on_bright_grey    = "\e[100m";
    inline constexpr auto on_bright_red     = "\e[101m";
    inline constexpr auto on_bright_green   = "\e[102m";
    inline constexpr auto on_bright_yellow  = "\e[103m";
    inline constexpr auto on_bright_blue    = "\e[104m";
    inline constexpr auto on_bright_magenta = "\e[105m";
    inline constexpr auto on_bright_cyan    = "\e[106m";
    inline constexpr auto on_bright_white   = "\e[107m";
};