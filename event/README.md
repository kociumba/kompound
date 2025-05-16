# event 📆

This odin package contains a thread safe publisher/subscriber event system written in pure odin.

The core philosophy is that types are events, this means you subscribe to a `typeid_of(type)` and publish events with an instance of that type.

Since odin has non-capturing closures, a context is registered with a subscriber which allows the subscriber to transfer whatever state it needs between scopes

> [!CAUTION]
> Again the main data gets allocated on the heap to allow global use, it has to be freed manually or using `destroy_event_bus(bus)`

Simple usage:

```odin
import "event"
import "core:log"

event_struct :: struct {
    data: string
}

main :: proc() {
    bus := event.create_event_bus()
    defer event.destroy_event_bus(bus)

    subsciber_context :: struct {
        call_count: int
    }

    // if you need to persist satate throught scope changes, use a heap allocated context
    ctx := subscriber_context{
        call_count = 0
    }

    // a subscriber has to match this signature
    subscriber :: proc(data: any, ctx_ptr: rawptr) {
        event, ok := data.(event_struct)
        if !ok {/* generally this should never happend but you should still check */}

        ctx := (^subscriber_context)(ctx_ptr)
        // use your context and perform subscriber work
    }

    id := subscribe(bus, subscriber, &ctx, typeid_of(event_struct))

    err := publish(bus, event_struct{data = "gabagool"})
    if err != nil {log.error(err)}

    err = unsubscribe(bus, id)
    if err != nil {log.error(err)}
}
```
