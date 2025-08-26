// odin bidirectional map - v1
//
// author: kociumba 
//
// this package provides a simple polymorphic bidirectional map implementation
// it's implemented using odin builtin maps, and follows odin design conventions
// as close as possible without being able to modify builtin functions
//
// the bimap is exclusive meaning that one entry can only map to one other entry,
// a non exclusive but non duplicating bidirectional map is also in the plans
package bimap

ALLOW_BIMAP_NEW :: #config(ALLOW_BIMAP_NEW, false) // change to enable the compilation of the new()creation functions, or use -define:ALLOW_BIMAP_NEW=true

// base bimap type, only kept in sync if not directly modified
//
// if modifying manually, need to relay on the sync functions to resync the bimap
bimap :: struct($left, $right: typeid) {
	left_to_right: map[left]right,
	right_to_left: map[right]left,
}

when ALLOW_BIMAP_NEW {

	@(require_results)
	// like new() in odin, creates and returns a pointer to the bimap
	new_bimap :: proc {
		new_bimap_empty,
		new_bimap_from_map,
	}

	new_bimap_empty :: proc($l: typeid, $r: typeid) -> (b: ^bimap(l, r)) {
		// b := &bimap(l, r){}
		b.left_to_right = make(map[l]r)
		b.right_to_left = make(map[r]l)
		return b
	}

	new_bimap_from_map :: proc(m: map[$left]$right) -> (b: ^bimap(left, right)) {
		// b := &bimap(left, right){}
		b.left_to_right = make(map[left]right)
		b.right_to_left = make(map[right]left)
		b.left_to_right = m
		for k, v in b.left_to_right {
			b.right_to_left[v] = k
		}
		return b
	}

}

@(require_results)
// like make() in odin, creates and returns the bimap by value
make_bimap :: proc {
	make_bimap_empty,
	make_bimap_from_map,
}

@(require_results)
make_bimap_empty :: proc($l: typeid, $r: typeid) -> (b: bimap(l, r)) {
	b.left_to_right = make(map[l]r)
	b.right_to_left = make(map[r]l)
	return
}

@(require_results)
// this is assumed as providing the left->right map
make_bimap_from_map :: proc(m: map[$left]$right) -> (b: bimap(left, right)) {
	b.left_to_right = make(map[left]right)
	b.right_to_left = make(map[right]left)
	b.left_to_right = m
	for k, v in b.left_to_right {
		b.right_to_left[v] = k
	}
	return
}

destroy_bimap :: proc(b: bimap($left, $right)) {
	delete(b.left_to_right)
	delete(b.right_to_left)
}

// destroys the right->left map and rebuilds it
sync_bimap_left :: proc(b: ^bimap($left, $right)) {
	clear(&b.right_to_left)
	for k, v in b.left_to_right {
		b.right_to_left[v] = k
	}
}

// destroys the left->right map and rebilds it
sync_bimap_right :: proc(b: ^bimap($left, $right)) {
	clear(&b.left_to_right)
	for k, v in b.right_to_left {
		b.left_to_right[v] = k
	}
}

insert :: proc(l: $left, r: $right, b: ^bimap(left, right)) -> bool {
	if l in b.left_to_right || r in b.right_to_left {
		return false
	}
	b.left_to_right[l] = r
	b.right_to_left[r] = l
	return true
}

insert_or_assign :: proc(l: $left, r: $right, b: ^bimap(left, right)) {
	erase_left(l, b)
	erase_right(r, b)

	b.left_to_right[l] = r
	b.right_to_left[r] = l
}

erase_left :: proc(l: $left, b: ^bimap(left, $right)) {
	_, v := delete_key(&b.left_to_right, l)
	delete_key(&b.right_to_left, v)
}

erase_right :: proc(r: $right, b: ^bimap($left, right)) {
	_, v := delete_key(&b.right_to_left, r)
	delete_key(&b.left_to_right, v)
}

clear_bimap :: proc(b: ^bimap($left, $right)) {
	clear(&b.left_to_right)
	clear(&b.right_to_left)
}

in_left :: proc(l: $left, b: ^bimap(left, $right)) -> bool {
	return l in b.left_to_right
}

in_right :: proc(r: $right, b: ^bimap($left, right)) -> bool {
	return r in b.right_to_left
}

at_left :: proc(l: $left, b: ^bimap(left, $right)) -> Maybe(right) {
	if l in b.left_to_right {
		return b.left_to_right[l]
	}
	return nil
}

at_right :: proc(r: $right, b: ^bimap($left, right)) -> Maybe(left) {
	if r in b.right_to_left {
		return b.right_to_left[r]
	}
	return nil
}

size :: proc(b: ^bimap($left, $right)) -> int {
	return len(b.left_to_right)
}
