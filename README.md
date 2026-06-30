<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>   

# SlimCommonHttpUrlSearchParam

A lightweight, standards-aligned HTTP URL search parameter implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript URLSearchParam object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Dependency of the [SlimCommonHttpUrlSearchParams](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParams) micro-library.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  
CI/CD supplied by unified workflows provided by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).

## Table of Contents

- [Overview](#overview)
- [Standards](#standards)
- [Features](#features)
- [Core API](#core-api)
  - [ErrorStatus enum](#errorstatus-enum)
  - [SearchParamParseException](#searchparamparseexception)
  - [UrlSearchParam class](#urlsearchparam-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Setters](#setters)
  - [Getters](#getters)
  - [Serialization](#serialization)
- [Building](#building)
- [Dependencies](#dependencies)
  - [required_packages](#required_packages)
- [Examples](#examples)

## Overview

This library provides a name/value pair parser and serializer for HTTP URL query string search parameters, supporting two distinct usage paths:

- **Parse path** — strict validation of pre-encoded wire input, percent-decoding into plain stored strings
- **Build path** — lenient validation of raw application data, percent-encoding deferred to `serialize()`

Name and value are stored decoded. `serialize()` percent-encodes both at output time. `+` decodes to space on the parse path, matching `application/x-www-form-urlencoded` convention.

[↑ Top](#table-of-contents)

## Standards

| Standard | Relevance |
|---|---|
| [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986) §2 | Percent-encoding and decoding rules for URI components |
| [RFC 3986](https://www.rfc-editor.org/rfc/rfc3986) §3.4 | Query component character rules |
| [WHATWG URL Standard](https://url.spec.whatwg.org/#concept-urlencoded) | `application/x-www-form-urlencoded` serialization — `+` as space, percent-encoding of reserved characters |
| [HTML5](https://html.spec.whatwg.org/multipage/form-elements.html#url-search-params) | `URLSearchParams` interface — name/value pair semantics, `+` decoding convention |

[↑ Top](#table-of-contents)

## Features

| Feature | Description |
|--------|-------------|
| Dual usage paths | Strict parse path for wire input; lenient build path for raw application data |
| Wire parsing | Construct from a pre-encoded `name=value` or bare `name` pair off the wire. Validates RFC character rules and percent-encoding format; throws on invalid input |
| Raw building | Set raw, unencoded name and value via `set_name`/`set_value`. Validates structural delimiters only; `percent_encode` applied at `serialize()` time |
| Decoded storage | Name and value stored decoded in both paths; no encoding in the backing store |
| `+` as space | A literal `+` in wire input decodes to a space, matching `application/x-www-form-urlencoded` convention; `%2B` decodes to a literal `+` |
| Deferred encoding | `serialize()` percent-encodes name and value at output time, pre-reserving buffer capacity to avoid reallocations |
| Strict wire validation | Parse path rejects `=`, `&`, `+`, control characters, DEL, and malformed `%XX` sequences in name; rejects `&`, control characters, and DEL in value |
| Lenient build validation | Build path rejects only `=` and `&` in name, and `&` in value; `+` and all printable characters including `=` in value are permitted |
| Error model | Strong enum-based status reporting via `ErrorStatus` (from [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)) |

[↑ Top](#table-of-contents)

## Core API

### ErrorStatus enum

`ErrorStatus` is the scoped enum provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

Values relevant to `UrlSearchParam`:

| Group | Value | Meaning |
|-------|-------|---------|
| Name | `SearchParamNameEmpty` | Name is empty |
| Name | `SearchParamNameInvalidChar` | Name contains a character not permitted in this path |
| Value | `SearchParamValueInvalidChar` | Value contains a character not permitted in this path |
| Encoding | `SearchParamInvalidPercentEncoding` | A `%XX` sequence is truncated or contains non-hex digits (parse path only) |
| General | `BadAllocation` | Memory allocation failed |
| `OK` | — | No error; the operation succeeded |

[↑ Top](#table-of-contents)

### SearchParamParseException

`SearchParamParseException` is thrown by the wire-path constructor when input fails validation or when an allocation fails. It carries the originating `ErrorStatus`, retrievable via `.error()`.

[↑ Top](#table-of-contents)

### UrlSearchParam class

```cpp
slim::common::http::UrlSearchParam p;
```

[↑ Top](#table-of-contents)

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `UrlSearchParam()` | Default constructor, produces an empty search param |
| `UrlSearchParam(std::string_view s)` | Parse path. Construct from a pre-encoded `name` or `name=value` pair off the wire. Splits on the first literal `=` before decoding. Validates RFC character rules and percent-encoding format. Percent-decodes into `name_` and `value_`. Throws `SearchParamParseException` on validation failure or allocation failure |

[↑ Top](#table-of-contents)

### Setters

Build path — for raw, unencoded application data. Stores as-is; encoding is deferred to `serialize()`.

| Method | Rejects | Allows |
|--------|---------|--------|
| `ErrorStatus set_name(std::string_view) noexcept` | Empty input, `=`, `&`, control characters, DEL | All printable characters including `+` |
| `ErrorStatus set_value(std::string_view) noexcept` | `&`, control characters, DEL | All printable characters including `+` and `=` |

Both return `SearchParamNameEmpty`, `SearchParamNameInvalidChar`, `SearchParamValueInvalidChar`, or `BadAllocation` as appropriate, and `OK` on success.

[↑ Top](#table-of-contents)

### Getters

Name and value are stored decoded. Getters return the stored strings directly with no additional processing.

| Method | Returns |
|--------|---------|
| `std::string get_name() const noexcept` | Decoded name |
| `std::string get_value() const noexcept` | Decoded value |

[↑ Top](#table-of-contents)

### Serialization

```cpp
std::string UrlSearchParam::serialize() const;
// -> "key%20name=val%20ue"
```

Percent-encodes `name_` and `value_` at output time and returns `name=value`. Pre-reserves buffer capacity based on worst-case encoding expansion to avoid reallocations. Throws `SearchParamParseException` (carrying `ErrorStatus::BadAllocation`) if allocation fails.

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

### required_packages

External package dependencies for this library are declared in the [`required_packages`](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParam/src/branch/master/required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

```
SlimCommonHttp
SlimCommonUtilities
```

- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)
- [SlimCommonUtilities](https://codeberg.org/greergan/SlimCommonUtilities)

[↑ Top](#table-of-contents)

## Examples

```cpp
// Parse path: construct from a pre-encoded wire pair
try {
    slim::common::http::UrlSearchParam p("key%20name=val%20ue");
    auto name  = p.get_name();   // "key name"
    auto value = p.get_value();  // "val ue"
    auto wire  = p.serialize();  // "key%20name=val%20ue"
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
// -> "key%20name=val%2Bue"
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
// Round trip: parse wire input, inspect decoded values
try {
    slim::common::http::UrlSearchParam p("ke%2By=val+ue");
    // + in name is rejected on wire path; % is valid encoding
    // + in value decodes to space per application/x-www-form-urlencoded
    auto name  = p.get_name();   // "ke+y"
    auto value = p.get_value();  // "val ue"
}
catch (const slim::common::http::SearchParamParseException& e) {
    std::cerr << "Parse failed: " << e.what() << '\n';
}
```

[↑ Top](#table-of-contents)
