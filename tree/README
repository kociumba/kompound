# the 🌲

This odin package contains a fully polymorphic binary tree written in pure odin. 

The tree itself is very simple and consists of a sincle `node(T)` struct, but there are quite a few utility and general convinience functions.

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