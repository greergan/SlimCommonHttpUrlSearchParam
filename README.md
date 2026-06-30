<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>   

# SlimCommonHttpSearchParam

A lightweight, Node `URLSearchParams`-compatible HTTP search parameter implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript URLSearchParams object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Dependency of the [SlimCommonHttpSearchParams](https://codeberg.org/greergan/SlimCommonHttpSearchParams) micro-library.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  
CI/CD supplied by unified workflows provided by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).

## Table of Contents

- [Overview](#overview)
- [Features](#features)
  - [Node parity](#node-parity)
- [Core API](#core-api)
  - [ErrorStatus enum](#errorstatus-enum)
  - [SearchParamParseException](#searchparamparseexception)
  - [SearchParam class](#searchparam-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Setters](#setters)
  - [Getters](#getters)
  - [Serialization](#serialization)
- [Building](#building)
- [Dependencies](#dependencies)
  - [required_packages](#required_packages)
- [Examples](#examples)

## Overview

This library provides a name/value pair parser and serializer for HTTP query string search parameters, behaviorally matched to Node's `URLSearchParams`, with:
- Two construction paths: parsing an already-encoded wire pair, and building from raw application data
- Permissive parsing — no input is rejected on validation grounds, matching Node's behavior
- Percent-encoding applied on the build path; percent-decoding applied transparently on read via the getters
- `+` decodes to space, matching `application/x-www-form-urlencoded` convention
- Malformed percent-encoding is left untouched rather than rejected
- Explicit status reporting via [`ErrorStatus`](https://codeberg.org/greergan/SlimCommonHttp) reserved for genuine allocation failure
- Heavy use of `noexcept`

[↑ Top](#table-of-contents)

## Features

| Feature | Description |
|--------|-------------|
| Wire parsing | Construct directly from a `name=value` or bare `name` query pair already off the wire |
| Raw building | Construct from unencoded application-level name/value strings, encoded on store |
| No validation rejection | No character, including control characters, `=`, `&`, or DEL, causes a parse or build failure |
| Permissive percent-encoding | Malformed `%XX` sequences are left as literal text rather than rejected |
| Plus-as-space decoding | A literal `+` decodes to a space; `%2B` decodes to a literal `+` |
| Transparent decoding | Getters return percent-decoded strings regardless of construction path |
| Serialize | Produces a `name=value` string from the stored, encoded representation |
| Error model | `ErrorStatus` reserved for genuine allocation failure, not input validation |

[↑ Top](#table-of-contents)

### Node parity

`SearchParam`'s parse path is intentionally matched to Node's `URLSearchParams` behavior for a single name/value pair. The following examples were confirmed directly against Node v20:

| Input | Node `URLSearchParams` result | `SearchParam` result |
|---|---|---|
| `key=value` | `[["key", "value"]]` | name `key`, value `value` |
| `key=val+ue` | `[["key", "val ue"]]` | name `key`, value `val ue` |
| `key=val%2Bue` | `[["key", "val+ue"]]` | name `key`, value `val+ue` |
| `key=val=ue` | `[["key", "val=ue"]]` | name `key`, value `val=ue` |
| `ke%2Fy=value` | `[["ke/y", "value"]]` | name `ke/y`, value `value` |
| `a%3Db=c` | `[["a=b", "c"]]` | name `a=b`, value `c` |
| `.set('', 'value')` | `=value` (empty name allowed) | empty name allowed |

Unlike Node, `SearchParam` does not split a full query string on `&`; it represents a single pair, and callers are expected to split a multi-pair query string before constructing each `SearchParam`.

[↑ Top](#table-of-contents)

## Core API

### ErrorStatus enum

`ErrorStatus` is the scoped enum provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

For `SearchParam`, only one value is relevant in practice:

| Group | Examples | Meaning |
|-------|----------|---------|
| General | `BadAllocation` | Memory allocation failed while storing name or value |
| `OK` | — | No error; the operation succeeded |

`SearchParam` does not use any of the name/value/encoding-specific `ErrorStatus` values for rejection, since no input is rejected on validation grounds.

[↑ Top](#table-of-contents)

### SearchParamParseException

`SearchParamParseException` is thrown by both constructors, but only when storing the name or value fails due to allocation failure. It is never thrown for malformed, unexpected, or unusual input content. It carries the originating `ErrorStatus`, retrievable via `.error()`.

[↑ Top](#table-of-contents)

### SearchParam class

```cpp
slim::common::http::SearchParam p;
```

[↑ Top](#table-of-contents)

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `SearchParam()` | Default constructor, produces an empty search param |
| `SearchParam(std::string_view query_pair)` | Parse path. Construct from an already-encoded `name` or `name=value` pair off the wire. Splits on the first literal `=` before any decoding occurs. Does not split on `&`; callers are responsible for splitting a full query string into individual pairs beforehand. Throws `SearchParamParseException` only on allocation failure |
| `SearchParam(std::string_view name, std::string_view value)` | Build path. Construct from raw, unencoded name and value. Encodes on store. An empty name is permitted. Throws `SearchParamParseException` only on allocation failure |

[↑ Top](#table-of-contents)

### Setters

Used for raw, unencoded application data. Percent-encodes on store.

| Method | Description |
|--------|-------------|
| `ErrorStatus set_name(std::string_view) noexcept` | Set name from raw input. Always succeeds (returns `OK`) barring allocation failure; an empty name is permitted |
| `ErrorStatus set_value(std::string_view) noexcept` | Set value from raw input. Always succeeds (returns `OK`) barring allocation failure |

[↑ Top](#table-of-contents)

### Getters

| Method | Returns |
|--------|---------|
| `std::string get_name() const noexcept` | Percent-decoded name |
| `std::string get_value() const noexcept` | Percent-decoded value |

[↑ Top](#table-of-contents)

### Serialization

```cpp
std::string SearchParam::serialize() const;
// -> "key=value"
```

Produces a `name=value` string from the stored (already-encoded) representation. Throws `SearchParamParseException` (carrying `ErrorStatus::BadAllocation`) if string concatenation fails.

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

### required_packages

External package dependencies for this library are declared in the [`required_packages`](https://codeberg.org/greergan/SlimCommonHttpSearchParam/src/branch/master/required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

```
SlimCommonHttp
SlimCommonUtilities
```

- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)
- [SlimCommonUtilities](https://codeberg.org/greergan/SlimCommonUtilities)

[↑ Top](#table-of-contents)

## Examples

```cpp
// Parsing a query pair already off the wire
slim::common::http::SearchParam p("key%20name=val%20ue");
auto name = p.get_name();   // "key name"
auto value = p.get_value(); // "val ue"
```

```cpp
// Building from raw application data
slim::common::http::SearchParam p("key name", "val ue");
auto header = p.serialize();
// -> "key%20name=val%20ue"
```

```cpp
// Default construction with status checking
slim::common::http::SearchParam p;

ErrorStatus e = p.set_name("key");
if (e != ErrorStatus::OK) return e;

e = p.set_value("value");
if (e != ErrorStatus::OK) return e;

auto header = p.serialize();
```

```cpp
// Catching allocation failure
try {
    slim::common::http::SearchParam p("key", "value");
    auto header = p.serialize();
}
catch (const slim::common::http::SearchParamParseException& e) {
    std::cerr << "Search param allocation failure: " << e.what() << '\n';
}
```

[↑ Top](#table-of-contents)
