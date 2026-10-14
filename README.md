<div align="center">

# BanglaScript

### A Bengali programming language built with C

<p>
  <em>
    BanglaScript is an experimental programming language designed to make
    programming more accessible to Bengali speakers.
  </em>
</p>

<p>
  <img src="https://img.shields.io/badge/Language-C-A8B9CC?style=for-the-badge&logo=c&logoColor=white" alt="C">
  <img src="https://img.shields.io/badge/Platform-Cross--platform-2ea44f?style=for-the-badge" alt="Cross-platform">
  <img src="https://img.shields.io/badge/Project-Experimental-orange?style=for-the-badge" alt="Experimental project">
</p>

</div>

## About

BanglaScript is a programming language project written in the C programming
language. It aims to provide Bengali-friendly programming syntax and a simple
foundation for learning programming, language design, and compiler or
interpreter development.

The repository now contains a self-contained C11 compiler/runtime. It parses
BanglaScript into an AST and executes it with lexical environments, functions,
and source-line diagnostics. The implementation has no third-party
dependencies.

## Goals

- Make programming concepts more approachable for Bengali speakers.
- Provide readable Bengali syntax for common programming constructs.
- Build a lightweight and understandable language implementation in C.
- Serve as an educational project for language and compiler development.

## Planned Features

- Bengali keywords and identifiers
- Variables and basic data types
- Arithmetic and logical expressions
- Conditional statements
- Loops
- Functions and return values
- Clear syntax and helpful error messages

## Getting Started

### Prerequisites

You need a C compiler and Git installed on your system.

- [GCC](https://gcc.gnu.org/) or [Clang](https://clang.llvm.org/)
- [Git](https://git-scm.com/)

### Clone the repository

```bash
git clone https://github.com/nosibbiswas22/bangla_script.git
cd bangla_script
```

### Build

The project uses a modular layout:

```text
include/   public compiler and runtime interfaces
src/       lexer, parser, runtime, symbol table, and CLI sources
tests/     BanglaScript sample programs
```

Using Make:

```bash
make
```

Run the sample integration program:

```bash
make test
```

The equivalent direct GCC command is:

```bash
gcc -Wall -Wextra -std=c11 -pedantic -Iinclude -o blang src/*.c -lm
```

### Run

```bash
./blang tests/hello.bs
```

On Windows, run:

```powershell
.\blang.exe tests\hello.bs
```

After building, a source file can be run with the short command:

```text
blang fileName.bs
```

In PowerShell, use `.\blang.exe fileName.bs` when the project directory is not
on `PATH`. To use `blang fileName.bs` from any CMD or PowerShell window, add
the project directory to your user `PATH`:

```powershell
$userPath = [Environment]::GetEnvironmentVariable("Path", "User")
[Environment]::SetEnvironmentVariable(
    "Path",
    $userPath + ";E:\Code-X\c\bangla_script",
    "User"
)
```

Restart CMD or PowerShell after changing `PATH`.

`blang` currently compiles the C implementation into a BanglaScript runtime
and executes `.bs` source files. It does not yet emit native machine-code
executables like GCC.

## Language reference

Statements end with `;`. Braces create blocks. Bengali keywords have English
aliases, so either spelling can be used:

```text
ধরি নাম = "বাংলা";
ফাংশন অভিবাদন(ব্যক্তি) {
    দেখাও ব্যক্তি + ", পৃথিবী!";
}
অভিবাদন(নাম);

ধরি n = 1;
যতক্ষণ (n <= 5) {
    যদি (n % 2 == 0) দেখাও n;
    নাহলে দেখাও "বিজোড়";
    n = n + 1;
}
```

Supported Bengali keywords are `ধরি` (let), `দেখাও` (print), `যদি`/`নাহলে`
(if/else), `যতক্ষণ` (while), `ফাংশন` (function), `ফেরত` (return), `সত্য`,
`মিথ্যা`, and `শূন্য`. Operators include arithmetic, comparison, `&&`, `||`,
`!`, assignment, and function calls. Strings support `\n`, `\t`, `\"`, and
`\\`. Comments begin with `//`.

Run a program from a file:

```powershell
.\blang.exe tests\hello.bs
```

On Windows, the compiler configures the console for UTF-8 automatically. If
an older terminal still displays Bengali text incorrectly, run
`chcp 65001` before starting the program and use a Unicode-capable font such
as Nirmala UI or Noto Sans Bengali.

With no file argument the executable starts a small interactive REPL.

## Project status

BanglaScript is an experimental project. Syntax, implementation details, and
features may change as development continues.

## Community and contribution

Contributions, bug reports, ideas, and documentation improvements are welcome.
Please read the project guidelines before participating:

- [Contributing guide](CONTRIBUTING.md)
- [Code of Conduct](CODE_OF_CONDUCT.md)
- [Security policy](SECURITY.md)

When reporting a bug, include the operating system, compiler version, steps to
reproduce the issue, and the smallest relevant `.bs` program.

## License

BanglaScript is distributed under the [MIT License](LICENSE).

---

<div align="center">

Made with C and a vision for Bengali programming

</div>
