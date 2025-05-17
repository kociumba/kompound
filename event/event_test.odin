package event

import "core:testing"
import "core:fmt"
import "core:log"

@(test)
test_event_dispatch :: proc(t: ^testing.T) {
	defer free_all(context.allocator)

	// bus := create_event_bus({.DEBUG_LOGGING})
    bus := create_event_bus()
	defer destroy_event_bus(bus)

	// Define event types
	test_event_a :: struct {
		message: string,
	}

	test_event_b :: struct {
		value: int,
		id:    string,
	}

	test_event_c :: struct {
		flag: bool,
	}

	// Context for tracking handler calls
	handler_context :: struct {
		call_count:   int,
		prefix:       string,
		last_message: string,
		last_value:   int,
		last_id:      string,
		last_flag:    bool,
	}

	// Create handler contexts
	ctx_a1 := handler_context{}
	// defer free(ctx_a1)
	ctx_a1 = handler_context {
		prefix = "handler_a1",
	}

	ctx_a2 := new(handler_context)
	defer free(ctx_a2)
	ctx_a2^ = handler_context {
		prefix = "handler_a2",
	}

	ctx_b := new(handler_context)
	defer free(ctx_b)
	ctx_b^ = handler_context {
		prefix = "handler_b",
	}

	ctx_c := new(handler_context)
	defer free(ctx_c)
	ctx_c^ = handler_context {
		prefix = "handler_c",
	}

	// Handler functions
	handle_test_event_a :: proc(data: any, ctx_raw: rawptr) -> (err: bool) {
		event_data, ok := data.(test_event_a)
		if !ok {
			fmt.eprintln("Error: handle_test_event_a received unexpected data type")
			return
		}

		ctx := (^handler_context)(ctx_raw)
		ctx.call_count += 1
		ctx.last_message = event_data.message

		log.infof(
			"[%s|Call %d] test_event_a received: %s",
			ctx.prefix,
			ctx.call_count,
			event_data.message,
		)

        // if ctx.call_count == 2 { // tested but needs to be disabled since log.error is considered as a test fail
        //     err = true
        // }

        return
	}

	handle_test_event_b :: proc(data: any, ctx_raw: rawptr) {
		event_data, ok := data.(test_event_b)
		if !ok {
			fmt.eprintln("Error: handle_test_event_b received unexpected data type")
			return
		}

		ctx := (^handler_context)(ctx_raw)
		ctx.call_count += 1
		ctx.last_value = event_data.value
		ctx.last_id = event_data.id

		log.infof(
			"[%s|Call %d] test_event_b received: ID=%s, Value=%d",
			ctx.prefix,
			ctx.call_count,
			event_data.id,
			event_data.value,
		)
	}

	handle_test_event_c :: proc(data: any, ctx_raw: rawptr) {
		event_data, ok := data.(test_event_c)
		if !ok {
			fmt.eprintln("Error: handle_test_event_c received unexpected data type")
			return
		}

		ctx := (^handler_context)(ctx_raw)
		ctx.call_count += 1
		ctx.last_flag = event_data.flag

		log.infof(
			"[%s|Call %d] test_event_c received: Flag=%t",
			ctx.prefix,
			ctx.call_count,
			event_data.flag,
		)
	}

	// Test Case 1: Basic subscription and publishing
	subscribe(bus, handle_test_event_a, &ctx_a1, typeid_of(test_event_a))
	subscribe(bus, handle_test_event_a, ctx_a2, typeid_of(test_event_a))
	subscribe(bus, handle_test_event_b, ctx_b, typeid_of(test_event_b))

	log.info(get_registered_events(bus))

	// Publish events
	err := publish(bus, test_event_a{"Hello Odin!"})
	testing.expect(t, err == nil, "Expected no error when publishing test_event_a")

	err = publish(bus, test_event_b{value = 123, id = "test_id"})
	testing.expect(t, err == nil, "Expected no error when publishing test_event_b")

	// Verify handler execution
	testing.expect(
		t,
		ctx_a1.call_count == 1,
		fmt.aprintf("handler_a1 call_count was %d, expected 1", ctx_a1.call_count),
	)
	testing.expect(
		t,
		ctx_a2.call_count == 1,
		fmt.aprintf("handler_a2 call_count was %d, expected 1", ctx_a2.call_count),
	)
	testing.expect(
		t,
		ctx_b.call_count == 1,
		fmt.aprintf("handler_b call_count was %d, expected 1", ctx_b.call_count),
	)

	testing.expect(
		t,
		ctx_a1.last_message == "Hello Odin!",
		fmt.aprintf(
			"handler_a1 last_message was '%s', expected 'Hello Odin!'",
			ctx_a1.last_message,
		),
	)
	testing.expect(
		t,
		ctx_b.last_value == 123,
		fmt.aprintf("handler_b last_value was %d, expected 123", ctx_b.last_value),
	)
	testing.expect(
		t,
		ctx_b.last_id == "test_id",
		fmt.aprintf("handler_b last_id was '%s', expected 'test_id'", ctx_b.last_id),
	)

    log.infof("%#v", bus)

	// Test Case 2: Unsubscribe
	err = unsubscribe_adv(bus, handle_test_event_a, ctx_a2, typeid_of(test_event_a))
	testing.expect(t, err == nil, "Expected no error when unsubscribing handler_a2")

	// Publish again after unsubscribing
	err = publish(bus, test_event_a{"Event bus is cool."})
	testing.expect(t, err == nil, "Expected no error when publishing test_event_a again")

	// Verify that ctx_a1 got the update but ctx_a2 didn't
	testing.expect(
		t,
		ctx_a1.call_count == 2,
		fmt.aprintf("handler_a1 call_count was %d, expected 2", ctx_a1.call_count),
	)
	testing.expect(
		t,
		ctx_a2.call_count == 1,
		fmt.aprintf("handler_a2 call_count was %d, expected 1", ctx_a2.call_count),
	)
	testing.expect(
		t,
		ctx_a1.last_message == "Event bus is cool.",
		fmt.aprintf(
			"handler_a1 last_message was '%s', expected 'Event bus is cool.'",
			ctx_a1.last_message,
		),
	)

	// Test Case 3: Unregistered event
	err = publish(bus, test_event_c{flag = true})
	testing.expect(
		t,
		err == .UNREGISTERED_EVENT_ERROR,
		"Expected UNREGISTERED_EVENT_ERROR when publishing unregistered event",
	)

	// Test Case 4: Register and then remove an event
	subscribe(bus, handle_test_event_c, ctx_c, typeid_of(test_event_c))

	err = publish(bus, test_event_c{flag = true})
	testing.expect(
		t,
		err == nil,
		"Expected no error when publishing test_event_c after registration",
	)
	testing.expect(
		t,
		ctx_c.call_count == 1,
		fmt.aprintf("handler_c call_count was %d, expected 1", ctx_c.call_count),
	)

	err = remove_event(bus, typeid_of(test_event_c))
	testing.expect(t, err == nil, "Expected no error when removing test_event_c")

	err = publish(bus, test_event_c{flag = false})
	testing.expect(
		t,
		err == .UNREGISTERED_EVENT_ERROR,
		"Expected UNREGISTERED_EVENT_ERROR after removing event",
	)
	testing.expect(
		t,
		ctx_c.call_count == 1,
		fmt.aprintf(
			"handler_c call_count should still be 1 after event removal, was %d",
			ctx_c.call_count,
		),
	)

	// Test Case 5: Trying to unsubscribe from non-existent event
	err = unsubscribe_adv(bus, handle_test_event_c, ctx_c, typeid_of(test_event_c))
	testing.expect(
		t,
		err == .UNREGISTERED_EVENT_ERROR,
		"Expected UNREGISTERED_EVENT_ERROR when unsubscribing from removed event",
	)
}
