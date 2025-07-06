# Differences to `lox`

[Lox](https://craftinginterpreters.com/the-lox-language.html)
and
[Crafting Interpreters](https://craftinginterpreters.com) were the inspiration for `slo`. However, `slo` has digressed and will continue to digress from `lox` as more features are added and things are changed. This page attempts to document some of the major changes.

## Breaking Changes

'Breaking changes' that make `lox` code incompatible with `slo`.

- `print` is now a function (`print()` or `println()` with newlines) and supports multiple arguments
- `func` instead of `fun`
- `__init__` instead of `init`
- `self` instead of `this`
- `extends` rather than `<`

## New Features

- prefix and postfix increment / decrement (`--`, `++`)
- compound assignment operators (`+=`, `-=`, `*=`, `/=`)
- string formatting with `"Hello ${name}!"` syntax

### More native functions

- `len` native function for length of strings/lists/dicts
- native functions like: `min`, `max`, `abs`, etc
- `time()` to get current now
- `exit()` to exit with optional status code
- methods for some type conversions: `str()`, `bool()`, `number()`

### Standard library and imports

Can import standard library modules with `import math;` or `import math as maths;`.

Modules:

- `math` module for things like `sin`, `cos`, `tan`, `ceil`, `floor`, `abs`, `sqrt`, etc
- `random` module for things like `random`, `randint`, `randrange`, `choice`, `shuffle`, `gauss`, `sample`, etc
- `json` module for interacting with json strings / files with `load`, `loads`, `dump`, `dumps`
- `os` module for interacting with files / directories, environment variables, etc

### Strings

Added support for standard string methods:

```slo
"hello world!".upper()             # HELLO WORLD!
"HELLO WORLD!".lower()             # hello world!
"hello world!".title()             # Hello World!
"hello, world!".split(",")         # list [2]: ['hello', ' world!']
" hello, world!   ".strip()        # hello, world!
"Hello world".startswith("Hello")  # true
"Hello world".endswith("Hello")    # false

"Hello, world!".replace("world", "mum");  # Hello, mum!
"Hello, world!".count("l");               # 3
"Hello, world!".find("o");                # 4
"Hello, world!".index("w");               # 7
```

Formatting:

```slo
var name = "Elliot";
println("Your name is ${name}");
println("Calc: ${5 * 5}");
```

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

### files

Support for basic file operations:

```slo
var path = "/some/path/to/file";

var f = open(path);
print(f);  # should show file is open
var contents = f.read();
print(contents);
f.close();
print(f);  # should show file is closed

var lines = ["Line 1", "Line 2", "Line 3"];
var f = open("/tmp/text.txt", "w");
f.writelines(lines);
f.close();
```
