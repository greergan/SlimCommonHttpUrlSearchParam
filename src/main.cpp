#include <array>
#include <slim/common/http/error_codes.h>
#include <slim/common/http/url/search/param.h>
#include <slim/common/utilities.h>

namespace slim::common::http {

namespace {
    using slim::common::utilities::is_valid_percent_encoding;
    using slim::common::utilities::percent_decode;
    using slim::common::utilities::percent_encode;
    using slim::common::utilities::trim;

    struct AsciiTables {
        std::array<bool, 256> is_invalid_name_char{};
        std::array<bool, 256> is_invalid_value_char{};
        std::array<bool, 256> is_invalid_name_char_raw{};
        std::array<bool, 256> is_invalid_value_char_raw{};
        constexpr AsciiTables() noexcept {
            for (std::size_t i = 0; i < 256; ++i) {
                // parse path — pre-encoded wire input
                // invalid in name: = & + DEL and anything below 0x21
                is_invalid_name_char[i]      = (i == '=' || i == '&' || i == '+' || i == 0x7F || i < 0x21);
                // invalid in value: & DEL and anything below 0x21 except +
                is_invalid_value_char[i]     = (i == '&' || i == 0x7F || (i < 0x21 && i != '+'));
                // build path — raw application data, percent_encode handles encoding on serialize
                // invalid in name: = & (structural delimiters) DEL and control chars
                is_invalid_name_char_raw[i]  = (i == '=' || i == '&' || i == 0x7F || i < 0x21);
                // invalid in value: & (structural delimiter) DEL and control chars
                is_invalid_value_char_raw[i] = (i == '&' || i == 0x7F || i < 0x21);
            }
        }
    };
    constexpr AsciiTables ascii{};

    // Parse path: validates pre-encoded wire input, percent-decodes into out
    ErrorStatus parse_name(std::string_view s, std::string& out) noexcept {
        trim(s);
        if (s.empty()) {
            return ErrorStatus::SearchParamNameEmpty;
        }
        for (std::size_t i = 0; i < s.size(); ++i) {
            const auto c = static_cast<unsigned char>(s[i]);
            if (c == '%') {
                if (!is_valid_percent_encoding(s.data() + i, s.size() - i)) {
                    return ErrorStatus::SearchParamInvalidPercentEncoding;
                }
                i += 2;
                continue;
            }
            if (ascii.is_invalid_name_char[c]) {
                return ErrorStatus::SearchParamNameInvalidChar;
            }
        }
        percent_decode(s, out);
        return ErrorStatus::OK;
    }

    ErrorStatus parse_value(std::string_view s, std::string& out) noexcept {
        trim(s);
        for (std::size_t i = 0; i < s.size(); ++i) {
            const auto c = static_cast<unsigned char>(s[i]);
            if (c == '%') {
                if (!is_valid_percent_encoding(s.data() + i, s.size() - i)) {
                    return ErrorStatus::SearchParamInvalidPercentEncoding;
                }
                i += 2;
                continue;
            }
            if (ascii.is_invalid_value_char[c]) {
                return ErrorStatus::SearchParamValueInvalidChar;
            }
        }
        percent_decode(s, out);
        return ErrorStatus::OK;
    }
} // namespace

// Parse path: construct from an already-encoded "name=value" pair off the wire.
// Validates and percent-decodes into name_ and value_.
UrlSearchParam::UrlSearchParam(std::string_view s) {
    trim(s);
    const auto eq = s.find('=');
    if (eq == std::string_view::npos) {
        auto e = parse_name(s, name_);
        if (e != ErrorStatus::OK) {
            throw SearchParamParseException(e);
        }
    }
    else {
        auto e = parse_name(s.substr(0, eq), name_);
        if (e != ErrorStatus::OK) {
            throw SearchParamParseException(e);
        }
        e = parse_value(s.substr(eq + 1), value_);
        if (e != ErrorStatus::OK) {
            throw SearchParamParseException(e);
        }
    }
}

// Build path: validate and store raw unencoded name as-is.
// Rejects structural delimiters (= &) and control chars; + is allowed as literal application data.
ErrorStatus UrlSearchParam::set_name(std::string_view s) noexcept {
    trim(s);
    if (s.empty()) {
        return ErrorStatus::SearchParamNameEmpty;
    }
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (ascii.is_invalid_name_char_raw[c]) {
            return ErrorStatus::SearchParamNameInvalidChar;
        }
    }
    try {
        name_ = s;
    } catch (...) {
        return ErrorStatus::BadAllocation;
    }
    return ErrorStatus::OK;
}

// Build path: validate and store raw unencoded value as-is.
// Rejects structural delimiter (&) and control chars; + and = are allowed as literal application data.
ErrorStatus UrlSearchParam::set_value(std::string_view s) noexcept {
    trim(s);
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (ascii.is_invalid_value_char_raw[c]) {
            return ErrorStatus::SearchParamValueInvalidChar;
        }
    }
    try {
        value_ = s;
    } catch (...) {
        return ErrorStatus::BadAllocation;
    }
    return ErrorStatus::OK;
}

// Encode name and value at serialization time
std::string UrlSearchParam::serialize() const {
    try {
        std::string name_enc;
        std::string value_enc;
        name_enc.reserve(name_.size() * 3);
        value_enc.reserve(value_.size() * 3);
        percent_encode(name_, name_enc);
        percent_encode(value_, value_enc);
        std::string out;
        out.reserve(name_enc.size() + value_enc.size() + 1);
        out  = std::move(name_enc);
        out += '=';
        out += value_enc;
        return out;
    } catch (...) {
        throw SearchParamParseException(ErrorStatus::BadAllocation);
    }
}

} // namespace slim::common::http
