#include <slim/common/http/error_codes.h>
#include <slim/common/http/search/param.h>
#include <slim/common/utilities.h>

namespace slim::common::http {

namespace {
    using slim::common::utilities::percent_decode;
    using slim::common::utilities::percent_encode;

    // Mirrors Node's URLSearchParams parsing for a single name=value pair:
    // never rejects input on validation grounds. The first '=' splits
    // name/value. Callers are responsible for splitting a full query
    // string on '&' before constructing a SearchParam; this class does
    // not expect or handle '&'.
    ErrorStatus parse_query_pair(std::string_view query_pair, std::string& name_out, std::string& value_out) noexcept {
        try {
            const auto eq = query_pair.find('=');
            if (eq == std::string_view::npos) {
                name_out = query_pair;
                value_out.clear();
            }
            else {
                name_out = query_pair.substr(0, eq);
                value_out = query_pair.substr(eq + 1);
            }
        } catch (...) {
            return ErrorStatus::BadAllocation;
        }
        return ErrorStatus::OK;
    }
} // namespace

// Parse path: construct from an already-encoded "name=value" pair off the wire.
// Matches Node's URLSearchParams: never rejects input on validation grounds.
// Only throws if storing the name/value fails (e.g. allocation failure).
SearchParam::SearchParam(std::string_view query_pair) {
    auto e = parse_query_pair(query_pair, name_, value_);
    if (e != ErrorStatus::OK) {
        throw SearchParamParseException(e);
    }
}

// Build path: construct from raw name and value, encodes on store.
// Matches Node's URLSearchParams.set(): never rejects input on validation
// grounds, including an empty name. Only throws if storing the name/value
// fails (e.g. allocation failure).
SearchParam::SearchParam(std::string_view name, std::string_view value) {
    auto e = set_name(name);
    if (e != ErrorStatus::OK) {
        throw SearchParamParseException(e);
    }
    e = set_value(value);
    if (e != ErrorStatus::OK) {
        throw SearchParamParseException(e);
    }
}

ErrorStatus SearchParam::set_name(std::string_view s) noexcept {
    try {
        percent_encode(s, name_);
    } catch (...) {
        return ErrorStatus::BadAllocation;
    }
    return ErrorStatus::OK;
}

ErrorStatus SearchParam::set_value(std::string_view s) noexcept {
    try {
        percent_encode(s, value_);
    } catch (...) {
        return ErrorStatus::BadAllocation;
    }
    return ErrorStatus::OK;
}

std::string SearchParam::get_name() const noexcept {
    try {
        std::string out;
        percent_decode(name_, out);
        return out;
    } catch (...) {
        return {};
    }
}

std::string SearchParam::get_value() const noexcept {
    try {
        std::string out;
        percent_decode(value_, out);
        return out;
    } catch (...) {
        return {};
    }
}

std::string SearchParam::serialize() const {
    try {
        return name_ + "=" + value_;
    } catch (...) {
        throw SearchParamParseException(ErrorStatus::BadAllocation);
    }
}

} // namespace slim::common::http
