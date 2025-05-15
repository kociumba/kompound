# the 🌲

This odin package contains a fully polymorphic binary tree written in pure odin. 

The tree itself is very simple and consists of a single `node(T)` struct, but there are quite a few utility and general convenience functions.

> [!CAUTION]
> The tree is allocated on the heap, so not calling `destroy_node` will leak memory, but this also means that a tree can be global.

Simple usage looks something like:

```odin
import "tree"
import "core:fmt"

main :: proc() {
    // the entire tree will inherit the data type used to create root
    root := tree.make_node(data_for_root)
    defer tree.destroy_node(root)

    tree.assign_left(root, data_for_left_branch) // types are required to match the tree

    fmt.println(root.left.data)
}
```