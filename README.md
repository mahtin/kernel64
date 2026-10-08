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

## Description of code

With massive respect to John Lions and his books abount the Unix v6 kernel (which taught me back in the 80's), I quote [this](https://wiki.tuhs.org/doku.php?id=anecdotes:not_expected_to_understand_this):
```C
	/* You are not expected to understand this. */
```

## CI

Presently tested on three platforms via GitHub actions:

- MacOS ARM 
  - OS: 26.6.2
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- MacOS Intel
  - OS: 26.6.1
  - Compiler: Apple clang version 21.0.0 (clang-2100.1.1.101)
- Ubuntu Intel
  - OS: Ubuntu 24.04.5 LTS
  - Compiler: cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
- Windows Intel

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

