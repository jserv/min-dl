# min-dl: minimal dynamic linker implementation

To support dynamic linking, each ELF shared libary and each executable that
uses shared libraries has a Procedure Linkage Table (PLT), which adds a level
of indirection for function calls analogous to that provided by the GOT for
data. The PLT also permits "lazy evaluation", that is, not resolving
procedure addresses until they are called for the first time.

Since the PLT tends to have a lot more entries than the GOT, and most of the
routines will never be called in any given program, that can both speed
startup and save considerable time overall.

`min-dl` introduces a straightforward way to load the specified shared
objects and then perform the necessary relocations, including the shared
objects that the target shared object uses.

# Licensing
`min-dl` is freely redistributable under the two-clause BSD License.
Use of this source code is governed by a BSD-style license that can be found
in the `LICENSE` file.
