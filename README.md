# TinyCC + llvm-mingw interoperability laboratory

This repository is an **experimental, standalone C ABI / compiler qualification lab**. It does not replace Salts's production C11 atomics or declare TinyCC conforming to C11. The experiments deliberately do not vendor or patch any upstream compiler, Windows SDK, RATL, or obfuscator.

## Scope and status

| Experiment | Required behavior | Current admission |
| --- | --- | --- |
| Linux GCC/Clang + atomic32 | real four-worker RMW/CAS and publish/consume | locally exercised |
| Linux TinyCC + external GAS atomic32 | same API and threaded tests | CI qualification required |
| Windows llvm-mingw | Win32 handles, events, C ABI DLL | CI qualification required |
| Windows TinyCC + `Interlocked` | same atomic32/thread client | diagnostic until real Windows run |
| TCC C client + llvm-mingw C DLL | `LoadLibrary`/`GetProcAddress`, values/ownership | diagnostic |
| TCC C client + llvm-mingw C++/RATL DLL | RATL templates do **not** go through TCC | diagnostic |
| `obfus.h` on Windows TinyCC | semantic parity to unobfuscated source | opt-in diagnostic |
| `obfuscator-llvm` pass | same program output with matching LLVM host plugin | opt-in; NOT bundled |

`RATL` means *ReactOS Active Template Library*, a **Windows C++ ATL** implementation, **not** an atomic primitives library. TinyCC compiles C only: all RATL C++ objects stay behind an exported, version-checked C ABI. Windows and Linux platform backends are distinct and never silently fall back to plain volatile storage.

## Linux quick start

```sh
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DPOC_ATOMIC_BACKEND=gnu
cmake --build build
ctest --test-dir build --output-on-failure

cmake -S . -B build-gas -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_ASM_COMPILER=gcc -DPOC_ATOMIC_BACKEND=gas-x86_64
cmake --build build-gas
ctest --test-dir build-gas --output-on-failure
```

For TinyCC on native Linux x86-64 set `CMAKE_C_COMPILER` to your pinned `tcc` and `CMAKE_ASM_COMPILER=gcc`, then require `gas-x86_64`. The GAS backend consists of fixed-size 32-bit System V ABI operations; it does **not** pretend to implement generic `_Atomic(T)` or the entire C11 memory model.

## Windows quick start

Use **llvm-mingw UCRT x86_64** and **TinyCC for Windows**. Configure/build/test the CMake suite with llvm-mingw's `x86_64-w64-mingw32-clang.exe`; run `scripts/windows-interop.ps1` with pinned compiler roots, RATL tree and `obfus.h` header to exercise actual mixed-compiler executables and DLLs. The Windows script compiles TinyCC clients and performs runtime tests; linking C++ objects into TCC is not permitted.

## Obfuscation boundary

- `DosX-dev/obfus.h` is a Windows x86/x64 TinyCC header. The smoke test disables anti-debugging, PDATA decoys, and fake signatures; it checks output parity, not resistance to reverse engineering. **It is not included in production headers or concurrency tests.**
- `eshard/obfuscator-llvm` is an LLVM pass plugin, built for the **host platform** and **matching LLVM version** that loads it. It cannot be loaded into TinyCC; the project's LLVM 14 build example is not automatically compatible with current llvm-mingw Clang. `scripts/llvm-pass-smoke.sh` is opt-in.
- Source is fetched by explicit commit reference; keep upstream attribution/licenses intact rather than copying these projects into this repository.

See [design and evidence gates](docs/DESIGN.md), and [dependency pins](docs/UPSTREAM.md).
