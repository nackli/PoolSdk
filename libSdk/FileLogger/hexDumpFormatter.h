// hexDumpFormatter.h
#pragma once
#include "fmt/format.h"
#include <cctype>
#include <algorithm>

struct HexDump {
    const uint8_t* data;
    size_t len;

    HexDump(const void* ptr, size_t length)
        : data(static_cast<const uint8_t*>(ptr)), len(length) {}
};

namespace fmt {
    template <>
    struct formatter<HexDump> {
        int bytes_per_line = 16;

        constexpr auto parse(format_parse_context& ctx) {
            auto it = ctx.begin();
            if (it != ctx.end() && *it == '>') {
                ++it;
                int value = 0;
                while (it != ctx.end() && *it >= '0' && *it <= '9') {
                    value = value * 10 + (*it - '0');
                    ++it;
                }
                if (value > 0) bytes_per_line = value;
            }
            return it;
        }

        template <typename FormatContext>
        auto format(const HexDump& dump, FormatContext& ctx) const {  // 注意这里是 const
            auto out = ctx.out();
            out = fmt::format_to(out, "Mem len: {:08x}  \n", dump.len);
            for (size_t i = 0; i < dump.len; i += bytes_per_line) {
                size_t line_len = std::min((size_t)bytes_per_line, dump.len - i);

                out = fmt::format_to(out, "{:08x}  ", i);
                for (size_t j = 0; j < (size_t)bytes_per_line; ++j) {
                    if (j < line_len) {
                        out = fmt::format_to(out, "{:02x} ", dump.data[i + j]);
                    } else {
                        out = fmt::format_to(out, "   ");
                    }
                }
                out = fmt::format_to(out, " |");
                for (size_t j = 0; j < line_len; ++j) {
                    unsigned char c = dump.data[i + j];
                    out = fmt::format_to(out, "{}",
                        std::isprint(c) ? static_cast<char>(c) : '.');
                }
                out = fmt::format_to(out, "|\n");
            }

            return out;
        }
    };
}