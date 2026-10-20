# Native dependencies

Reviewed against upstream on 2026-10-20. This inventory covers the CMake DLL and
test launcher; the Tauri UI has separate npm and Cargo manifests and lockfiles.

| Library | Version / commit | Source and update policy |
| --- | --- | --- |
| apclientpp | [v0.6.6](https://github.com/black-sliver/apclientpp/tree/v0.6.6), `6b28cc1781006deae0e28c61752d11a83819585a` | Git submodule; use release tags. The previous checkout reported 0.6.3 in CMake and 0.6.4 in its header. |
| wswrap | `aeba7ac428028723fb26ce92488f260660f786b1` | [Git submodule](https://github.com/black-sliver/wswrap/tree/aeba7ac428028723fb26ce92488f260660f786b1); upstream has no release tags. This update only advances its browser-only wsjs submodule. |
| standalone Asio | [1.32.0](https://github.com/chriskohlhoff/asio/tree/asio-1-32-0), `03ae834edbace31a96157b89bf50e5ee464e5ef9` | Git submodule; last release before the API removals in 1.33.0 that affect WebSocket++. |
| WebSocket++ | `4dfe1be74e684acca19ac1cf96cce0df9eac2a2d` | [Git submodule](https://github.com/zaphoyd/websocketpp/tree/4dfe1be74e684acca19ac1cf96cce0df9eac2a2d); current upstream master, containing 0.8.2 plus a CMake 4 compatibility fix. |
| nlohmann/json | [3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0) | CMake FetchContent release archive, verified with upstream's SHA-256 checksum. Shared by configuration loading and apclientpp. |
| OpenSSL | 3.6.5 in the local build | External development libraries found by CMake. Use static libraries with the dynamic MSVC runtime (`x64-windows-static-md` for vcpkg). The version is supplied by the developer's installation, not pinned by this repository. |
| MinHook | [1.3.4](https://github.com/TsudaKageyu/minhook/releases/tag/v1.3.4), with local changes | Vendored sources in `MinHook/`. Retain the local signed relative instruction operands and internal static variables when updating; do not replace the directory blindly. |
| stb_image / stb_image_write | 2.30 / 1.16 | Vendored headers in `Image/stb/`; match current [upstream](https://github.com/nothings/stb/tree/a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d). |
| stb_image_resize2 | 2.18 | Vendored header in `Image/stb/`. Upstream moved to [nothings/stb_image_resize](https://github.com/nothings/stb_image_resize) for 2.50. Retained because 2.50 explicitly changes output determinism relative to prior versions; updating needs a separate review of generated carbox images. |

## Building and updating

Clone with `--recurse-submodules`, or initialize the pinned dependencies:

```powershell
git submodule update --init --recursive
```

CMake initializes missing Archipelago dependencies automatically. It preserves
existing checkouts, so after pulling a dependency update, run the command above
to move initialized submodules to the commits recorded by the parent repository.
Avoid `git submodule update --remote` for routine builds: that bypasses the pins.

For a deliberate update, check that each submodule is clean, fetch upstream, check
out the selected tag or commit, update any nested submodules, then commit the
changed submodule pointers in the parent repository. For JSON, update both the
release URL and `URL_HASH` in `DLL/CMakeLists.txt` using the upstream release checksum.

Keep Asio and WebSocket++ compatible. [Asio 1.33.0 removed deprecated APIs](https://github.com/chriskohlhoff/asio/blob/asio-1-38-2/src/doc/history.qbk)
including `io_service`, `io_context::work`, and timer/resolver interfaces used by
the pinned WebSocket++ headers. Advancing to current Asio 1.38.2 requires a
WebSocket++ compatibility update first.

The Archipelago translation unit defines `ASIO_STANDALONE`, `AP_NO_SCHEMA`, and
`WSWRAP_NO_COMPRESSION`. Thus Boost, valijson, and zlib are not required by the
current build. TLS remains enabled and uses the Windows trusted root store.
Upstream warns that Archipelago may require compression in the future; enabling
it would require linking zlib and removing `WSWRAP_NO_COMPRESSION`.

Validate native updates in both configurations:

```powershell
cmake -S . -B build
cmake --build build --config Debug --target randomizer TestLauncher
cmake --build build --config Release --target randomizer TestLauncher
```

An OpenSSL toolchain or `OPENSSL_ROOT_DIR` must already be configured. See
`DLL/Archipelago/README.md` for the local TLS setup and connection-testing procedure.
Also run apclientpp's upstream `test/test_basic.cpp` with the same dependency
checkouts to exercise a local WebSocket connection and RoomInfo/RoomUpdate
callbacks. A successful build alone does not verify a live Archipelago room or
TLS certificate validation.
