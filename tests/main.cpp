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
    SECTION("whitespace-only name is rejected (trimmed down to empty)") {
        REQUIRE(p.set_name("   ") == ErrorStatus::SearchParamNameEmpty);
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
    SECTION("name with literal space is rejected") {
        REQUIRE(p.set_name("ke y") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with control character is rejected") {
        REQUIRE(p.set_name("ke\x01y") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with DEL is rejected") {
        REQUIRE(p.set_name("ke\x7Fy") == ErrorStatus::SearchParamNameInvalidChar);
    }
    SECTION("name with literal plus is allowed (build path diverges from parse path)") {
        REQUIRE(p.set_name("ke+y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke+y");
    }
    SECTION("value with literal ampersand is rejected") {
        REQUIRE(p.set_value("bad&value") == ErrorStatus::SearchParamValueInvalidChar);
    }
    SECTION("value with literal space is rejected") {
        REQUIRE(p.set_value("val ue") == ErrorStatus::SearchParamValueInvalidChar);
    }
    SECTION("value with control character is rejected") {
        REQUIRE(p.set_value("va\x01ue") == ErrorStatus::SearchParamValueInvalidChar);
    }
    SECTION("value with literal equals is allowed") {
        REQUIRE(p.set_value("val=ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("value with literal plus is allowed") {
        REQUIRE(p.set_value("val+ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val+ue");
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
    SECTION("surrounding whitespace is trimmed") {
        REQUIRE(p.set_name("  key  ") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "key");
    }
    SECTION("name is stored raw, not percent-decoded") {
        REQUIRE(p.set_name("ke%20y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke%20y");
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
    SECTION("surrounding whitespace is trimmed") {
        REQUIRE(p.set_value("  value  ") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "value");
    }
    SECTION("value is stored raw, not percent-decoded") {
        REQUIRE(p.set_value("val%20ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val%20ue");
    }
}

TEST_CASE("UrlSearchParam construction from query pair (parse path, well-formed input)", "[UrlSearchParam]") {
    SECTION("name only, no equals sign") {
        UrlSearchParam p("key");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "");
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
}

TEST_CASE("UrlSearchParam construction from query pair (parse path, malformed input throws)", "[UrlSearchParam]") {
    SECTION("empty name before equals throws") {
        REQUIRE_THROWS_AS(UrlSearchParam("=value"), SearchParamParseException);
    }
    SECTION("literal plus in name is rejected (asymmetric with value)") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke+y=value"), SearchParamParseException);
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

TEST_CASE("UrlSearchParam trims whitespace around the whole pair and each half", "[UrlSearchParam]") {
    SECTION("leading/trailing whitespace around the full pair is trimmed") {
        UrlSearchParam p("  key=value  ");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("whitespace hugging the equals sign is trimmed from each half") {
        UrlSearchParam p("key = value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
}

// Confirmed directly against Node v20's URLSearchParams for the well-formed cases below.
// Several behaviors intentionally diverge from Node's URLSearchParams; those are called
// out explicitly rather than asserted as parity.
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
    SECTION("percent-encoded equals in name portion does not affect split, matches Node") {
        UrlSearchParam p("a%3Db=c");
        REQUIRE(p.get_name() == "a=b");
        REQUIRE(p.get_value() == "c");
    }
    SECTION("DIVERGENCE: Node allows an empty name ('=value'); this implementation rejects it") {
        REQUIRE_THROWS_AS(UrlSearchParam("=value"), SearchParamParseException);
    }
    SECTION("DIVERGENCE: Node decodes '+' to space in a name; this implementation rejects it") {
        REQUIRE_THROWS_AS(UrlSearchParam("ke+y=value"), SearchParamParseException);
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
    SECTION("%20 decodes to space on parse, but re-encodes as '+' on serialize (not a byte-identical round trip)") {
        UrlSearchParam p("ke%20y=val%20ue");
        REQUIRE(p.serialize() == "ke+y=val+ue");
    }
    SECTION("form-url-safe punctuation (* - . _) passes through serialize unescaped") {
        UrlSearchParam p;
        p.set_name("a*b-c.d_e");
        p.set_value("v");
        REQUIRE(p.serialize() == "a*b-c.d_e=v");
    }
    SECTION("raw plus set via build path is percent-encoded on serialize, not left literal") {
        UrlSearchParam p;
        p.set_name("ke+y");
        p.set_value("val");
        REQUIRE(p.serialize() == "ke%2By=val");
    }
}
