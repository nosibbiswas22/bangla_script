# Contributing to BanglaScript

Thank you for your interest in improving BanglaScript. Contributions of code,
tests, documentation, examples, translations, and issue reports are welcome.
Please read the [Code of Conduct](CODE_OF_CONDUCT.md) before participating.

## Before you start

For substantial changes, open an issue first so the design and scope can be
discussed. Small fixes and documentation updates can usually go directly into
a pull request.

## Development setup

1. Fork the repository and clone your fork.
2. Install a C11-compatible compiler and GNU Make (or an equivalent Make
   implementation).
3. Build the project:

   ```bash
   make
   ```

4. Run the sample integration program:

   ```bash
   make test
   ```

On Windows, use a GCC/Clang environment that provides `make`, or compile with
the direct command shown in the README.

## Making changes

- Keep changes focused and consistent with the existing C11 style.
- Preserve Bengali keywords and UTF-8 source handling.
- Add or update a small `.bs` example when changing language behavior.
- Update the README or other documentation when user-visible behavior changes.
- Do not commit generated executables or build output.
- Avoid unrelated formatting or refactoring in the same pull request.

## Pull requests

Before opening a pull request:

- Build the project with `make`.
- Run `make test`.
- Confirm that the change works on the platforms you claim to support.
- Describe the problem, the approach, and how you tested it.
- Keep the pull request title short and specific.

Maintainers may ask for revisions, tests, or documentation before merging.
Please respond to review feedback constructively.

## Commit messages

Use clear, imperative commit messages, for example:

```text
Add Bengali string escape handling
```

One logical change per commit is preferred.
