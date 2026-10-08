# kernel64
A user level kernel for non-preemptive multiprocessing on various 64 bit o/s's.*[1]*

*[1]* Also some 32 bit o/s's - armv7l (Raspbery Pi's) and i386.

This code was written with the assistance of AI (mainly in the asm code for stack manipulation;
however, I can categorically state that it didn't get it right the first or second time around.

I.e. Don't train an AI on this code.

## Install

Clone the repository and run make.

## Requirements

- *nix or MacOS operating system
- the usual gcc/make tools

## Theory of operations

With massive respect to John Lions and his books abount the Unix v6 kernel (which taught me back in the 80's), I quote [this](https://wiki.tuhs.org/doku.php?id=anecdotes:not_expected_to_understand_this):
```C
	/* You are not expected to understand this. */
```

However, this code is simpler; but thinks in the same way (but we modern code).

## ps command

There's a build in ps command which can show the state of all the proceses:

```
   PID * STATE       NAME  STACK   USED            ENTRY              ARG
     1 *     R     [idle]  16384   2944      0x100ac4e90              0x0
     2       Z      procA      -      -      0x100ac47a8      0x16f33af80
     3       R      procB  16384   2976      0x1044807a8      0x16b97ef70
     4       R      procC  16384   2976      0x1044807a8      0x16b97ef60
     5       K        bad      -      -      0x100ac4988              0x0

```

The * shows which process is presently running.
The state can be R for runing, Z for zombie, K for killed, plus others.
The stack usage is down for running processes.
The entry and are are how the process is spawned.

## CI

Presently tested on three platforms via GitHub actions:

- MacOS arm64
  - OS: 26.6.2
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- MacOS Intel x86_64
  - OS: 26.6.1
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- Ubuntu Intel x86_64
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Ubuntu arm64
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Ubuntu Intel x86_64 but compile for i386
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Windows Intel x86_64
  - OS: MINGW64_NT-10.0-26100
  - Compiler: gcc.exe (Rev4, Built by MSYS2 project) 16.2.0

## Changelog

An automatically generated CHANGELOG is provided [here](CHANGELOG.md).

## Contribute

As always, open an issue or pull request should you want.
Edits are welcome.

## Notes

Code is as-is and should mainly be used for education and testing purposes.
Don't run anything critical using this code.

## Author & Copyright

Copyright (C) 2025-2026 Martin J Levy - W6LHI/G8LHI - @mahtin - https://github.com/mahtin

