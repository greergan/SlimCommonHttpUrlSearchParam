#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/error_codes.h>
#include <slim/common/http/url/search/param.h>

using slim::common::http::ErrorStatus;
using slim::common::http::SearchParamParseException;
using slim::common::http::UrlSearchParam;

TEST_CASE("UrlSearchParam default construction", "[UrlSearchParam]") {
    UrlSearchParam p;
    REQUIRE(p.get_name() == "");
    REQUIRE(p.get_value() == "");
}

TEST_CASE("UrlSearchParam construction via set_name/set_value (build path)", "[UrlSearchParam]") {
    UrlSearchParam p;

    SECTION("valid name and value") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("value") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty name is rejected") {
        REQUIRE(p.set_name("") == ErrorStatus::SearchParamNameEmpty);
    }
    SECTION("whitespace-only name is allowed; encodes to non-empty on serialize") {
        REQUIRE(p.set_name("   ") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "+++=v");
    }
    SECTION("empty value is allowed") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "");
    }
    SECTION("name with literal equals is rejected") {
        REQUIRE(p.set_name("bad=name") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with literal ampersand is rejected") {
        REQUIRE(p.set_name("bad&name") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with DEL is rejected") {
        REQUIRE(p.set_name("ke\x7Fy") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with literal space is allowed; encodes to + on serialize") {
        REQUIRE(p.set_name("ke y") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke+y=v");
    }
    SECTION("name with control character is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("ke\x01y") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke%01y=v");
    }
    SECTION("name with null byte is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name(std::string("ke\x00y", 4)) == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke%00y=v");
    }
    SECTION("name with high byte is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("ke\x80y") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke%80y=v");
    }
    SECTION("name with literal plus is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("ke+y") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke%2By=v");
    }
    SECTION("value with literal ampersand is rejected") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("bad&value") == ErrorStatus::SearchParamValueInvalidChar);
    }
    SECTION("value with DEL is rejected") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("val\x7Fue") == ErrorStatus::SearchParamValueInvalidChar);
    }
    SECTION("value with literal space is allowed; encodes to + on serialize") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("val ue") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=val+ue");
    }
    SECTION("value with control character is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("va\x01ue") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=va%01ue");
    }
    SECTION("value with null byte is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value(std::string("va\x00ue", 5)) == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=va%00ue");
    }
    SECTION("value with high byte is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("va\x80ue") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=va%80ue");
    }
    SECTION("value with literal equals is allowed") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("val=ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("value with literal plus is allowed; percent-encodes on serialize") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.set_value("val+ue") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=val%2Bue");
    }
}

TEST_CASE("UrlSearchParam::set_name", "[UrlSearchParam]") {
    UrlSearchParam p;

    SECTION("valid simple name") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "key");
    }
    SECTION("empty name is rejected") {
        REQUIRE(p.set_name("") == ErrorStatus::SearchParamNameEmpty);
    }
    SECTION("name is stored as-is, not trimmed") {
        REQUIRE(p.set_name("  key  ") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "  key  ");
    }
    SECTION("name is stored raw, not percent-decoded") {
        REQUIRE(p.set_name("ke%20y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke%20y");
    }
    SECTION("percent sign in name is stored raw and double-encodes on serialize") {
        REQUIRE(p.set_name("ke%20y") == ErrorStatus::OK);
        REQUIRE(p.set_value("v") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "ke%2520y=v");
    }
}

TEST_CASE("UrlSearchParam::set_value", "[UrlSearchParam]") {
    UrlSearchParam p;
    p.set_name("key");

    SECTION("valid simple value") {
        REQUIRE(p.set_value("value") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty value is allowed") {
        REQUIRE(p.set_value("") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "");
    }
    SECTION("value is stored as-is, not trimmed") {
        REQUIRE(p.set_value("  value  ") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "  value  ");
    }
    SECTION("value is stored raw, not percent-decoded") {
        REQUIRE(p.set_value("val%20ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val%20ue");
    }
    SECTION("percent sign in value is stored raw and double-encodes on serialize") {
        REQUIRE(p.set_value("val%20ue") == ErrorStatus::OK);
        REQUIRE(p.serialize() == "key=val%2520ue");
    }
}

TEST_CASE("UrlSearchParam construction from query pair (parse path, well-formed input)", "[UrlSearchParam]") {
    SECTION("name only, no equals sign") {
        UrlSearchParam p("key");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "");
    }
    SECTION("empty name before equals is allowed per WHATWG") {
        UrlSearchParam p("=value");
        REQUIRE(p.get_name() == "");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty name serializes correctly") {
        UrlSearchParam p("=value");
        REQUIRE(p.serialize() == "=value");
    }
    SECTION("name and value") {
        UrlSearchParam p("key=value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("split happens on first literal equals; rest stays in value") {
        UrlSearchParam p("ke=y=value");
        REQUIRE(p.get_name() == "ke");
        REQUIRE(p.get_value() == "y=value");
    }
    SECTION("percent-encoded equals in name does not affect split point") {
        UrlSearchParam p("a%3Db=c");
        REQUIRE(p.get_name() == "a=b");
        REQUIRE(p.get_value() == "c");
    }
    SECTION("percent-encoded equals in value") {
        UrlSearchParam p("a=b%3Dc");
        REQUIRE(p.get_name() == "a");
        REQUIRE(p.get_value() == "b=c");
    }
    SECTION("equals sign in value is preserved") {
        UrlSearchParam p("key=val=ue");
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("plus in name decodes to space per WHATWG") {
        UrlSearchParam p("ke+y=value");
        REQUIRE(p.get_name() == "ke y");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("plus in name round-trips through serialize as +") {
        UrlSearchParam p("ke+y=value");
        REQUIRE(p.serialize() == "ke+y=value");
    }
    SECTION("plus sign in value decodes to space") {
        UrlSearchParam p("key=val+ue");
        REQUIRE(p.get_value() == "val ue");
    }
    SECTION("percent-encoded plus in value stays literal") {
        UrlSearchParam p("key=val%2Bue");
        REQUIRE(p.get_value() == "val+ue");
    }
    SECTION("uppercase hex percent-encoding decodes") {
        UrlSearchParam p("ke%2Fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("lowercase hex percent-encoding decodes") {
        UrlSearchParam p("ke%2fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("null byte percent-encoded in name decodes") {
        UrlSearchParam p("ke%00y=value");
        REQUIRE(p.get_name() == std::string("ke\x00y", 4));
    }
    SECTION("high byte percent-encoded in value decodes") {
        UrlSearchParam p("key=val%80ue");
        REQUIRE(p.get_value() == "val\x80ue");
    }
}

TEST_CASE("UrlSearchParam construction from query pair (parse path, malformed input throws)", "[UrlSearchParam]") {
    SECTION("literal ampersand in name is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke&y=value"), SearchParamParseException);
    }
    SECTION("literal ampersand in value is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("key=val&ue"), SearchParamParseException);
    }
    SECTION("literal space in name is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke y=value"), SearchParamParseException);
    }
    SECTION("control character in name is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke\x01y=value"), SearchParamParseException);
    }
    SECTION("DEL character in name is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke\x7Fy=value"), SearchParamParseException);
    }
    SECTION("DEL character in value is rejected") {
        REQUIRE_THROWS_AS(UrlSearchParam("key=val\x7Fue"), SearchParamParseException);
    }
    SECTION("truncated percent-encoding in name throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke%2y=value"), SearchParamParseException);
    }
    SECTION("percent with no hex digits in name throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke%zzy=value"), SearchParamParseException);
    }
    SECTION("bare percent at end of name throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("key%=value"), SearchParamParseException);
    }
    SECTION("truncated percent-encoding in value throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("key=val%2"), SearchParamParseException);
    }
    SECTION("percent with no hex digits in value throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("key=val%zzue"), SearchParamParseException);
    }
    SECTION("bare percent at end of value throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("key=value%"), SearchParamParseException);
    }
}

TEST_CASE("UrlSearchParam round-trip (parse then serialize)", "[UrlSearchParam]") {
    SECTION("plain pair is byte-identical") {
        UrlSearchParam p("key=value");
        REQUIRE(p.serialize() == "key=value");
    }
    SECTION("%20 decodes to space on parse, re-encodes as + on serialize") {
        UrlSearchParam p("ke%20y=val%20ue");
        REQUIRE(p.serialize() == "ke+y=val+ue");
    }
    SECTION("+ in name and value re-encodes as + on serialize") {
        UrlSearchParam p("ke+y=val+ue");
        REQUIRE(p.serialize() == "ke+y=val+ue");
    }
    SECTION("percent-encoded slash round-trips") {
        UrlSearchParam p("ke%2Fy=val%2Fue");
        REQUIRE(p.serialize() == "ke%2Fy=val%2Fue");
    }
    SECTION("empty name round-trips") {
        UrlSearchParam p("=value");
        REQUIRE(p.serialize() == "=value");
    }
    SECTION("empty value round-trips") {
        UrlSearchParam p("key=");
        REQUIRE(p.serialize() == "key=");
    }
}

// Confirmed directly against Node v20's URLSearchParams.
TEST_CASE("UrlSearchParam vs Node URLSearchParams", "[UrlSearchParam][node-parity]") {
    SECTION("plain name and value matches Node") {
        UrlSearchParam p("key=value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("plus decodes to space in value, matches Node") {
        UrlSearchParam p("key=val+ue");
        REQUIRE(p.get_value() == "val ue");
    }
    SECTION("plus decodes to space in name, matches Node") {
        UrlSearchParam p("ke+y=value");
        REQUIRE(p.get_name() == "ke y");
    }
    SECTION("empty name is allowed, matches Node") {
        UrlSearchParam p("=value");
        REQUIRE(p.get_name() == "");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("percent-encoded equals in name portion does not affect split, matches Node") {
        UrlSearchParam p("a%3Db=c");
        REQUIRE(p.get_name() == "a=b");
        REQUIRE(p.get_value() == "c");
    }
}

TEST_CASE("UrlSearchParam::serialize", "[UrlSearchParam]") {
    SECTION("name and value") {
        UrlSearchParam p;
        p.set_name("key");
        p.set_value("value");
        REQUIRE(p.serialize() == "key=value");
    }
    SECTION("empty value") {
        UrlSearchParam p;
        p.set_name("key");
        REQUIRE(p.serialize() == "key=");
    }
    SECTION("form-url-safe punctuation (* - . _) passes through unescaped") {
        UrlSearchParam p;
        p.set_name("a*b-c.d_e");
        p.set_value("v");
        REQUIRE(p.serialize() == "a*b-c.d_e=v");
    }
    SECTION("raw plus in name percent-encodes on serialize") {
        UrlSearchParam p;
        p.set_name("ke+y");
        p.set_value("val");
        REQUIRE(p.serialize() == "ke%2By=val");
    }
    SECTION("raw plus in value percent-encodes on serialize") {
        UrlSearchParam p;
        p.set_name("key");
        p.set_value("val+ue");
        REQUIRE(p.serialize() == "key=val%2Bue");
    }
    SECTION("space in name encodes to + on serialize") {
        UrlSearchParam p;
        p.set_name("ke y");
        p.set_value("v");
        REQUIRE(p.serialize() == "ke+y=v");
    }
    SECTION("space in value encodes to + on serialize") {
        UrlSearchParam p;
        p.set_name("key");
        p.set_value("val ue");
        REQUIRE(p.serialize() == "key=val+ue");
    }
    SECTION("control character in name percent-encodes on serialize") {
        UrlSearchParam p;
        p.set_name("ke\x01y");
        p.set_value("v");
        REQUIRE(p.serialize() == "ke%01y=v");
    }
    SECTION("high byte in value percent-encodes on serialize") {
        UrlSearchParam p;
        p.set_name("key");
        p.set_value("va\x80ue");
        REQUIRE(p.serialize() == "key=va%80ue");
    }
}
