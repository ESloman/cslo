# cslo

C implementation of `slo`. `slo` is a high-level and dynamically typed language.
This is primarily based off of [Lox](https://craftinginterpreters.com/the-lox-language.html)
and this C implementation was created whilst reading
[Crafting Interpreters](https://craftinginterpreters.com).

The language has undergone quite a few changes that differentiates it from `lox`. Nothing major and most `lox` code should be runnable in `slo`, but there are a few changes. Most of the changes are additional functionality and support for new types, etc.

My primary goal is to one day be able to complete challenges like **Advent of Code** in my own language. You can read more about the [project goals here](./GOALS.md).

## Installing

Clone the repo.

```bash
git clone https://github.com/ESloman/cslo.git
```

Run the install script.

```bash
./util/install.sh
```

This should copy the latest `./build/cslo` file to `/usr/bin/slo` and create a symlink to `/usr/bin/cslo`.
It currently uses `sudo` for the `cp` commands.

## Running

After installing, simply do:

```bash
slo
```

to run the REPL. Or:

```bash
slo path/to/file.slo
```

## Major differences with lox

This is a non-exhaustive list of major things that are different to `lox`. This is mostly things that could cause `lox` code to in compatible with `slo`. A slightly more complete list can be found in [docs/differences_to_lox.md](docs/differences_to_lox.md).

- `print` is now a function (`print()` or `println()` with newlines) and supports multiple arguments
- `func` instead of `fun`
- `__init__` instead of `init`
- `self` instead of `this`
- `extends` rather than `<`

Besides from these differences, I _think_ that `lox` should be runnable in `slo`.

## VS Code Extension

A [VS Code extension](https://github.com/ESloman/slo-vscode) is available to provide syntax highlighting and basic language support for Slo files (`.slo`).
This extension highlights keywords, constants, operators, function definitions and calls, built-in functions, and more.

### Installing the Extension

If you want to use the extension:

1. Clone the [slo-vscode](https://github.com/ESloman/slo-vscode) repository.
2. Run the install script:

   ```sh
   ./install.sh
   ```

   This will install dependencies, package the extension, and install it into your local VS Code.

3. Open a `.slo` file in VS Code to activate syntax highlighting and language features.

For more details, see the [slo-vscode README](https://github.com/ESloman/slo-vscode/blob/main/README.md).
