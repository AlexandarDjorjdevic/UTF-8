#include <stddef.h>
#include <UTF8/utf8.hpp>

namespace{

    const std::vector< UTF8::UnicodeCodePoint > REPLACEMENT_CHARACTER = {0xef, 0xbf, 0xbd};
    const size_t UTF8_ENCODED_CHAR_MAX_LEN = 4;
    /*
        Consts
    */
    const UTF8::UnicodeCodePoint LAST_LEGAL_UNICODE_CODEPOINT = 0x10FFFF;
    const UTF8::UnicodeCodePoint FIRST_SUROGATE = 0xD800, LAST_SUROGATE = 0xDFFF;

    template<typename T> 
    size_t log2n(T number){
        size_t answer = 0;
        while(number > 0){
            answer++;
            number >>= 1;
        }
        return answer;
    }
}
namespace UTF8{

    std::vector< UnicodeCodePoint > AsciiToUnicode(const std::string& ascii){
        return std::vector< UnicodeCodePoint > (ascii.begin(), ascii.end());
    }

    struct UTF8::Impl {
        UnicodeCodePoint unicodeCodePointValue;
        size_t utf8RepresnetationLen;
        size_t encodingPosition;
    };

    UTF8::UTF8()
        : pimpl(new Impl)
    {
    }

    UTF8::~UTF8() = default;

    std::vector< uint8_t> UTF8::Encode(const std::vector< UnicodeCodePoint >& codePoints){
        std::vector< uint8_t > encode;
        encode.reserve(4);
        for ( auto codePoint : codePoints){
            const auto bitsForCodePoint = log2n(codePoint);
            if(bitsForCodePoint <= 7){
                encode.emplace_back(UnicodeCodePoint(codePoint & 0x7f));
            } else if (bitsForCodePoint <= 11){ 
                //chars between 0x80 - 0x7ff
                //1101 1111 | 1011 1111 
                encode.emplace_back(UnicodeCodePoint(((codePoint >> 6) & 0x1f) | 0xC0));
                encode.emplace_back(UnicodeCodePoint((codePoint & 0x3f) | 0x80));
            } else if (bitsForCodePoint <= 16){
                //chars between 0x800 - 0xffff
                //1110 1111 | 1011 1111 | 1011 1111
                if(
                    (codePoint >= FIRST_SUROGATE) && 
                    (codePoint <= LAST_SUROGATE)
                )
                {
                    encode.insert(encode.end(), REPLACEMENT_CHARACTER.begin(), REPLACEMENT_CHARACTER.end());
                } else {
                    encode.emplace_back(UnicodeCodePoint(((codePoint >> 12) & 0x0f) | 0xE0));
                    encode.emplace_back(UnicodeCodePoint(((codePoint >> 6) & 0x3f) | 0x80));
                    encode.emplace_back(UnicodeCodePoint((codePoint & 0x3f) | 0x80));
                }
            } else if (bitsForCodePoint <= 21 && codePoint <= LAST_LEGAL_UNICODE_CODEPOINT){
                //1111 0111 | 1011 1111 | 1011 1111 | 1011 1111
                encode.emplace_back(UnicodeCodePoint(((codePoint >> 18) & 0x7) | 0xF0));
                encode.emplace_back(UnicodeCodePoint(((codePoint >> 12) & 0x3f) | 0x80));
                encode.emplace_back(UnicodeCodePoint(((codePoint >> 6) & 0x3f) | 0x80));
                encode.emplace_back(UnicodeCodePoint((codePoint & 0x3f) | 0x80));
            } else {
                //The standard also recommends replacing each error with the replacement character "�" (U+FFFD)
                encode.insert(encode.end(), REPLACEMENT_CHARACTER.begin(), REPLACEMENT_CHARACTER.end());
            }
        }
        return encode;
    }

    std::vector< UnicodeCodePoint > UTF8::Decode(const std::vector< uint8_t >& encoding){
        std::vector< UnicodeCodePoint > output;
        pimpl->unicodeCodePointValue = 0;
        pimpl->encodingPosition = 0;
        for ( auto chunk : encoding){ 
            if(!(chunk & 0x80)){
                output.push_back(UnicodeCodePoint(chunk));
            }
            else
            {
                if(pimpl->encodingPosition == 0){
                    pimpl->utf8RepresnetationLen = 0;
                    while((0x80 & chunk) && (pimpl->utf8RepresnetationLen < UTF8_ENCODED_CHAR_MAX_LEN)){
                        pimpl->utf8RepresnetationLen++;
                        chunk <<= 1;
                    }
                    pimpl->unicodeCodePointValue = chunk >> pimpl->utf8RepresnetationLen;
                    pimpl->encodingPosition++;
                }
                else
                {
                    pimpl->unicodeCodePointValue <<= 6;
                    pimpl->unicodeCodePointValue += (chunk & 0x3f);
                    if(pimpl->encodingPosition == (pimpl->utf8RepresnetationLen - 1))
                    {
                        output.push_back(pimpl->unicodeCodePointValue);
                        pimpl->unicodeCodePointValue = 0;
                        pimpl->encodingPosition = 0;
                    }
                    else pimpl->encodingPosition++;
                }
            }
        }     
        return output;
    }
}
