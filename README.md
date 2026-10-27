<!--
  ======================================================================
  vhlibfsys — README
  Revision : 0.1.1
  Author   : Viktor Glebov / V01G04A81
  License  : MIT
  ======================================================================
-->

# vhlibfsys

**A small C++17 filesystem helper library with 64-bit binary file I/O.**

[![Language](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](#requirements-and-portability)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](#requirements-and-portability)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](#license)

| Property | Value |
|---|---|
| **Library** | `vhlibfsys` |
| **Revision** | `0.1.1` |
| **Language** | C++17 |
| **Author** | Viktor Glebov / V01G04A81 |
| **License** | MIT |
| **Repository** | <https://github.com/vigatron/vhlibfsys> |
| **Dependency** | None — C++17 standard library only |

---

## Table of contents

- [Introduction](#introduction)
- [Features](#features)
- [Design principles](#design-principles)
- [Requirements and portability](#requirements-and-portability)
- [Project structure](#project-structure)
- [Integration](#integration)
- [Quick start](#quick-start)
- [API reference: `VHFile`](#api-reference-vhfile)
  - [Class overview](#class-overview)
  - [Lifetime and ownership](#lifetime-and-ownership)
  - [Open modes](#open-modes)
  - [Function reference](#function-reference)
  - [Large-file behaviour](#large-file-behaviour)
  - [Thread safety](#thread-safety)
- [API reference: `VHFSYS`](#api-reference-vhfsys)
  - [Class overview](#vhfsys-class-overview)
  - [Function reference](#vhfsys-function-reference)
  - [Return codes](#return-codes)
- [Usage examples](#usage-examples)
- [Error handling summary](#error-handling-summary)
- [Known limitations and roadmap](#known-limitations-and-roadmap)
- [Version history](#version-history)
- [License](#license)

---

## Introduction

`vhlibfsys` is a compact, dependency-free C++17 library for two common tasks:

1. **Reading and writing binary files** with explicit 64-bit offsets and sizes.
2. **Small filesystem queries and helpers** built on top of `std::filesystem`.

The library contains two classes:

- **`VHFile`** — a move-only RAII wrapper around a C `FILE *` handle. It always opens files in binary mode, supports files larger than 2 GiB, reads and writes exact byte counts, and does not change the stream position when querying the file size.
- **`VHFSYS`** — a stateless collection of static helpers for checking files and directories, creating nested directories, reading a whole file into a `std::vector<uint8_t>`, writing a vector to a file, and extracting the filename component from a path.

Both classes are declared in the global namespace. Include `vhfile.hpp` for `VHFile` and `vhfsys.hpp` for `VHFSYS`.

`vhlibfsys` was written for tools that parse large binary formats such as GGUF model files. The design therefore favours predictable error handling, binary-safe I/O, and 64-bit arithmetic over convenience wrappers.

---

## Features

- **64-bit offsets and sizes** — `std::uint64_t` is used for file positions and sizes. Files larger than 2 GiB are supported on Windows and on 32-bit POSIX builds.
- **Binary-only I/O** — `VHFile` always uses `"rb"` / `"wb"`; there is no text-mode translation.
- **RAII ownership** — the destructor closes the handle. Copying is disabled; moving transfers ownership.
- **Exact reads and writes** — `read()` and `write()` loop until the requested byte count is transferred or an error occurs.
- **Exact file size without moving the position** — `filesize()` uses `fstat` / `_fstat64`, not `seek` to the end.
- **Portable 64-bit seek** — `fseeko` / `ftello` on POSIX, `_fseeki64` / `_ftelli64` on Windows.
- **Predictable boolean contract** — every fallible `VHFile` operation returns `true` on success and `false` on failure.
- **Filesystem helpers** — existence checks, recursive directory creation, whole-file vector I/O, and filename extraction in `VHFSYS`.
- **No global state** — the library is safe to use from independent objects; no configuration or singleton is required.
- **Small footprint** — only the C++17 standard library is needed. Since 0.1.1 the library has no external dependencies; `VHFSYS` defines its own `VHFSYS::verr` type alias.

---

## Design principles

### Binary first

All `VHFile` opens use binary mode:

```cpp
std::fopen(fname.c_str(), "rb"); // read
std::fopen(fname.c_str(), "wb"); // write, truncate
```

This avoids the CRLF translation and Ctrl-Z handling that text mode performs on Windows.

### 64-bit everywhere

Offsets, sizes, and positions are `std::uint64_t`. On POSIX the translation unit defines `_FILE_OFFSET_BITS=64` before including system headers, and the CMake target exports the same definition. On Windows the library calls `_fseeki64`, `_ftelli64`, `_fstat64`, and `_fileno`.

### Explicit success values

`VHFile` returns `bool`: `true` means success, `false` means failure. It never returns a raw `fseek` result, because `fseek` returns `0` on success and non-zero on failure — a classic source of inverted logic.

`VHFSYS` predicates return `bool`; operations that can fail return `VHFSYS::verr` (`std::uint32_t`), where `0` means success.

### Open once, use many times

`VHFile` opens the file on demand and keeps it open:

- the first `read`, `seek`, `pos`, or `filesize` on a closed object opens the file read-only;
- the first `write` on a closed object creates or truncates the file for writing.

Once open, the handle stays open until `closefile()`, `assign()`, move assignment, or destruction. This avoids reopening the file for every small read.

### Move-only ownership

A raw `FILE *` must have exactly one owner. `VHFile` therefore deletes the copy constructor and copy assignment operator and provides move operations. A moved-from object is left closed and safe to destroy.

---

## Requirements and portability

| Requirement | Details |
|---|---|
| **C++ standard** | C++17 or newer |
| **Build system** | CMake 3.16+ (or compile `vhfile.cpp` / `vhfsys.cpp` directly) |
| **Compiler** | GCC 9+, Clang 10+, MSVC 2019+ (recommended) |
| **POSIX** | `fseeko`, `ftello`, `fstat`, `fileno`; `_FILE_OFFSET_BITS=64` is defined by CMake |
| **Windows** | `_fseeki64`, `_ftelli64`, `_fstat64`, `_fileno`; binary mode |
| **Dependency** | None — only the C++17 standard library |

### Portability notes

- On POSIX, the library requires large-file support. The CMake target adds `_FILE_OFFSET_BITS=64` publicly; the source file also defines it as a fallback when compiled outside CMake.
- On Windows, the library uses the 64-bit CRT functions directly, so `long` being 32-bit does not limit file offsets.
- `VHFile` accepts a narrow `std::string` filename and passes it to `fopen`. On Windows, non-ASCII paths depend on the active narrow-character code page. If Unicode paths are required, convert them to the active code page before constructing `VHFile`, or extend the class with a wide-character overload.
- `VHFSYS` uses `std::filesystem`, which may require linking `stdc++fs` on older GCC versions (GCC 8 and earlier). Modern toolchains do not need an extra library.
- Since 0.1.1 both headers are self-contained: `vhfile.hpp` and `vhfsys.hpp` include only standard headers and no `vhlibplatform` header. `_FILE_OFFSET_BITS=64` is defined both in `vhfile.cpp` and, on POSIX, by the CMake target.

---

## Project structure

```text
vhlibfsys/
├── CMakeLists.txt          # Build configuration
├── version.h.in            # CMake-generated version header template
├── README.md               # This document
├── docs/
│   ├── header.txt          # Source header template
│   └── footer.txt          # Source footer template
└── src/
    ├── vhfile.hpp          # VHFile class declaration
    ├── vhfile.cpp          # VHFile implementation
    ├── vhfsys.hpp          # VHFSYS class declaration
    └── vhfsys.cpp          # VHFSYS implementation
```

Generated header:

```text
<build-dir>/generated/version_vhlibfsys.h
```

When `vhlibfsys` is added to a parent project with `add_subdirectory`, the generated header is placed under the corresponding build subdirectory, for example:

```text
<build-dir>/src/extmods/vhlibfsys/generated/version_vhlibfsys.h
```

It defines `VHLIB_FSYS_VERSION_MAJOR`, `VHLIB_FSYS_VERSION_MINOR`, `VHLIB_FSYS_VERSION_PATCH`, and `VHLIB_FSYS_VERSION_HEX`.

---

## Integration

### CMake (recommended)

`vhlibfsys` has no external CMake targets. Add the subdirectory and link the `vhlib_fsys` target:

```cmake
add_subdirectory(src/extmods/vhlibfsys)

target_link_libraries(your_target PRIVATE vhlib_fsys)
```

`vhlib_fsys` is a `STATIC` library. Its include directories (`src` and the generated version-header directory) are public, and on non-Windows builds it publicly defines `_FILE_OFFSET_BITS=64`.

The parent project only needs:

```cmake
cmake_minimum_required(VERSION 3.16)
project(your_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/extmods/vhlibfsys)
```

No `vhlibplatform` target, header, or source is required.

### Standalone build

The library can be built on its own without any additional directories or submodules:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

This produces `build/libvhlib_fsys.a` (or `vhlib_fsys.lib` on Windows) and `build/generated/version_vhlibfsys.h`.

### Building as part of a parent project

Build the parent target that links `vhlib_fsys`:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Manual integration

Add `vhlibfsys/src` to the include path and compile `vhfile.cpp` and `vhfsys.cpp`. Both source files depend only on the C++17 standard library. On POSIX, define `_FILE_OFFSET_BITS=64` for all translation units that include `vhfile.hpp`; the implementation defines it as a fallback, and the CMake target exports it publicly.

```bash
c++ -std=c++17 -D_FILE_OFFSET_BITS=64 -I vhlibfsys/src \
    your_app.cpp vhlibfsys/src/vhfile.cpp vhlibfsys/src/vhfsys.cpp
```

---

## Quick start

```cpp
#include "vhfile.hpp"

#include <cstdint>
#include <iostream>

int main()
{
    VHFile file("model.gguf");

    std::uint64_t size = 0;
    if (!file.filesize(&size))
    {
        std::cerr << "Cannot determine file size\n";
        return 1;
    }

    char magic[4] = {};
    if (!file.seek(0) || !file.read(magic, sizeof(magic)))
    {
        std::cerr << "Cannot read the file header\n";
        return 1;
    }

    std::cout << "size=" << size
              << " magic=" << std::string(magic, 4) << "\n";

    return 0;
}
```

`VHFile file("model.gguf")` stores the filename but does **not** open the file. The first operation (`filesize`) opens it read-only. The destructor closes it.

---

## API reference: `VHFile`

Header: `src/vhfile.hpp`

```cpp
class VHFile
{
public:
    VHFile();
    explicit VHFile(const std::string &filename);
    ~VHFile();

    VHFile(const VHFile &) = delete;
    VHFile &operator=(const VHFile &) = delete;

    VHFile(VHFile &&other) noexcept;
    VHFile &operator=(VHFile &&other) noexcept;

    void assign(const std::string &filename);

    bool filesize(std::uint64_t *size);
    bool pos(std::uint64_t *pos);
    bool seek(std::uint64_t pos);

    bool read(void *ptr, std::size_t sz, std::uint64_t seekoffs = 0);
    bool write(const void *ptr, std::size_t sz);

    bool openfile();
    bool createfile();
    bool closefile();

    bool isopen() const;
    bool isclosed() const;

private:
    void dmperr();
    void dmpwerr();

    std::string fname;
    FILE *file;
    bool writable;
};
```

### Class overview

`VHFile` owns a single `FILE *` handle and a filename string. The `writable` flag records whether the handle was opened by `createfile()` (write-only, `"wb"`) or by `openfile()` (read-only, `"rb"`).

The class is intentionally small:

- it does not buffer data itself;
- it does not provide formatted input/output;
- it does not use `mmap` or positional `pread`/`ReadFile`;
- it does not lock the file.

### Lifetime and ownership

| Operation | Behaviour |
|---|---|
| `VHFile()` | Creates a closed object with an empty filename. |
| `VHFile(const std::string &)` | Stores the filename but does **not** open the file. |
| `~VHFile()` | Calls `closefile()` if the file is open. No exception is thrown. |
| Copy construction / copy assignment | Deleted. A `FILE *` must have exactly one owner. |
| Move construction / move assignment | Transfers the handle and filename. The moved-from object becomes closed and safe to destroy. |
| `assign()` | Closes the current handle (if any) and stores a new filename. It does not open the new file. |

A `VHFile` can be stored in `std::vector` or returned from a factory because move operations are available.

### Open modes

`VHFile` has two explicit modes:

| Mode | Opener | `fopen` mode | Effect |
|---|---|---|---|
| Read-only | `openfile()` | `"rb"` | Opens an existing file for reading. Does not truncate. |
| Write-only | `createfile()` | `"wb"` | Creates a new file or truncates an existing one for writing. |

Consequences:

- `read()`, `seek()`, `pos()`, and `filesize()` on a closed object implicitly call `openfile()`.
- `write()` on a closed object implicitly calls `createfile()`.
- `read()` returns `false` if the handle is open for writing.
- `write()` returns `false` if the handle is open for reading.
- To switch from reading to writing (or vice versa), call `closefile()` first, then the appropriate opener, or call `assign()` with the same filename.
- There is no append mode and no read/write (`"r+b"`) mode in the current API.

<a id="function-reference"></a>
### Function reference

<a id="vhfile-constructor-default"></a>
#### `VHFile()`

Creates a closed `VHFile` with an empty filename.

**Parameters:** none.

**Returns:** nothing.

**Notes:**
- `file` is `nullptr`; `writable` is `false`.
- The object is valid but cannot perform I/O until `assign()` supplies a filename.

```cpp
VHFile file;
file.assign("data.bin");
```

---

<a id="vhfile-constructor-filename"></a>
#### `explicit VHFile(const std::string &filename)`

Creates a closed `VHFile` and stores `filename`.

**Parameters:**
- `filename` — path to the file that will be opened later.

**Returns:** nothing.

**Notes:**
- The constructor does **not** open the file. This makes construction cheap and lets the caller decide whether to read or write.
- The first read-type operation opens the file with `"rb"`; the first `write()` opens it with `"wb"`.

```cpp
VHFile file("model.gguf");
std::uint64_t size = 0;
file.filesize(&size); // opens the file read-only
```

---

<a id="vhfile-destructor"></a>
#### `~VHFile()`

Destroys the object and closes the file if it is still open.

**Parameters:** none.

**Returns:** nothing.

**Notes:**
- Calls `closefile()`; errors from `fclose` are ignored during destruction.
- Because the class is move-only, destruction cannot double-close a handle by accident.

---

<a id="vhfile-copy"></a>
#### `VHFile(const VHFile &) = delete` / `VHFile &operator=(const VHFile &) = delete`

Copy construction and copy assignment are explicitly disabled.

**Parameters:** not applicable.

**Returns:** not applicable.

**Notes:**
- A `FILE *` must have exactly one owner; a shallow copy would make two destructors close the same handle.
- Use move operations or references/pointers when passing a `VHFile` around.
- This also prevents accidental copies during `std::vector` reallocation; the container uses the move constructor instead.

---

<a id="vhfile-move-constructor"></a>
#### `VHFile(VHFile &&other) noexcept`

Move constructor. Transfers the filename, handle, and write-mode flag from `other`.

**Parameters:**
- `other` — object to move from.

**Returns:** nothing.

**Notes:**
- After the move, `other.file == nullptr`, so `other` is closed and safe to destroy.
- The moved-to object owns the handle and is responsible for closing it.
- The move is `noexcept`, which allows `std::vector` to relocate `VHFile` objects without copying.

```cpp
VHFile a("data.bin");
VHFile b(std::move(a)); // b owns the handle; a is closed
```

---

<a id="vhfile-move-assignment"></a>
#### `VHFile &operator=(VHFile &&other) noexcept`

Move assignment. Closes the current handle, then takes ownership of `other`'s filename and handle.

**Parameters:**
- `other` — object to move from.

**Returns:** `*this`.

**Notes:**
- Self-assignment is checked and does nothing.
- The current handle is closed before the new one is adopted.
- `other` is left closed.
- The operation is `noexcept`.

```cpp
VHFile a("a.bin");
VHFile b("b.bin");
a = std::move(b); // a closes a.bin, then owns b.bin
```

---

<a id="vhfile-assign"></a>
#### `void assign(const std::string &filename)`

Changes the filename associated with the object.

**Parameters:**
- `filename` — new path.

**Returns:** nothing.

**Notes:**
- If a file is currently open, it is closed first.
- The new file is not opened immediately.
- If the previous `fclose` fails, the failure is ignored and the new name is still assigned.

```cpp
VHFile file("old.bin");
file.assign("new.bin");
file.openfile(); // opens new.bin
```

---

<a id="vhfile-filesize"></a>
#### `bool filesize(std::uint64_t *size)`

Returns the current size of the file in bytes.

**Parameters:**
- `size` — output pointer that receives the size. Must not be `nullptr`.

**Returns:**
- `true` on success and `*size` is valid.
- `false` if `size == nullptr`, the file cannot be opened, the flush fails, `fstat`/`_fstat64` fails, or the reported size is negative.

**Notes:**
- If the file is closed, it is opened read-only first.
- If the file is open for writing, buffered data is flushed before `fstat` so the returned size includes pending writes.
- The function does **not** change the current file position.
- The size is returned as `std::uint64_t`; values above 2 GiB are supported.

```cpp
std::uint64_t size = 0;
if (!file.filesize(&size))
    return false;
```

---

<a id="vhfile-pos"></a>
#### `bool pos(std::uint64_t *pos)`

Returns the current file position.

**Parameters:**
- `pos` — output pointer that receives the absolute offset in bytes. Must not be `nullptr`.

**Returns:**
- `true` on success.
- `false` if `pos == nullptr`, the file cannot be opened, or the 64-bit tell operation fails.

**Notes:**
- If the file is closed, it is opened read-only first.
- Uses `ftello` on POSIX and `_ftelli64` on Windows.
- The position is an absolute offset from the beginning of the file.

```cpp
std::uint64_t position = 0;
if (!file.pos(&position))
    return false;
```

---

<a id="vhfile-seek"></a>
#### `bool seek(std::uint64_t pos)`

Moves the read/write position to an absolute byte offset.

**Parameters:**
- `pos` — absolute offset from the beginning of the file.

**Returns:**
- `true` on success.
- `false` if the file cannot be opened or the seek fails.

**Notes:**
- If the file is closed, it is opened read-only first.
- The operation is always absolute (`SEEK_SET`). There is no relative seek and no `SEEK_END` variant; use `filesize()` and arithmetic if you need them.
- Uses `fseeko` on POSIX and `_fseeki64` on Windows.
- Offsets above 2 GiB are supported.
- The function validates that the 64-bit value fits into the platform's `off_t` / `__int64`.

```cpp
if (!file.seek(5ull * 1024 * 1024 * 1024)) // 5 GiB
    return false;
```

---

<a id="vhfile-read"></a>
#### `bool read(void *ptr, std::size_t sz, std::uint64_t seekoffs = 0)`

Reads exactly `sz` bytes into `ptr`.

**Parameters:**
- `ptr` — destination buffer. Must not be `nullptr` when `sz != 0`.
- `sz` — number of bytes to read.
- `seekoffs` — optional absolute offset to seek to before reading. The default value `0` means **do not seek**.

**Returns:**
- `true` if exactly `sz` bytes were read (`sz == 0` also returns `true`).
- `false` on a null destination, open failure, seek failure, read error, or premature end of file.

**Notes:**
- If the file is closed, it is opened read-only first.
- The function returns `false` if the handle is open for writing.
- The read is performed in a loop until all requested bytes are transferred or an error occurs. A partial read is treated as failure.
- On success, the file position advances by `sz` bytes. On a short read, it advances by the number of bytes actually transferred.
- `seekoffs` is absolute. Because `0` means “no seek”, the parameter cannot be used to explicitly seek to offset 0; use `seek(0)` for that.
- The function does not allocate memory; the caller owns the buffer.

```cpp
char magic[4] = {};
if (!file.read(magic, sizeof(magic)))
    return false;

// Read from an explicit absolute offset:
std::uint8_t header[16] = {};
if (!file.read(header, sizeof(header), 1024))
    return false;
```

---

<a id="vhfile-write"></a>
#### `bool write(const void *ptr, std::size_t sz)`

Writes exactly `sz` bytes from `ptr`.

**Parameters:**
- `ptr` — source buffer. Must not be `nullptr` when `sz != 0`.
- `sz` — number of bytes to write.

**Returns:**
- `true` if exactly `sz` bytes were written (`sz == 0` also returns `true`).
- `false` on a null source, open/create failure, write error, or short write.

**Notes:**
- If the file is closed, it is created or truncated first (`"wb"`). If the file is open for reading, the function returns `false`.
- Writing starts at the current position. Use `seek()` to change it.
- The write is performed in a loop until all requested bytes are transferred or an error occurs.
- The function does **not** flush. Data is flushed by `closefile()`, the destructor, or `filesize()` when called on a writable handle. There is no public `flush()` method in the current API.
- `sz == 0` returns `true`, but if the handle was closed it still creates/truncates the file first.

```cpp
VHFile out("output.bin");
const char data[] = "hello";
if (!out.write(data, sizeof(data) - 1))
    return false;
out.closefile();
```

---

<a id="vhfile-openfile"></a>
#### `bool openfile()`

Opens the file for reading in binary mode (`"rb"`).

**Parameters:** none.

**Returns:**
- `true` if the file is open for reading.
- `false` if the file cannot be opened, or if it is already open for writing.

**Notes:**
- If the file is already open read-only, the function returns `true` (idempotent).
- If the file is already open writable, it returns `false`; close it first to reopen for reading.
- On failure, an error message with `errno` is printed to `stderr`.
- Does not truncate the file.
- The filename must have been set with `assign()` or the filename constructor.

```cpp
VHFile file("input.bin");
if (!file.openfile())
    return false;
```

---

<a id="vhfile-createfile"></a>
#### `bool createfile()`

Opens the file for writing in binary mode (`"wb"`), creating it if necessary and truncating it if it exists.

**Parameters:** none.

**Returns:**
- `true` if the file is open for writing.
- `false` if the file cannot be created, or if it is already open for reading.

**Notes:**
- If the file is already open writable, the function returns `true` (idempotent).
- If the file is already open read-only, it returns `false`; close it first to reopen for writing.
- **Destructive:** an existing file is truncated immediately.
- On failure, an error message with `errno` is printed to `stderr`.

```cpp
VHFile out("output.bin");
if (!out.createfile())
    return false;
```

---

<a id="vhfile-closefile"></a>
#### `bool closefile()`

Closes the file and releases the handle.

**Parameters:** none.

**Returns:**
- `true` if `fclose` succeeded.
- `false` if the file was already closed or if `fclose` reported an error.

**Notes:**
- The handle is reset to `nullptr` and `writable` is set to `false` even if `fclose` fails, so the object remains safe to destroy.
- Safe to call multiple times; the second call returns `false`.
- Closing flushes buffered writes.
- `fclose` errors are returned but not printed.

```cpp
if (!file.closefile())
{
    // Either the file was already closed, or flushing/closing failed.
}
```

---

<a id="vhfile-isopen"></a>
#### `bool isopen() const`

Reports whether a file handle is currently open.

**Parameters:** none.

**Returns:** `true` if `file != nullptr`, otherwise `false`.

**Notes:** const-qualified; does not modify the object.

---

<a id="vhfile-isclosed"></a>
#### `bool isclosed() const`

Reports whether the object currently has no open handle.

**Parameters:** none.

**Returns:** `true` if `file == nullptr`, otherwise `false`.

**Notes:** equivalent to `!isopen()`.

---

### Private helpers

<a id="vhfile-dmperr"></a>
#### `void dmperr()`

Internal diagnostic used by `openfile()`. Prints a message to `stderr` containing the filename and `std::strerror(errno)`.

---

<a id="vhfile-dmpwerr"></a>
#### `void dmpwerr()`

Internal diagnostic used by `createfile()`. Prints a message to `stderr` containing the filename and `std::strerror(errno)`.

---

### Data members

| Member | Type | Purpose |
|---|---|---|
| `fname` | `std::string` | Filename associated with the object. |
| `file` | `FILE *` | Open handle, or `nullptr` when closed. |
| `writable` | `bool` | `true` when opened by `createfile()` (`"wb"`), `false` for `"rb"`. |

### Large-file behaviour

- Offsets and sizes are `std::uint64_t` throughout the public API.
- On POSIX, `_FILE_OFFSET_BITS=64` makes `off_t`, `fseeko`, `ftello`, and `fstat` 64-bit. The library defines it in `vhfile.cpp` and exports it from CMake.
- On Windows, `_fseeki64`, `_ftelli64`, and `_fstat64` are used directly.
- `read()` and `write()` accept `std::size_t` byte counts. On a 64-bit process this can exceed 2 GiB per call, but the caller must provide a buffer of that size. For very large transfers, chunk the operation (see [Usage examples](#usage-examples)).
- `filesize()` returns `std::uint64_t`.
- `VHFile` is still a sequential stream: it has one shared file position and no positional `pread`/`ReadFile` API. Use separate `VHFile` instances or an OS-specific positional API if concurrent random access is required.

### Thread safety

`VHFile` is **not thread-safe for a single instance**. Concurrent calls on the same object can interleave `fread`/`fwrite`/`fseek` operations and corrupt the stream position.

Different `VHFile` instances that refer to different files are independent and can be used from different threads. `VHFSYS` has no mutable global state, but filesystem operations are subject to normal OS race conditions (for example, a file can disappear between an existence check and an open).

---

## API reference: `VHFSYS`

Header: `src/vhfsys.hpp`

```cpp
class VHFSYS
{
public:
    using verr = std::uint32_t;

    static bool FileExist(const std::string &filename);

    static bool DirExist(const std::filesystem::path &dirpath);
    static bool DirExist(const std::string &dirpath);

    static verr CreateNestedDir(const std::string &dirpath);

    static std::vector<std::uint8_t> ReadVectorFromFile(const std::string &filename);
    static verr WriteVectorToFile(const std::string &filename,
                                  const std::vector<std::uint8_t> &data);

    static std::string ExtractFilename(const std::string &fullPathFileName);
};
```

Header dependencies: `vhfsys.hpp` includes `<filesystem>`, `<string>`, and `<vector>`. It does not include `<cstdint>` explicitly and uses the unqualified `uint32_t` / `uint8_t` names, which are provided in practice by the standard headers above on the tested toolchains (GCC, Clang, MinGW). Include `<cstdint>` before `vhfsys.hpp` if your standard library does not expose those names in the global namespace.

<a id="vhfsys-class-overview"></a>
### Class overview

`VHFSYS` is a stateless utility class. All member functions are `static`; there is no need to instantiate it. It uses `std::filesystem` for path queries and `std::ifstream` / `std::ofstream` for whole-file vector I/O.

`verr` is the nested type alias `VHFSYS::verr`, which is `std::uint32_t`. Successful operations return `0`. The names `vok` and `verror()` are internal helpers used inside `vhfsys.cpp`; they are **not** exported by `vhfsys.hpp` and are therefore not available to consumers. Compare the returned value with `0` (or with `VHFSYS::verr(0)`).

If the same project also uses `vhlibplatform`, its global `verr`/`vok`/`verror` are separate declarations. They have compatible types, so comparing `VHFSYS::verr` against `vok` still works, but `vhlibfsys` does not require or include that library.

<a id="vhfsys-function-reference"></a>
### Function reference

<a id="vhfsys-fileexist"></a>
#### `static bool FileExist(const std::string &filename)`

Checks whether a path exists and is not a directory.

**Parameters:**
- `filename` — path to inspect.

**Returns:**
- `true` if the path exists and `std::filesystem::is_directory()` is `false`.
- `false` if the path does not exist, is a directory, or the check fails.

**Notes:**
- The implementation does not verify that the path is a **regular** file. A FIFO, socket, device, or other non-directory filesystem object returns `true`.
- The function follows symbolic links: a symlink to a regular file returns `true`; a symlink to a directory returns `false`; a broken symlink returns `false`.
- Access permissions are not checked.
- The current implementation uses throwing `std::filesystem` overloads. On an OS-level error (for example, permission denied or a symlink loop), `std::filesystem::filesystem_error` may propagate.

```cpp
if (!VHFSYS::FileExist("model.gguf"))
    return false;
```

---

<a id="vhfsys-direxist-path"></a>
#### `static bool DirExist(const std::filesystem::path &dirpath)`

Checks whether a path exists and is a directory.

**Parameters:**
- `dirpath` — path to inspect.

**Returns:**
- `true` if `dirpath` is non-empty, exists, and is a directory.
- `false` for an empty path, a missing path, a non-directory path, or a failed check.

**Notes:**
- Symbolic links are followed; a symlink to a directory returns `true`.
- The current implementation may throw `std::filesystem::filesystem_error` on OS-level errors.

```cpp
if (!VHFSYS::DirExist(std::filesystem::path("output")))
    return false;
```

---

<a id="vhfsys-direxist-string"></a>
#### `static bool DirExist(const std::string &dirpath)`

Convenience overload that converts a string to `std::filesystem::path` and calls the path overload.

**Parameters:**
- `dirpath` — directory path as a string.

**Returns:** same as the path overload.

**Notes:**
- The two `DirExist` overloads make a bare string literal ambiguous:

  ```cpp
  VHFSYS::DirExist("output");            // ambiguous: path or std::string?
  VHFSYS::DirExist(std::string("output")); // OK
  VHFSYS::DirExist(std::filesystem::path("output")); // OK
  ```

- Pass a `std::string` or `std::filesystem::path` explicitly when calling with a literal.

```cpp
if (!VHFSYS::DirExist(std::string("output")))
    return false;
```

---

<a id="vhfsys-createnesteddir"></a>
#### `static verr CreateNestedDir(const std::string &dirpath)`

Creates a directory and all missing parent directories.

**Parameters:**
- `dirpath` — directory path to create.

**Returns:**
- `0` if the directory already exists or was created successfully.
- `1` if `dirpath` is empty, the path exists but is not a directory, or directory creation fails.

**Notes:**
- Existing parent directories are preserved.
- The operation is idempotent when the target is already a directory.
- If the path exists as a regular file, the function returns `1`.
- The current implementation may throw `std::filesystem::filesystem_error` on OS-level errors.
- The check-then-create sequence is not atomic; another process can change the filesystem between the check and `create_directories`.

```cpp
if (VHFSYS::CreateNestedDir("output/2026/data") != 0)
    return false;
```

---

<a id="vhfsys-readvectorfromfile"></a>
#### `static std::vector<uint8_t> ReadVectorFromFile(const std::string &filename)`

Reads the entire contents of a file into a byte vector.

**Parameters:**
- `filename` — path to the input file.

**Returns:**
- A `std::vector<uint8_t>` containing the file data.
- An empty vector if the file is empty, cannot be opened, cannot be measured, or a read error occurs.

**Notes:**
- The file is opened in binary mode with `std::ios::ate` to determine its size.
- The function allocates a vector of the full file size and reads the file in one `std::ifstream::read` call.
- **Empty result is ambiguous:** an empty file and an I/O error both produce an empty vector.
- **Not suitable for multi-gigabyte files.** The whole file is held in memory; large files can cause `std::bad_alloc` or `std::length_error`.
- `std::streamsize` and `std::size_t` limits apply.
- For large files, use `VHFile::read()` in chunks instead.

```cpp
std::vector<uint8_t> data = VHFSYS::ReadVectorFromFile("small.bin");
if (data.empty())
{
    // Either the file is empty or an error occurred.
}
```

---

<a id="vhfsys-writevectortofile"></a>
#### `static verr WriteVectorToFile(const std::string &filename, const std::vector<uint8_t> &data)`

Writes a byte vector to a file, creating parent directories when needed.

**Parameters:**
- `filename` — output path.
- `data` — bytes to write.

**Returns:**

| Code | Meaning |
|---|---|
| `0` | Success. |
| `1` | Empty filename, or a parent directory could not be created. |
| `2` | The output file could not be opened. |
| `3` | A write, flush, or close operation failed. |

**Notes:**
- If the parent directory does not exist, `CreateNestedDir()` is called first.
- The output file is opened with `std::ios::binary | std::ios::trunc`; an existing file is overwritten.
- Empty `data` creates an empty file (after truncation).
- The write is **not atomic**. A failure in the middle can leave a partially written file.
- The function does not call `fsync` or `FlushFileBuffers`; durability is left to the OS.
- `data.size()` is converted to `std::streamsize`; the vector must fit into that type.

```cpp
std::vector<uint8_t> data = {0x01, 0x02, 0x03};
VHFSYS::verr rc = VHFSYS::WriteVectorToFile("output/data.bin", data);
if (rc != 0)
    return false;
```

---

<a id="vhfsys-extractfilename"></a>
#### `static std::string ExtractFilename(const std::string &fullPathFileName)`

Returns the filename component of a path string.

**Parameters:**
- `fullPathFileName` — full or relative path.

**Returns:**
- The substring after the last `/` or `\`.
- The input unchanged if it contains no separator.
- An empty string if the input is empty or ends with a separator.

**Notes:**
- Both forward slashes and backslashes are recognised on every platform.
- Drive letters are not treated specially. For example, `"C:\\"` returns an empty string because the path ends with a separator.
- This is a string operation; it does not resolve `.` / `..` or verify that the path exists.

| Input | Result |
|---|---|
| `"/home/user/file.bin"` | `"file.bin"` |
| `"C:\\data\\model.gguf"` | `"model.gguf"` |
| `"file.bin"` | `"file.bin"` |
| `"/home/user/"` | `""` |
| `""` | `""` |
| `"C:\\"` | `""` |

```cpp
std::string name = VHFSYS::ExtractFilename("output/models/model.gguf");
// name == "model.gguf"
```

---

### Return codes

`VHFSYS` defines its own `verr` type alias inside the class:

```cpp
class VHFSYS
{
public:
    using verr = std::uint32_t;
    // ...
};
```

Successful operations return `0`. Internally `vhfsys.cpp` uses `vok` (`0`) and `verror(code)` (returns `code`), but these names are not part of the public header. In consumer code use `0`, or define your own constant:

```cpp
constexpr VHFSYS::verr vok = 0;

if (VHFSYS::CreateNestedDir("output/2026/data") != vok)
    return false;
```

| Function | Success | Failure |
|---|---|---|
| `FileExist` | `true` | `false` |
| `DirExist` | `true` | `false` |
| `CreateNestedDir` | `0` | `1` |
| `ReadVectorFromFile` | non-empty vector (or empty for an empty file) | empty vector |
| `WriteVectorToFile` | `0` | `1`, `2`, or `3` |
| `ExtractFilename` | filename string | empty string |

---

## Usage examples

### Read a binary header and check the size

```cpp
#include "vhfile.hpp"

#include <cstdint>
#include <iostream>

bool ReadHeader(const std::string &path)
{
    VHFile file(path);

    std::uint64_t size = 0;
    if (!file.filesize(&size))
        return false;

    if (size < 8)
        return false;

    char magic[4] = {};
    std::uint32_t version = 0;

    if (!file.seek(0))
        return false;

    if (!file.read(magic, sizeof(magic)))
        return false;

    if (!file.read(&version, sizeof(version)))
        return false;

    std::cout << "magic=" << std::string(magic, 4)
              << " version=" << version
              << " size=" << size << "\n";

    return true;
}
```

### Write data at an offset beyond 2 GiB

```cpp
#include "vhfile.hpp"

bool WriteMarker(const std::string &path)
{
    VHFile file(path);

    if (!file.createfile())
        return false;

    const std::uint64_t offset = 4ull * 1024 * 1024 * 1024; // 4 GiB
    if (!file.seek(offset))
        return false;

    const char marker[4] = {'A', 'B', 'C', 'D'};
    if (!file.write(marker, sizeof(marker)))
        return false;

    return file.closefile();
}
```

### Copy a large file in chunks

```cpp
#include "vhfile.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

bool CopyLargeFile(const std::string &src, const std::string &dst)
{
    VHFile in(src);
    VHFile out(dst);

    std::uint64_t remaining = 0;
    if (!in.filesize(&remaining))
        return false;

    // Create/truncate the destination even when the source is empty.
    if (!out.createfile())
        return false;

    constexpr std::size_t chunkSize = 1 << 20; // 1 MiB
    std::vector<std::uint8_t> buffer(chunkSize);

    while (remaining != 0)
    {
        const std::size_t chunk = static_cast<std::size_t>(
            std::min<std::uint64_t>(remaining, buffer.size()));

        if (!in.read(buffer.data(), chunk))
            return false;

        if (!out.write(buffer.data(), chunk))
            return false;

        remaining -= chunk;
    }

    return out.closefile();
}
```

`out.createfile()` creates or truncates the destination before copying. Do not point `src` and `dst` at the same file.

### Filesystem helpers

```cpp
#include "vhfsys.hpp"

#include <string>
#include <vector>

bool SaveSmallFile(const std::string &path, const std::vector<std::uint8_t> &data)
{
    if (!VHFSYS::DirExist(std::string("output")))
    {
        if (VHFSYS::CreateNestedDir("output") != 0)
            return false;
    }

    return VHFSYS::WriteVectorToFile(path, data) == 0;
}
```

---

## Error handling summary

### `VHFile`

| Function | Failure indication | Side effect on failure |
|---|---|---|
| `filesize` | `false`; `*size` unchanged | May open the file read-only. |
| `pos` | `false`; `*pos` unchanged | May open the file read-only. |
| `seek` | `false` | May open the file read-only. |
| `read` | `false`; buffer may contain partial data | Position advances by the bytes actually read. |
| `write` | `false` | May create/truncate the file; position advances by the bytes actually written. |
| `openfile` | `false`; message to `stderr` | Handle remains closed (or stays write-only). |
| `createfile` | `false`; message to `stderr` | Handle remains closed (or stays read-only). |
| `closefile` | `false` | Handle is reset even if `fclose` fails. |
| `isopen` / `isclosed` | — | Pure queries. |

`errno` may be set by the underlying C call. `openfile()` and `createfile()` print a diagnostic with `std::strerror(errno)`; the other operations return `false` without printing.

### `VHFSYS`

| Function | Failure indication |
|---|---|
| `FileExist` / `DirExist` | `false`; may throw `std::filesystem::filesystem_error`. |
| `CreateNestedDir` | `1`; may throw `std::filesystem::filesystem_error`. |
| `ReadVectorFromFile` | Empty vector; cannot distinguish an empty file from an error. May throw `std::bad_alloc` / `std::length_error`. |
| `WriteVectorToFile` | `1`, `2`, or `3`. |
| `ExtractFilename` | Empty string for empty input or a trailing separator. |

---

## Known limitations and roadmap

The current revision is intentionally small. The following limitations are known:

1. **`VHFSYS` filesystem queries use throwing overloads.** `FileExist`, `DirExist`, and `CreateNestedDir` can propagate `std::filesystem::filesystem_error` on permission errors, symlink loops, and similar OS failures. A future revision should use the `std::error_code` overloads and return `false` / `verr` instead.
2. **`ReadVectorFromFile` loads the whole file.** It is convenient for small configuration and metadata files but unsuitable for multi-gigabyte models. Use `VHFile` with a chunked loop for large data.
3. **`WriteVectorToFile` is not atomic.** A partial write can leave a truncated output file. A write-to-temporary-then-rename strategy would be safer.
4. **No positional I/O.** `VHFile` has a single shared position. Concurrent random access requires separate instances or an OS-specific `pread` / `ReadFile` / `mmap` layer.
5. **No read/write or append mode.** `VHFile` supports read-only (`"rb"`) and write-only (`"wb"`) handles. Switching modes requires closing and reopening.
6. **`read()`'s `seekoffs == 0` means “no seek”.** To explicitly read from offset 0, call `seek(0)` first.
7. **`DirExist` overload ambiguity.** A bare string literal is ambiguous between `std::filesystem::path` and `std::string`; pass an explicit type.
8. **Narrow filename strings on Windows.** `VHFile` uses `std::fopen` with a narrow `std::string`. Non-ASCII paths depend on the active code page.
9. **No file locking.** The library does not lock files or coordinate access between processes.
10. **No `flush()` API.** `closefile()`, the destructor, or `filesize()` on a writable handle flushes buffered data.
11. **`FileExist` accepts non-regular files.** It only excludes directories; FIFOs, sockets, and devices return `true`.
12. **`WriteVectorToFile` does not fsync.** Data durability is left to the operating system.
13. **`vhfsys.hpp` relies on transitive `<cstdint>` inclusion.** The header uses unqualified `uint32_t` / `uint8_t` but includes only `<filesystem>`, `<string>`, and `<vector>`. This works on the tested GCC, Clang, and MinGW toolchains, but a different standard library may require an explicit `#include <cstdint>`.

Possible future additions:

- `std::error_code`-based filesystem helpers;
- streaming/chunked `ReadVectorFromFile` alternatives;
- atomic file writes via temporary file + rename;
- positional reads (`read_at`) and memory-mapped access;
- wide-character filename overloads for Windows;
- an explicit `flush()` method and read/write mode support;
- an explicit `<cstdint>` include in `vhfsys.hpp`.

---

## Version history

| Version | Changes |
|---|---|
| **0.1.1** | Removed the `vhlibplatform` dependency. `VHFSYS` now defines its own nested `verr` alias (`std::uint32_t`); `vok`/`verror` are internal helpers in `vhfsys.cpp`; `vhfsys.hpp` includes `<string>` and `<vector>` directly; `CMakeLists.txt` no longer searches for or links `vhlib_platform`; standalone builds work with the C++17 standard library only. |
| **0.1.0** | 64-bit offsets and sizes; large-file support on Windows and POSIX; binary-only mode; exact `read`/`write`; `filesize` via `fstat` without moving the position; move-only ownership; `seek` and `write` correctness fixes. |
| **0.0.2** | Initial public revision. |
| **0.0.1** | Initial implementation. |

The version macros are generated from the CMake project version into `version_vhlibfsys.h`:

```cpp
#include "version_vhlibfsys.h"

static_assert(VHLIB_FSYS_VERSION_MAJOR == 0);
```

---

## License

This project is licensed under the **MIT License**.

Copyright © 2006–2026 Viktor Glebov / V01G04A81.

The software is provided “AS IS”, without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose, and non-infringement.

---

**vhlibfsys** · Revision **0.1.1** · C++17 · MIT

(С) 2026 Viktor Glebov / V01G04A81
