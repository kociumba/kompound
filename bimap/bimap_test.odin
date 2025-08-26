#+feature dynamic-literals

package bimap

import "core:testing"

@(test)
test_make_bimap_empty :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	testing.expect(t, len(b.left_to_right) == 0, "Empty bimap left_to_right should have length 0")
	testing.expect(t, len(b.right_to_left) == 0, "Empty bimap right_to_left should have length 0")
	testing.expect(t, size(&b) == 0, "Empty bimap size should be 0")
}

@(test)
test_make_bimap_from_map :: proc(t: ^testing.T) {
	b := make_bimap_from_map(map[int]string{1 = "one", 2 = "two"})
	defer destroy_bimap(b)

	testing.expect(t, len(b.left_to_right) == 2, "Bimap left_to_right should have length 2")
	testing.expect(t, len(b.right_to_left) == 2, "Bimap right_to_left should have length 2")
	testing.expect(t, b.left_to_right[1] == "one", "Left to right mapping incorrect for key 1")
	testing.expect(
		t,
		b.right_to_left["one"] == 1,
		"Right to left mapping incorrect for value 'one'",
	)
	testing.expect(t, size(&b) == 2, "Bimap size should be 2")
}

@(test)
test_insert :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	success := insert(1, "one", &b)
	testing.expect(t, success, "Insert should succeed for new key-value pair")
	testing.expect(t, in_left(1, &b), "Key 1 should exist in left_to_right")
	testing.expect(t, in_right("one", &b), "Value 'one' should exist in right_to_left")
	testing.expect(t, at_left(1, &b) == "one", "at_left(1) should return 'one'")
	testing.expect(t, at_right("one", &b) == 1, "at_right('one') should return 1")

	success = insert(1, "two", &b)
	testing.expect(t, !success, "Insert should fail for duplicate key")
}

@(test)
test_insert_or_assign :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	insert(1, "one", &b)
	insert_or_assign(1, "new_one", &b)

	testing.expect(
		t,
		at_left(1, &b) == "new_one",
		"insert_or_assign should update value for key 1",
	)
	testing.expect(
		t,
		at_right("new_one", &b) == 1,
		"insert_or_assign should update right_to_left mapping",
	)
	testing.expect(t, !in_right("one", &b), "Old value 'one' should no longer exist")
}

@(test)
test_erase :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	insert(1, "one", &b)
	erase_left(1, &b)

	testing.expect(t, !in_left(1, &b), "Key 1 should be erased from left_to_right")
	testing.expect(t, !in_right("one", &b), "Value 'one' should be erased from right_to_left")

	insert(2, "two", &b)
	erase_right("two", &b)

	testing.expect(t, !in_right("two", &b), "Value 'two' should be erased from right_to_left")
	testing.expect(t, !in_left(2, &b), "Key 2 should be erased from left_to_right")
}

@(test)
test_sync :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	insert(1, "one", &b)
	// Manually modify left_to_right to break sync
	b.left_to_right[2] = "two"

	sync_bimap_left(&b)
	testing.expect(t, in_right("two", &b), "sync_bimap_left should add 'two' to right_to_left")
	testing.expect(t, at_right("two", &b) == 2, "sync_bimap_left should map 'two' to 2")

	// Manually modify right_to_left to break sync
	b.right_to_left["three"] = 3
	sync_bimap_right(&b)
	testing.expect(t, in_left(3, &b), "sync_bimap_right should add 3 to left_to_right")
	testing.expect(t, at_left(3, &b) == "three", "sync_bimap_right should map 3 to 'three'")
}

@(test)
test_clear_bimap :: proc(t: ^testing.T) {
	b := make_bimap_empty(int, string)
	defer destroy_bimap(b)

	insert(1, "one", &b)
	insert(2, "two", &b)
	clear_bimap(&b)

	testing.expect(t, len(b.left_to_right) == 0, "clear_bimap should empty left_to_right")
	testing.expect(t, len(b.right_to_left) == 0, "clear_bimap should empty right_to_left")
	testing.expect(t, size(&b) == 0, "clear_bimap should set size to 0")
}
