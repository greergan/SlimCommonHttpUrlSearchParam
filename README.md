- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)
- [SlimCommonUtilities](https://codeberg.org/greergan/SlimCommonUtilities)

[↑ Top](#table-of-contents)

## Examples

```cpp
// Parse path: construct from a pre-encoded wire pair
try {
    slim::common::http::UrlSearchParam p("key+name=val+ue");
    auto name  = p.get_name();   // "key name"
    auto value = p.get_value();  // "val ue"
    auto wire  = p.serialize();  // "key+name=val+ue"
}
catch (const slim::common::http::SearchParamParseException& e) {
    std::cerr << "Parse failed: " << e.what() << '\n';
}
```

```cpp
// Build path: set raw application data, encode on serialize
slim::common::http::UrlSearchParam p;

ErrorStatus e = p.set_name("key name");
if (e != ErrorStatus::OK) return e;

e = p.set_value("val+ue");
if (e != ErrorStatus::OK) return e;

auto wire = p.serialize();
// -> "key+name=val%2Bue"
```

```cpp
// Build path: raw value containing = is permitted
slim::common::http::UrlSearchParam p;
p.set_name("filter");
p.set_value("a=1");
auto wire = p.serialize();
// -> "filter=a%3D1"
```

```cpp
// Round trip: percent-encoded plus in name decodes to literal +, re-encodes on serialize
try {
    slim::common::http::UrlSearchParam p("ke%2By=val+ue");
    auto name  = p.get_name();   // "ke+y"
    auto value = p.get_value();  // "val ue"
    auto wire  = p.serialize();  // "ke%2By=val+ue"
}
catch (const slim::common::http::SearchParamParseException& e) {
    std::cerr << "Parse failed: " << e.what() << '\n';
}
```

[↑ Top](#table-of-contents)
