// a fully polymorphic binary tree with traversal and utility procedures
package tree

import "core:container/queue"
import "core:fmt"
import "core:log"

@(private)
void :: union {}

// generic tree structure, can hold any data type, to create use:
//
//      make_node(data)
//
// to free recursivley use:
//
//      destroy_node(^node(T))
node :: struct(T: typeid) {
	data:   T,
	parent: ^node(T),
	left:   ^node(T),
	right:  ^node(T),
}

// creates and allocated a new node on the heap, you need to use:
//
//      destroy_node(^node(T))
//
// to free that node
@(require_results)
make_node :: proc(data: $T) -> ^node(T) {
	n := new(node(T))
	n.data = data
	return n
}

// deallocates all nodes below the one passed in
destroy_node :: proc(node: ^node($T)) {
	if node == nil {return}

	destroy_node(node.left)
	destroy_node(node.right)
	free(node)
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =- C O N S T R U C T I O N   F U N C T I O N S -=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// assigns data to the left child, creating it, if a child with a full
// left node already exists, it creates a left child of it and assigns the data there
assign_left :: proc(parent: ^node($T), data: T) {
	if parent.left != nil {
		assign_left(parent.left, data)
	} else {
		parent.left = make_node(data)
		parent.left.parent = parent
	}
}

// force sets the data on the left child, if the child does not exist it is created
set_left :: proc(parent: ^node($T), data: T) {
	if parent.left != nil {
		parent.left.data = data
	} else {
		parent.left = make_node(data)
		parent.left.parent = parent
	}
}

// assigns data to the right child, creating it, if a child with a full
// right node already exists, it creates a right child of it and assigns the data there
assign_right :: proc(parent: ^node($T), data: T) {
	if parent.right != nil {
		assign_right(parent.right, data)
	} else {
		parent.right = make_node(data)
		parent.right.parent = parent
	}
}

// force sets the data on the right child, if the child does not exist it is created
set_right :: proc(parent: ^node($T), data: T) {
	if parent.right != nil {
		parent.right.data = data
	} else {
		parent.right = make_node(data)
		parent.right.parent = parent
	}
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-     M O V E M E N T   F U N C T I O N S     -=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

get_left :: proc(n: ^node($T)) -> (left: ^node(T), moved: bool) {
	if n.left != nil {
		left = n.left
		return left, true
	}
	return
}

get_right :: proc(n: ^node($T)) -> (right: ^node(T), moved: bool) {
	if n.right != nil {
		right = n.right
		return right, true
	}
	return
}

get_up :: proc(n: ^node($T)) -> (up: ^node(T), moved: bool) {
	if n.parent != nil {
		up = n.parent
		return up, true
	}
	return
}

get_root :: proc(n: ^node($T)) -> (root: ^node(T)) {
	if n == nil {return nil}
	root = n
	for root.parent != nil {
		root = root.parent
	}
	return
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-         D F S   T R A V E R S A L S         -=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

pre_order :: proc {
	pre_order_data_no_context,
	pre_order_data_context,
	pre_order_node_no_context,
	pre_order_node_context,
}

pre_order_data_no_context :: proc(node: ^node($T), visit: proc(data: T) -> (stop: bool)) {
	v := visit
	pre_order_data_context(
		node,
		proc(data: T, ctx: ^proc(data: T) -> (stop: bool)) -> (stop: bool) {
			return ctx^(data)
		},
		&v,
	)
}

pre_order_data_context :: proc(
	node: ^node($T),
	visit: proc(data: T, ctx: ^$E = nil) -> (stop: bool),
	ctx: ^E = nil,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if visit(node.data, ctx) {return true}
	if pre_order(node.left, visit, ctx) {return true}
	if pre_order(node.right, visit, ctx) {return true}
	return
}

pre_order_node_no_context :: proc(node: ^node($T), visit: proc(node: ^node(T)) -> (stop: bool)) {
	v := visit
	pre_order_node_context(
		node,
		proc(node: ^node(T), ctx: ^proc(node: ^node(T)) -> (stop: bool)) -> (stop: bool) {
			return ctx^(node)
		},
		&v,
	)
}

pre_order_node_context :: proc(
	node: ^node($T),
	visit: proc(node: ^node(T), ctx: ^$E = nil) -> (stop: bool),
	ctx: ^E = nil,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if visit(node, ctx) {return true}
	if pre_order(node.left, visit, ctx) {return true}
	if pre_order(node.right, visit, ctx) {return true}
	return
}

in_order :: proc {
	in_order_data_no_context,
	in_order_data_context,
	in_order_node_no_context,
	in_order_node_context,
}

in_order_data_no_context :: proc(node: ^node($T), visit: proc(data: T) -> (stop: bool)) {
	v := visit
	in_order_data_context(node, proc(data: T, ctx: ^proc(data: T) -> (stop: bool)) -> (stop: bool) {
			return ctx^(data)
		}, &v)
}

in_order_data_context :: proc(
	node: ^node($T),
	visit: proc(data: T, ctx: ^$E) -> (stop: bool),
	ctx: ^E,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if in_order(node.left, visit, ctx) {return true}
	if visit(node.data, ctx) {return true}
	if in_order(node.right, visit, ctx) {return true}
	return
}

in_order_node_no_context :: proc(node: ^node($T), visit: proc(node: ^node(T)) -> (stop: bool)) {
	v := visit
	in_order_node_context(
		node,
		proc(node: ^node(T), ctx: ^proc(node: ^node(T)) -> (stop: bool)) -> (stop: bool) {
			return ctx^(node)
		},
		&v,
	)
}

in_order_node_context :: proc(
	node: ^node($T),
	visit: proc(node: ^node(T), ctx: ^$E) -> (stop: bool),
	ctx: ^E,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if in_order(node.left, visit, ctx) {return true}
	if visit(node, ctx) {return true}
	if in_order(node.right, visit, ctx) {return true}
	return
}

post_order :: proc {
	post_order_data_no_context,
	post_order_data_context,
	post_order_node_no_context,
	post_order_node_context,
}

post_order_data_no_context :: proc(node: ^node($T), visit: proc(data: T) -> (stop: bool)) {
	v := visit
	post_order_data_context(
		node,
		proc(data: T, ctx: ^proc(data: T) -> (stop: bool)) -> (stop: bool) {
			return ctx^(data)
		},
		&v,
	)
}

post_order_data_context :: proc(
	node: ^node($T),
	visit: proc(data: T, ctx: ^$E) -> (stop: bool),
	ctx: ^E,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if post_order(node.left, visit, ctx) {return true}
	if post_order(node.right, visit, ctx) {return true}
	if visit(node.data, ctx) {return true}
	return
}

post_order_node_no_context :: proc(node: ^node($T), visit: proc(node: ^node(T)) -> (stop: bool)) {
	v := visit
	post_order_node_context(
		node,
		proc(node: ^node(T), ctx: ^proc(node: ^node(T)) -> (stop: bool)) -> (stop: bool) {
			return ctx^(node)
		},
		&v,
	)
}

post_order_node_context :: proc(
	node: ^node($T),
	visit: proc(node: ^node(T), ctx: ^$E) -> (stop: bool),
	ctx: ^E,
) -> (
	early_stop: bool,
) {
	if node == nil {return}
	if post_order(node.left, visit, ctx) {return true}
	if post_order(node.right, visit, ctx) {return true}
	if visit(node, ctx) {return true}
	return
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-         B F S   T R A V E R S A L S         -=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

level_order :: proc {
	level_order_data_no_context,
	level_order_data_context,
	level_order_node_no_context,
	level_order_node_context,
}

level_order_data_no_context :: proc(root: ^node($T), visit: proc(data: T) -> (stop: bool)) {
	v := visit
	level_order_data_context(
		root,
		proc(data: T, ctx: ^proc(data: T) -> (stop: bool)) -> (stop: bool) {
			return ctx^(data)
		},
		&v,
	)
}

level_order_data_context :: proc(
	root: ^node($T),
	visit: proc(data: T, ctx: ^$E = nil) -> (stop: bool),
	ctx: ^E = nil,
) {
	if root == nil {return}

	q: queue.Queue(^node(T))
	queue.init(&q)
	defer queue.destroy(&q)

	queue.push(&q, root)

	for queue.len(q) != 0 {
		curr := queue.pop_front(&q)
		if visit(curr.data, ctx) {return}
		if curr.left != nil {
			queue.push(&q, curr.left)
		}
		if curr.right != nil {
			queue.push(&q, curr.right)
		}
	}
}

level_order_node_no_context :: proc(root: ^node($T), visit: proc(node: ^node(T)) -> (stop: bool)) {
	v := visit
	level_order_node_context(
		root,
		proc(node: ^node(T), ctx: ^proc(node: ^node(T)) -> (stop: bool)) -> (stop: bool) {
			return ctx^(node)
		},
		&v,
	)
}

level_order_node_context :: proc(
	root: ^node($T),
	visit: proc(node: ^node(T), ctx: ^$E = nil) -> (stop: bool),
	ctx: ^E = nil,
) {
	if root == nil {return}

	q: queue.Queue(^node(T))
	queue.init(&q)
	defer queue.destroy(&q)

	queue.push(&q, root)

	for queue.len(q) != 0 {
		curr := queue.pop_front(&q)
		if visit(curr, ctx) {return}
		if curr.left != nil {
			queue.push(&q, curr.left)
		}
		if curr.right != nil {
			queue.push(&q, curr.right)
		}
	}
}

// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// =-              U T I L I T I E S              -=
// =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

@(private)
finder_ctx :: struct(T: typeid) {
	data: T,
	node: ^node(T),
}

// finds the node holding the specified data, only works for nodes(T) where T is comparable
find :: proc(search_root: ^node($T), data: T) -> (n: ^node(T)) {
	ctx := finder_ctx(T) {
		data = data,
		node = n,
	}

	finder := proc(search_node: ^node(T), ctx: ^finder_ctx(T)) -> (stop: bool) {
		if search_node.data == ctx.data {
			ctx.node = search_node
			return true
		}
		return
	}

	level_order(search_root, finder, &ctx)
	n = ctx.node

	return
}

size :: proc(root: ^node($T)) -> int {
	if root == nil {return 0}
	return 1 + size(root.left) + size(root.right)
}

height :: proc(root: ^node($T)) -> int {
	if root == nil {return -1}
	left_h := height(root.left)
	right_h := height(root.right)
	return 1 + max(left_h, right_h)
}
