# Parallel Filesystem Inspection

A C++17 example of inspecting filesystem metadata with bounded parallel workers and explicit error handling. It lists regular files beneath a chosen directory without opening their contents or modifying them.

The repository's historical name says “exception-free.” More precisely, the example uses `std::error_code` overloads for ordinary filesystem failures. Memory allocation, thread creation, and other runtime operations can still throw; those failures are reported at the program boundary.

## Build and run

Requirements: CMake 3.16+ and a C++17 toolchain with filesystem and thread support, such as MSVC, GCC 9+, or a recent Clang standard library.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

For MinGW-w64 with Ninja, add `-G Ninja`. Visual Studio executables are in `build/Release/`; Ninja/Makefiles executables are in `build/`.

```powershell
.\build\filesystem-inspect.exe "C:\Projects"
.\build\filesystem-inspect.exe                # Current directory
.\build\filesystem-inspect.exe --help
```

On Linux, use `./build/filesystem-inspect`. File paths go to standard output as UTF-8; the summary and errors go to standard error. Use a UTF-8 terminal to display all names. Exit codes: `0` for a completed inspection without reported errors, `1` for failures, `2` for invalid arguments.

## Code guide

| File | Purpose |
| --- | --- |
| `scan.hpp` | Result types and the reusable `inspection::scan` function |
| `scan.cpp` | Serial path collection followed by bounded parallel metadata inspection |
| `main.cpp` | Native path arguments, UTF-8 reporting, and error handling |
| `tests/scan_test.cpp` | Coverage, worker-count, Unicode and missing-root regressions |

Paths remain `std::filesystem::path` objects throughout. On Windows, the executable uses wide command-line arguments; it never narrows a wide filename by copying its individual characters into a byte string.

An atomic job index assigns every collected entry to exactly one worker, including the final entry. Each worker writes only to its own result slot. A reported hardware concurrency of zero falls back to one worker; the pool is capped at 32 workers and the number of entries. Futures wait for workers and propagate unexpected exceptions.

## Scope and tradeoffs

- Traversal uses `skip_permission_denied`; inaccessible subdirectories may be skipped. A successful run is not proof that every file on a volume was accessible.
- Directory symlinks are not traversed. Entries use `symlink_status`, so a file symlink is reported as a symlink, not counted as a regular file.
- Metadata can change after traversal. Errors observed during inspection are retained in each result rather than hidden by a prior file-open check.
- Path collection is serial and stored in memory. Only metadata inspection is parallel; speedups depend on the filesystem and workload.

The test creates and removes an isolated fixture beneath its build directory. It checks 258 regular files, Unicode paths, empty and missing roots, and worker requests of 0, 1, 2, 4, and 1000.

## License

This repository does not currently include a license file.
