# cslo

C implementation of `slo`. `slo` is a high-level and dynamically typed language.
This is primarily based off of [Lox](https://craftinginterpreters.com/the-lox-language.html)
and this C implementation was created whilst reading
[Crafting Interpreters](https://craftinginterpreters.com).

Whilst I have made changes to the language as I went through (and since) it is still
primarily the same at the moment. The purpose of `slo` is a hobby and learning project and the
language will continue to see development and features as time goes on.
I will endeavour to keep the documentation up to date.

My primary goal is to one day be able to complete challenges like **Advent of Code** in my own language.

## Implementation goals

This is a list of some additional features and functionality I would like to change/add. They're not listed in
any particular order and are subject to change.

- ~~additional operands like `+=`, `++`, `--`~~
- ~~adding a `list` type~~
- adding a `dict` type for maps
- adding `enums`
- better error reporting
- everything as an object á la Python
- more builtin methods like `size`, `str`, ~~`max`~~, ~~`min`~~, etc
- string formatting
- standard library (and some `namespace`, `import`, `include` system)
  - things like a `math`, `io`, etc

I would like to eventually add type hints or some kind of type system but that's very much a stretch goal.

## Differences to lox

This is a non-exhaustive list of things that have been added / are different to `lox`.

- print is a function (`print()`) and supports multiple arguments
- support for `elif`
- `self` rather than `this` in classes
- `__init__` rather than `init` in classes
- `extends` for inheritance rather than `<`
- prefix and postfix increment / decrement (`--`, `++`)
- compound assignment operators (`+=`, `-=`, `*=`, `/=`)
- `len` native function for length of strings/lists
- math native functions, things like: `min`, `max`, `sin` / `cos` / `tan`, `ceil` / `floor`, `abs`, `sqrt`, etc
- environment variable handling with `setenv` and `getenv`
- `time()` to get current now
- `exit()` to exit with optional status code
- random native functions: `random()`, `randomInt()`, `randomRange()`
- lists declared like: `var x = [1, 2, 3];` and accessed like: `print(x[0]);` or `x[0] = 99`
- list slicing and support for negative indices
- list methods like: `insert`, `pop`, `remove`, `append`, `reverse`, `count`, `clear`
- membership checks with `has` and `has not` (ie `[1, 2, 3, 4] has 2  # true`)
- `for (var item in my_list) {}` syntax for lists
