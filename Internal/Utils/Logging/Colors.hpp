#pragma once

// Who the fuck defines stream functions for this stuff???
namespace colors {
    inline constexpr auto reset         = "\033[0m";
    inline constexpr auto bold          = "\033[1m";
    inline constexpr auto faint         = "\033[2m";
    inline constexpr auto italic        = "\033[3m";
    inline constexpr auto underline     = "\033[4m";
    inline constexpr auto blink         = "\033[5m";
    inline constexpr auto invisible     = "\033[6m";
    inline constexpr auto strikethrough = "\033[7m";
    // Foreground color.
    inline constexpr auto grey           = "\033[30m";
    inline constexpr auto red            = "\033[31m";
    inline constexpr auto green          = "\033[32m";
    inline constexpr auto yellow         = "\033[33m";
    inline constexpr auto blue           = "\033[34m";
    inline constexpr auto magenta        = "\033[35m";
    inline constexpr auto cyan           = "\033[36m";
    inline constexpr auto white          = "\033[37m";
    inline constexpr auto bright_grey    = "\033[90m";
    inline constexpr auto bright_red     = "\033[91m";
    inline constexpr auto bright_green   = "\033[92m";
    inline constexpr auto bright_yellow  = "\033[93m";
    inline constexpr auto bright_blue    = "\033[94m";
    inline constexpr auto bright_magenta = "\033[95m";
    inline constexpr auto bright_cyan    = "\033[96m";
    inline constexpr auto bright_white   = "\033[97m";
    // Background color
    inline constexpr auto on_grey           = "\033[40m";
    inline constexpr auto on_red            = "\033[41m";
    inline constexpr auto on_green          = "\033[42m";
    inline constexpr auto on_yellow         = "\033[43m";
    inline constexpr auto on_blue           = "\033[44m";
    inline constexpr auto on_magenta        = "\033[45m";
    inline constexpr auto on_cyan           = "\033[46m";
    inline constexpr auto on_white          = "\033[47m";
    inline constexpr auto on_bright_grey    = "\033[100m";
    inline constexpr auto on_bright_red     = "\033[101m";
    inline constexpr auto on_bright_green   = "\033[102m";
    inline constexpr auto on_bright_yellow  = "\033[103m";
    inline constexpr auto on_bright_blue    = "\033[104m";
    inline constexpr auto on_bright_magenta = "\033[105m";
    inline constexpr auto on_bright_cyan    = "\033[106m";
    inline constexpr auto on_bright_white   = "\033[107m";
};