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
- ~~adding a `dict` type for maps~~
- ~~adding `enums`~~
- better error reporting
- everything as an object á la Python
- more builtin methods like ~~`len`~~, `str`, ~~`max`~~, ~~`min`~~, etc
- string formatting
- standard library (~~and some `namespace`, `import`, `include` system~~)
  - things like a ~~`math`~~, `io`, `os`, ~~`random`~~, etc

I would like to eventually add type hints or some kind of type system but that's very much a stretch goal.

## Differences to lox

This is a non-exhaustive list of things that have been added / are different to `lox`.

- print is a function (`print()`) and supports multiple arguments
- `func` instead of `fun` for function declarations
- support for `elif`
- `self` rather than `this` in classes
- `__init__` rather than `init` in classes
- `extends` for inheritance rather than `<`
- prefix and postfix increment / decrement (`--`, `++`)
- compound assignment operators (`+=`, `-=`, `*=`, `/=`)
- `len` native function for length of strings/lists/dicts
- native functions like: `min`, `max`, `abs`, etc
- environment variable handling with `setenv` and `getenv`
- `time()` to get current now
- `exit()` to exit with optional status code
- beginnings of an import system with `import math;`
- stdlib modules:
  - `math` module for things like `sin`, `cos`, `tan`, `ceil`, `floor`, `abs`, `sqrt`, etc
  - `random` module for things like `random`, `randint`, `randrange`, `choice`, `shuffle`, `gauss`, etc

### Lists

Support for lists:

```slo
var mylist = [1, 2, 3, 4, 5];
for (var item in mylist) {
  print(item);
}
```

With slicing and negative indexing:

```slo
var x = [10, 20, 30, 40, 50];
print(x[1:4]);
print(x[:3]);
print(x[2:]);
print(x[-3:-1]);
```

List methods:

```slo
var x = [1, 2, 3, 4, 5];
x.append(6);
x.pop();
x.insert(0, 0);
x.remove(0);
x.reverse();
x.count(1);
var y = x.clone();
y.clear();
x.extend(y);
x.sort();
```

Membership checks:

```slo
var x = [1, 2, 3, 4, 6];
x has 6  # true
x has not 5  # true
```

### Dicts

Support for dictionaries:

```slo
var map = {
    "key": "value",
    "key1": "value1",
    "key2": "value2",
    "key": 1
};
print(map);

map["foo"] = "bar";
print(map["foo"]);

for (var key in map) {
  print(key, map[key]);
}
```

Dict methods:

```slo
var map = {"a": 1, "b": 2, "c": 3};
print(map.keys());
print(map.values());

for (var val in map.values()) {
  print(val);
}

map.clear();
map.pop("a");  # 1
var new_map = map.clone();

var new = {"e": 5, "f": 6};
map.update(new);
```

Membership checks:

```slo
var map = {
    "key": "value",
    "key1": "value1",
    "key2": "value2",
    "key": 1
};

map has "key";  # true
map has "foo";  # false
map has not "bar";  # true


map.get("foo");  # nil
map.get("foo", "bar");  # bar
```

### enums

Support for enums:

```slo
enum Colours {
  RED,     # 0
  ORANGE,  # 1
  BLUE,    # 2
  BLACK    # 3
}

print(Colours);
print(Colours.RED);
print(Colours.BLUE == 2);
```
