// a thread safe publisher/subscriber event system, that uses types as events
package event

import "core:fmt"
import "core:log"
import "core:sync"

// signature of the subscriber callback function
subscriber_callback :: union {
	subscriber_callback_no_return,
	subscriber_callback_return,
	subscriber_callback_pure_no_return,
	subscriber_callback_pure_return,
}

// this signature is for subscribers that do not care about signaling error states
subscriber_callback_no_return :: proc(data: any, ctx: rawptr)
// this signature allows for a boolean return, if this value is true the event_bus will become aware of an "error state"
subscriber_callback_return :: proc(data: any, ctx: rawptr) -> (report_error: bool)
// "pure" callback version, they have no side-effects for functional programming, this means no context passed to them
subscriber_callback_pure_no_return :: proc(data: any)
subscriber_callback_pure_return :: proc(data: any) -> (report_error: bool)

// holds a registered subscriber
subscriber :: struct {
	callback: subscriber_callback,
	ctx:      rawptr,
	id:       int,
}

// the main type of the `event` package, to create use:
// 
//      bus := create_event_bus()
//
// to deallocate use:
//
//      destroy_event_bus(bus)
event_bus :: struct {
	mu:          sync.Mutex,
	id_counter:  int,
	subscribers: map[typeid][dynamic]subscriber,
	options:     event_bus_flags,
}

event_bus_flags :: bit_set[event_bus_opts]
event_bus_opts :: enum {
	// integrates with core:log to log debugging info about event dispatch, requires you to set the default context logger
	DEBUG_LOGGING,
}

// holds possible errors operations on an event bus might return
event_error :: enum {
	// the event requested was not registered in this event bus
	UNREGISTERED_EVENT_ERROR,
	// signifies an error that occured while doing some operations on registered subscribers
	INTERNAL_SUBSCRIBER_MANAGEMENT_ERROR,
	// the event bus passed can not be used by the procedure, this typically means the event bus is corrupted
	INVALID_BUS_ERROR,
	// returned if a context is registered along side a pure (no context) subscriber
	NON_NIL_CONTEXT_FOR_A_PURE_SUBSCIBER_ERROR,
}

// used to create a new event bus and allocate it on the heap
create_event_bus :: proc(opts: event_bus_flags = nil) -> ^event_bus {
	bus := new(event_bus)
	bus.subscribers = make(map[typeid][dynamic]subscriber)
	bus.mu = sync.Mutex{}
	bus.options = opts
	return bus
}

// used to destroy an event bus allocated on the heap
destroy_event_bus :: proc(bus: ^event_bus) {
	delete(bus.subscribers)
	free(bus)
}

// subscribes to an event using a callback function with context, to subscribe a pure function with no context pass nil as ctx
subscribe :: proc(
	bus: ^event_bus,
	callback: subscriber_callback,
	ctx: ^$T,
	event: typeid,
) -> (
	sub_id: int,
	err: Maybe(event_error),
) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)

	if (type_of(callback) == subscriber_callback_pure_no_return ||
		   type_of(callback) == subscriber_callback_pure_return) &&
	   ctx != nil {
		return -1, .NON_NIL_CONTEXT_FOR_A_PURE_SUBSCIBER_ERROR
	}

	sub := subscriber{}
	sub.callback = callback
	sub.ctx = ctx

	// make the subscriber array if it's a new event
	if event not_in bus.subscribers {
		bus.subscribers[event] = make([dynamic]subscriber)
	}

	sync.atomic_add(&bus.id_counter, 1)
	sub.id = bus.id_counter
	sub_id = sub.id

	append(&bus.subscribers[event], sub)
	if .DEBUG_LOGGING in bus.options {
		log.infof(
			"subscriber: %#v, successfully registered on event: %#v, with context: %#v",
			sub_id,
			event,
			typeid_of(T),
		)
	}
	return
}

// unsubscribes a specific subscriber from a specific event
unsubscribe_adv :: proc(
	bus: ^event_bus,
	callback: subscriber_callback,
	ctx: ^$T,
	event: typeid,
) -> (
	err: Maybe(event_error),
) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)
    id: int

	if event not_in bus.subscribers {return .UNREGISTERED_EVENT_ERROR}
	subs := bus.subscribers[event]
	for sub, i in subs {
		if sub.callback == callback && sub.ctx == ctx {
            id = sub.id
			if !remove_at(
				&bus.subscribers[event],
				i,
			) {return .INTERNAL_SUBSCRIBER_MANAGEMENT_ERROR}
			break
		}
	}

	if .DEBUG_LOGGING in bus.options {
		log.infof(
			"subscriber: %#v with context: %#v, successfully removed from event: %#v",
			id,
			typeid_of(T),
			event,
		)
	}

	return
}

unsubscribe :: proc(bus: ^event_bus, sub_id: int) -> (err: Maybe(event_error)) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)
	e: typeid

	outer: for event, subs in bus.subscribers {
		for sub, i in subs {
			if sub.id == sub_id {
				if !remove_at(
					&bus.subscribers[event],
					i,
				) {return .INTERNAL_SUBSCRIBER_MANAGEMENT_ERROR}
				e = event
				break outer
			}
		}
	}

	if .DEBUG_LOGGING in bus.options {
		log.infof("subscriber with id: %#v, successfully removed from event: %#v", sub_id, e)
	}

	return
}

unsubscribe_all :: proc(bus: ^event_bus) -> (err: Maybe(event_error)) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)

	for event in bus.subscribers {
		delete_key(&bus.subscribers, event)
	}

	return
}

remove_at :: proc(arr: ^[dynamic]$T, index: int) -> (ok: bool) {
	if !(index >= 0 && index < len(arr)) {return}

	for i := index; i < len(arr) - 1; i += 1 {
		(arr)[i] = (arr)[i + 1]
	}

	resize(arr, len(arr) - 1)
	return true
}

// removes an event from the event bus and unregisters all subscribers currently subscribed to it
remove_event :: proc(bus: ^event_bus, event: typeid) -> (err: Maybe(event_error)) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)

	if event not_in bus.subscribers {return .UNREGISTERED_EVENT_ERROR}
	k, v := delete_key(&bus.subscribers, event)

	if .DEBUG_LOGGING in bus.options {
		log.infof("removed %#v subscribers from event: %#v", len(v), k)
	}

	return
}

// publishes an event, since events are types, you have to publish an instance of a previously registered event type
publish :: proc(bus: ^event_bus, data: $T) -> (err: Maybe(event_error)) {
	sync.lock(&bus.mu)

	if typeid_of(T) not_in bus.subscribers {
		sync.unlock(&bus.mu)
		return .UNREGISTERED_EVENT_ERROR
	}
	subs := bus.subscribers[typeid_of(T)]

	sync.unlock(&bus.mu)

	return internal_publish(bus, subs, data)
}

// simple wrapper to publish multiple events at onece, stops publishing when an error occurs 
publish_many :: proc(bus: ^event_bus, data: ..$T) -> (err: Maybe(event_error)) {
	sync.lock(&bus.mu)

	if typeid_of(T) not_in bus.subscribers {
		sync.unlock(&bus.mu)
		return .UNREGISTERED_EVENT_ERROR
	}
	subs := bus.subscribers[typeid_of(T)]

	sync.unlock(&bus.mu)

	for event in data {
		if err := internal_publish(bus, subs, event); err != nil {return err}
	}
}

@(private)
internal_publish :: proc(
	bus: ^event_bus,
	subs: [dynamic]subscriber,
	data: $T,
) -> (
	err: Maybe(event_error),
) {
	for sub in subs {
		err: bool
		switch callback in sub.callback {
		case subscriber_callback_no_return:
			callback(data, sub.ctx)
		case subscriber_callback_return:
			err = callback(data, sub.ctx)
        case subscriber_callback_pure_no_return:
            callback(data)
        case subscriber_callback_pure_return:
            err = callback(data)
		}

		if .DEBUG_LOGGING in bus.options && err == false {
			log.infof("dispatched subscriber: %#v for event: %#v", sub.id, typeid_of(T))
		} else if .DEBUG_LOGGING in bus.options && err == true {
			log.errorf(
				"subscriber %#v for event %#v, repoted and error state during dispatch",
				sub.id,
				typeid_of(T),
			)
		}
	}
	return
}

// returns an array of registered event typeids
get_registered_events :: proc(bus: ^event_bus) -> (events: [dynamic]typeid) {
	sync.lock(&bus.mu)
	defer sync.unlock(&bus.mu)

	for event in bus.subscribers {
		append(&events, event)
	}

	return
}
