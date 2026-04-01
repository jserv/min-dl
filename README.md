# min-dl: minimal dynamic linker implementation

`min-dl` is a minimal ELF dynamic linker that loads a shared library, performs
relocations, resolves symbols, and runs init/fini constructors -- all without
depending on glibc's `ld-linux` runtime.

## How it works
ELF executables and shared libraries that call external functions typically
contain a Procedure Linkage Table (PLT), which adds a level of indirection
for function calls analogous to that provided by the GOT for data. Deferring
symbol resolution can reduce startup cost because unresolved entries are only
fixed up when actually called.

`min-dl` loads a shared object via `mmap`, processes its dynamic section
(symbol tables, hash tables, relocation entries, init/fini arrays), and
performs the necessary relocations. Standard relocations (including PLT
`JUMP_SLOT` entries) are resolved eagerly at load time. A callback-based PLT
trampoline is also provided for callers that want deferred resolution through
the `set_plt_resolver` API.

## Supported architectures
- x86-64
- ARM (32-bit)
- AArch64
- RISC-V (64-bit)

Adding a new architecture requires an `arch/<name>.h` header with
platform-specific assembly macros, relocation handling in `loader.c`, and a
machine type check.

## Build and test

Prerequisites: a C toolchain (GCC), `make`, and for cross-architecture
testing, the corresponding cross-compiler and QEMU user-mode emulator.

```
make clean && make
make check
```

Cross-compilation targets (build only):
```
make arm          # ARM 32-bit
make aarch64      # AArch64
make riscv64      # RISC-V 64-bit
```

Each architecture has a corresponding `check_<arch>` target that runs the
tests under the appropriate QEMU user-mode emulator:
```
make check_arm
make check_aarch64
make check_riscv64
```

## Licensing
`min-dl` is licensed under the BSD 2-Clause License; see `LICENSE`.
