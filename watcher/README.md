# watcher.h

This is a single header file watcher api written in c99 and compatible with c and c++

It works exactly the same as any other single header lib, define the implementation macro
`WATCHER_IMPLEMENTATION` and include the header `watcher.h`

Since this is meant to be copy pasted into your codebase most documentation is in the library itself

The opening comment is an overview, and most functions and structures have doc comments

Simple usage:

```c
    #define WATCHER_IMPLEMENTATION
    #include "watcher.h"
    #include <stdio.h>

    void on_change(const char* path, void* ud) {
        (void)ud;
        printf("changed: %s\n", path);
    }

    int main(void) {
        Watcher* w = watcher_watch(".", on_change, NULL);
        if (!w) return 1;

        int running = 10;
        while (running) {
            watcher_poll();
            running--;
        }

        watcher_stop(w);
        watcher_cleanup();
        return 0;
    }
```
