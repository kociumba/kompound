package tree

import "core:testing"
import "core:fmt"
import "core:log"

@(test)
test_generic_tree :: proc(t: ^testing.T) {
	defer free_all(context.allocator)

	// Build a tree:
	//        root
	//       /    \
	//    left    right
	//      |
	//   left_2
	root := make_node("root")
	defer destroy_node(root)

	// Test assign_left and assign_right
	assign_left(root, "left")
	assign_left(root, "left_2") // should attach to left.left
	set_right(root, "right")
	set_right(root, "right_override") // should override right data

	testing.expect(t, root.left.data == "left", "assign_left did not set left child properly")
	testing.expect(t, root.left.left.data == "left_2", "nested assign_left did not go deeper")
	testing.expect(t, root.right.data == "right_override", "set_right did not override")

	// Test get_root
	left_most_leaf := root.left.left
	if left_most_leaf != nil {
		root_found := get_root(left_most_leaf)
		testing.expect(t, root_found == root, "get_root did not return actual root")
	}
	testing.expect(t, get_root(root) == root, "get_root on root node failed")

	tree_size := size(root)
	testing.expect(t, tree_size == 4, fmt.aprintf("expected size 4, got %d", tree_size))

	tree_height := height(root)
	testing.expect(t, tree_height == 2, fmt.aprintf("expected height 2, got %d", tree_height))

	found_l2 := find(root, "left_2")
	testing.expect(t, found_l2 != nil, "find_node failed to locate 'left_2'")
	if found_l2 != nil {
		testing.expect(t, found_l2.data == "left_2", "find_node returned wrong node for 'left_2'")
	}

	found_r := find(root, "right_override")
	testing.expect(t, found_r != nil, "find_node failed to locate 'right_override'")
	if found_r != nil {
		testing.expect(
			t,
			found_r.data == "right_override",
			"find_node returned wrong node for 'right_override'",
		)
	}

	found_blah := find(root, "blah")
	testing.expect(t, found_blah == nil, "find_node found a non-existent node 'blah'")

	find_root_ctx := finder_ctx(string) {
		data = "root",
	}
	pre_order(
		root,
		proc(n: ^node(string), ctx: ^finder_ctx(string)) -> (stop: bool) {
			log.infof("Pre-order visit: %s (target: %s)", n.data, ctx.data)
			if n.data == ctx.data {
				ctx.node = n
				return true // Stop
			}
			return false // Continue
		},
		&find_root_ctx,
	)
	testing.expect(t, find_root_ctx.node == root, "Pre-order find for 'root' failed")

	pre_order(
		root,
		proc(data: string) -> (stop: bool) {
			log.info("No-context pre_order_data visit: ", data)
			return // Never stop
		},
	)
}
