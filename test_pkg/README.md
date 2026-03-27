# strutil — String Utilities for Omnikarai

String helper functions for everyday text manipulation.

## Install

```
omnip install strutil
```

## Functions

- `repeat(s, n)` — repeat string s n times
- `pad_left(s, n, ch)` — pad string to length n from left with char ch
- `pad_right(s, n, ch)` — pad string to length n from right with char ch
- `count(s, sub)` — count occurrences of sub in s
- `reverse(s)` — reverse a string
- `starts_with(s, prefix)` — returns 1 if s starts with prefix
- `ends_with(s, suffix)` — returns 1 if s ends with suffix
- `trim(s)` — remove leading and trailing spaces

## Usage

```omnikarai
use strutil

set r = strutil.repeat("ab", 3)
print(r)  # ababab

set p = strutil.pad_left("42", 6, "0")
print(p)  # 000042

set rev = strutil.reverse("hello")
print(rev)  # olleh

set n = strutil.count("banana", "a")
print(n)  # 3

if strutil.starts_with("omnikarai", "omni"):
    print("yes")
```

## Version History

### 1.0.0
- Initial release: repeat, pad_left, pad_right, count, reverse, starts_with, ends_with, trim
