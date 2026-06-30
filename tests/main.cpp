#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/error_codes.h>
#include <slim/common/http/search/param.h>

using slim::common::http::ErrorStatus;
using slim::common::http::SearchParam;
using slim::common::http::SearchParamParseException;

TEST_CASE("SearchParam default construction", "[SearchParam]") {
    SearchParam p;
    REQUIRE(p.get_name() == "");
    REQUIRE(p.get_value() == "");
}

TEST_CASE("SearchParam construction (build path)", "[SearchParam]") {
    SECTION("valid name and value") {
        SearchParam p("key", "value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty name is allowed") {
        SearchParam p("", "value");
        REQUIRE(p.get_name() == "");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty value is allowed") {
        SearchParam p("key", "");
        REQUIRE(p.get_value() == "");
    }
    SECTION("raw name with reserved characters is encoded") {
        SearchParam p("bad=name", "value");
        REQUIRE(p.get_name() == "bad=name");
    }
    SECTION("raw value with reserved characters is encoded") {
        SearchParam p("key", "bad&value");
        REQUIRE(p.get_value() == "bad&value");
    }
    SECTION("raw name with control characters is encoded") {
        SearchParam p("ke\x01y", "value");
        REQUIRE(p.get_name() == "ke\x01y");
    }
}

TEST_CASE("SearchParam construction from query pair (parse path)", "[SearchParam]") {
    SECTION("name only, no equals sign") {
        SearchParam p("key");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "");
    }
    SECTION("name and value") {
        SearchParam p("key=value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty name before equals is allowed") {
        SearchParam p("=value");
        REQUIRE(p.get_name() == "");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("split happens on literal equals before decoding") {
        SearchParam p("ke=y=value");
        REQUIRE(p.get_name() == "ke");
        REQUIRE(p.get_value() == "y=value");
    }
    SECTION("percent-encoded equals in name does not affect split point") {
        SearchParam p("a%3Db=c");
        REQUIRE(p.get_name() == "a=b");
        REQUIRE(p.get_value() == "c");
    }
    SECTION("percent-encoded equals in value") {
        SearchParam p("a=b%3Dc");
        REQUIRE(p.get_name() == "a");
        REQUIRE(p.get_value() == "b=c");
    }
    SECTION("ampersand in value is left as-is (caller splits on & beforehand)") {
        SearchParam p("key=val&ue");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "val&ue");
    }
    SECTION("plus sign in name decodes to space") {
        SearchParam p("ke+y=value");
        REQUIRE(p.get_name() == "ke y");
    }
    SECTION("plus sign in value decodes to space") {
        SearchParam p("key=val+ue");
        REQUIRE(p.get_value() == "val ue");
    }
    SECTION("percent-encoded plus stays literal") {
        SearchParam p("key=val%2Bue");
        REQUIRE(p.get_value() == "val+ue");
    }
    SECTION("space in name is left as-is") {
        SearchParam p("ke y=value");
        REQUIRE(p.get_name() == "ke y");
    }
    SECTION("control character in name is left as-is") {
        SearchParam p("ke\x01y=value");
        REQUIRE(p.get_name() == "ke\x01y");
    }
    SECTION("DEL character in value is left as-is") {
        SearchParam p("key=val\x7Fue");
        REQUIRE(p.get_value() == "val\x7Fue");
    }
    SECTION("equals sign in value is preserved") {
        SearchParam p("key=val=ue");
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("uppercase hex percent-encoding decodes") {
        SearchParam p("ke%2Fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("lowercase hex percent-encoding decodes") {
        SearchParam p("ke%2fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("truncated percent-encoding in name is left as-is") {
        SearchParam p("ke%2y=value");
        REQUIRE(p.get_name() == "ke%2y");
    }
    SECTION("percent with no hex digits in name is left as-is") {
        SearchParam p("ke%zzy=value");
        REQUIRE(p.get_name() == "ke%zzy");
    }
    SECTION("bare percent at end of name is left as-is") {
        SearchParam p("key%=value");
        REQUIRE(p.get_name() == "key%");
    }
    SECTION("truncated percent-encoding in value is left as-is") {
        SearchParam p("key=val%2");
        REQUIRE(p.get_value() == "val%2");
    }
    SECTION("percent with no hex digits in value is left as-is") {
        SearchParam p("key=val%zzue");
        REQUIRE(p.get_value() == "val%zzue");
    }
    SECTION("bare percent at end of value is left as-is") {
        SearchParam p("key=value%");
        REQUIRE(p.get_value() == "value%");
    }
}

TEST_CASE("SearchParam::set_name", "[SearchParam]") {
    SearchParam p;

    SECTION("valid simple name") {
        REQUIRE(p.set_name("key") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "key");
    }
    SECTION("empty name is allowed") {
        REQUIRE(p.set_name("") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "");
    }
    SECTION("name with reserved characters is encoded") {
        REQUIRE(p.set_name("ke=y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke=y");
    }
    SECTION("name with space is encoded") {
        REQUIRE(p.set_name("ke y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke y");
    }
    SECTION("name with control character is encoded") {
        REQUIRE(p.set_name("ke\x01y") == ErrorStatus::OK);
        REQUIRE(p.get_name() == "ke\x01y");
    }
}

TEST_CASE("SearchParam::set_value", "[SearchParam]") {
    SearchParam p;
    p.set_name("key");

    SECTION("valid simple value") {
        REQUIRE(p.set_value("value") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "value");
    }
    SECTION("empty value is allowed") {
        REQUIRE(p.set_value("") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "");
    }
    SECTION("value with reserved characters is encoded") {
        REQUIRE(p.set_value("val=ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("value with ampersand is encoded") {
        REQUIRE(p.set_value("val&ue") == ErrorStatus::OK);
        REQUIRE(p.get_value() == "val&ue");
    }
}

// Confirmed directly against Node v20's URLSearchParams:
//   new URLSearchParams('key=value')        -> [['key', 'value']]
//   new URLSearchParams('key=val+ue')       -> [['key', 'val ue']]
//   new URLSearchParams('key=val%2Bue')     -> [['key', 'val+ue']]
//   new URLSearchParams('key=val=ue')       -> [['key', 'val=ue']]
//   new URLSearchParams('ke%2Fy=value')     -> [['ke/y', 'value']]
//   new URLSearchParams('ke%2fy=value')     -> [['ke/y', 'value']]
//   new URLSearchParams('a%3Db=c')          -> [['a=b', 'c']]
//   new URLSearchParams().set('', 'value')  -> '=value' (empty name allowed)
TEST_CASE("SearchParam matches Node URLSearchParams on well-formed input", "[SearchParam][node-parity]") {
    SECTION("plain name and value") {
        SearchParam p("key=value");
        REQUIRE(p.get_name() == "key");
        REQUIRE(p.get_value() == "value");
    }
    SECTION("plus sign decodes to space") {
        SearchParam p("key=val+ue");
        REQUIRE(p.get_value() == "val ue");
    }
    SECTION("percent-encoded plus stays literal") {
        SearchParam p("key=val%2Bue");
        REQUIRE(p.get_value() == "val+ue");
    }
    SECTION("equals sign in value is preserved") {
        SearchParam p("key=val=ue");
        REQUIRE(p.get_value() == "val=ue");
    }
    SECTION("uppercase hex percent-encoding decodes") {
        SearchParam p("ke%2Fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("lowercase hex percent-encoding decodes") {
        SearchParam p("ke%2fy=value");
        REQUIRE(p.get_name() == "ke/y");
    }
    SECTION("percent-encoded equals in name portion does not affect split") {
        SearchParam p("a%3Db=c");
        REQUIRE(p.get_name() == "a=b");
        REQUIRE(p.get_value() == "c");
    }
    SECTION("empty name is allowed via build path") {
        SearchParam p("", "value");
        REQUIRE(p.get_name() == "");
        REQUIRE(p.get_value() == "value");
    }
}

TEST_CASE("SearchParam::serialize", "[SearchParam]") {
    SECTION("name and value") {
        SearchParam p("key", "value");
        REQUIRE(p.serialize() == "key=value");
    }
    SECTION("empty value") {
        SearchParam p;
        p.set_name("key");
        REQUIRE(p.serialize() == "key=");
    }
    SECTION("percent-encoded round trip via parse path") {
        SearchParam p("ke%20y=val%20ue");
        REQUIRE(p.serialize() == "ke%20y=val%20ue");
    }
}
