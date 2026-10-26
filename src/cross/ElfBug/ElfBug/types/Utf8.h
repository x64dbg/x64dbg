#pragma once

#include <cstddef>
#include <string_view>

namespace ElfBug
{
    // The longest prefix of at most maxBytes that doesn't end inside a UTF-8 character.
    inline std::string_view Utf8Prefix(const std::string_view text, const size_t maxBytes)
    {
        if(text.size() <= maxBytes)
            return text;
        constexpr size_t kMaxContinuationBytes = 3;
        size_t end = maxBytes;
        while(end > 0 && maxBytes - end < kMaxContinuationBytes && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80)
            --end;
        return text.substr(0, end);
    }
}
