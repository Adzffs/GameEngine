#include "BoundedJson.h"

#include <charconv>
#include <stdexcept>
#include <utility>

namespace BoundedJson
{
    namespace
    {
        using Json = Value;
        class Reader
        {
        public:
            Reader(std::string_view bytes, std::size_t limit, std::string_view context)
                : bytes_(bytes), limit_(limit), context_(context) {}

            Json Read()
            {
                if (bytes_.size() > limit_)
                    Fail("input exceeds byte limit");
                Json result = Value(0);
                Space();
                if (cursor_ != bytes_.size())
                    Fail("trailing bytes");
                return result;
            }

        private:
            std::string_view bytes_;
            std::size_t limit_;
            std::string context_;
            std::size_t cursor_ = 0;
            std::size_t valueCount_ = 0;

            [[noreturn]] void Fail(const char *message) const
            {
                throw std::runtime_error(context_ + " at byte " +
                    std::to_string(cursor_) + ": " + message);
            }

            void Space()
            {
                while (cursor_ < bytes_.size() &&
                    (bytes_[cursor_] == ' ' || bytes_[cursor_] == '\n' ||
                     bytes_[cursor_] == '\r' || bytes_[cursor_] == '\t'))
                    ++cursor_;
            }

            bool Take(char ch)
            {
                Space();
                if (cursor_ < bytes_.size() && bytes_[cursor_] == ch)
                {
                    ++cursor_;
                    return true;
                }
                return false;
            }

            void Require(char ch)
            {
                if (!Take(ch))
                    Fail("unexpected token");
            }

            std::string String()
            {
                Require('"');
                std::string result;
                while (cursor_ < bytes_.size())
                {
                    const unsigned char ch = static_cast<unsigned char>(bytes_[cursor_++]);
                    if (ch == '"')
                        return result;
                    if (ch < 0x20 || ch >= 0x80)
                        Fail("unsupported string character (ASCII only)");
                    if (result.size() >= 16384)
                        Fail("string too long");
                    if (ch != '\\')
                    {
                        result += static_cast<char>(ch);
                        continue;
                    }
                    if (cursor_ >= bytes_.size())
                        Fail("incomplete escape");
                    const char escaped = bytes_[cursor_++];
                    switch (escaped)
                    {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    default: Fail("unsupported escape");
                    }
                }
                Fail("unterminated string");
            }

            Json Value(int depth)
            {
                if (depth > 12)
                    Fail("nesting too deep");
                // A byte limit alone permits many tiny values with large DOM
                // overhead, spread across individually legal arrays/objects.
                if (++valueCount_ > 65536)
                    Fail("too many JSON values");
                Space();
                if (cursor_ == bytes_.size())
                    Fail("unexpected end");
                if (Take('{'))
                {
                    Json value{Json::Type::Object};
                    if (Take('}')) return value;
                    do
                    {
                        if (value.object.size() >= 32)
                            Fail("too many object keys");
                        const std::string key = String();
                        Require(':');
                        Json child = Value(depth + 1);
                        if (!value.object.emplace(key, std::move(child)).second)
                            Fail("duplicate object key");
                        if (Take('}')) return value;
                        Require(',');
                    } while (true);
                }
                if (Take('['))
                {
                    Json value{Json::Type::Array};
                    if (Take(']')) return value;
                    do
                    {
                        if (value.array.size() >= 16384)
                            Fail("array too large");
                        value.array.push_back(Value(depth + 1));
                        if (Take(']')) return value;
                        Require(',');
                    } while (true);
                }
                if (bytes_[cursor_] == '"')
                {
                    Json value{Json::Type::String};
                    value.string = String();
                    return value;
                }
                if (bytes_[cursor_] == '-' || (bytes_[cursor_] >= '0' && bytes_[cursor_] <= '9'))
                {
                    const std::size_t start = cursor_;
                    if (bytes_[cursor_] == '-') ++cursor_;
                    if (cursor_ == bytes_.size()) Fail("invalid integer");
                    if (bytes_[cursor_] == '0') ++cursor_;
                    else
                    {
                        if (bytes_[cursor_] < '1' || bytes_[cursor_] > '9')
                            Fail("invalid integer");
                        while (cursor_ < bytes_.size() && bytes_[cursor_] >= '0' && bytes_[cursor_] <= '9')
                            ++cursor_;
                    }
                    int parsed = 0;
                    const auto conversion = std::from_chars(bytes_.data() + start, bytes_.data() + cursor_, parsed);
                    if (conversion.ec != std::errc{} || conversion.ptr != bytes_.data() + cursor_)
                        Fail("integer overflow");
                    Json value{Json::Type::Number};
                    value.number = parsed;
                    return value;
                }
                for (const auto keyword : {"true", "false", "null"})
                {
                    const std::string_view word(keyword);
                    if (bytes_.substr(cursor_, word.size()) == word)
                    {
                        cursor_ += word.size();
                        Json value{word == "null" ? Json::Type::Null : Json::Type::Boolean};
                        value.boolean = word == "true";
                        return value;
                    }
                }
                Fail("invalid JSON value");
            }
        };
    }

    Value Parse(std::string_view input, std::size_t byteLimit, std::string_view context)
    { return Reader(input, byteLimit, context).Read(); }
}
