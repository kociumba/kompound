# bimap ⇆

This package implements the rarely used bidirectional map data structure, it's not always convenient to manage bidirectional mappings in separate maps, and this makes that easy.

> [!NOTE]
> This bidirectional map is exclusive meaning that one entry can only ever map to one other entry

To create a bidirectional map use `make_bimap()`, this can take in 2 types or a map literal, if using 2 types an empty bimap allocated using those types is returned, if creating from an already existing map, the returned bimap will contain the entries in the parent map.

If you need functions like the builtin `new()`, you can use `-define:ALLOW_BIMAP_NEW=true`, but those functions are experimental and cause some undiagnosed issues in some enviornments.

example usage:

```odin
#+feature dynamic-literals
package example

import "bimap"
import "core:fmt"

main :: proc() {
    b := bimap.make_bimap(map[int]string{1 = "gabagool", 2 = "gleep"})
    defer bimap.destroy_bimap(b)

    fmt.printfln("%#v", bimap.at_left(1, &b)) // prints: "gabagool" 
    fmt.printfln("%#v", bimap.at_right("gleep", &b)) // prints: 2
}
```

As always for more usage take a look at the package tests.