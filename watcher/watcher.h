/* watcher.h - https://github.com/kociumba/kompound

A minimal, cross platform file/directory watcher header only C library
with optional C++ features. stdlib includes are kept to an absolute minimum.

SIMPLE EXAMPLE:
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

USAGE:
    Before using this library, define the implementation macro in exactly
    one C/C++ source file:

        #define WATCHER_IMPLEMENTATION
        #include "watcher.h"

    NOTE: It is recommended to compile the implementation file as C due to
        the use of `volatile`. Files that only include the header may be C++.

LIMITATIONS:
    Some functionality is intentionally limited to maintain consistency and
    simplicity across platforms. Notable unsupported features include:

        - Recursive directory watching
        - Granular event types

    MacOS is currently not implemented, using this library on macos will invoke
    placeholder code which might crash or simply do nothing

THREAD SAFETY:
    watcher.h provides a poll thread api, but it is more of a convenience and
    demonstration of the design than a fully fledged implementation

    The api is explicitly unsafe, and adding or removing watchers
    while the poll thread is running is likely to cause race conditions.
    All synchronization is the responsibility of the user.

REDEFINABLE MACROS:

    You can define these macros before including watcher.h to override some default behaviour

    - WATCHER_malloc: defines the malloc implementation watcher.h will use
    - WATCHER_free: defines the free implementation watcher.h will use
    - WATCHER_memset: defines the memset implementation watcher.h will use
    - WATCHER_memcpy: defines the memcpy implementation watcher.h will use
    - WATCHER_BUFFER_SIZE: defines the size of the event buffer in Watcher (needs to be DWORD aligned on windows)
    - WATCHER_POLL_THREAD_FREQ_MS: defines the time the thread poll sleeps between polling in ms
    - __bool_true_false_are_defined: checked to see if a custom definitions of bool and true/false exists
    - WATCHER_ERROR_PRINT(msg, ...): defines the function watcher.h uses to print error info, needs to handle printf formatting

TODO/ROADMAP:

    make the library safer - check all return values from system apis
*/

#ifndef WATCHER_H
#define WATCHER_H

#define WATCHER_VERSION 1  // bump to signal new features, detectable by downstream

#if !defined(WATCHER_malloc) || !defined(WATCHER_free) || !defined(WATCHER_memset) || \
    !defined(WATCHER_memcpy)
#include <stdlib.h>
#include <string.h>
#endif  // #if !defined(WATCHER_malloc) || !defined(WATCHER_free)

#if !defined(WATCHER_malloc)
#define WATCHER_malloc malloc
#endif  // #if !defined(WATCHER_malloc)

#if !defined(WATCHER_free)
#define WATCHER_free free
#endif  // #if !defined(WATCHER_free)

#if !defined(WATCHER_memset)
#define WATCHER_memset memset
#endif  // #if !defined(WATCHER_memset)

#if !defined(WATCHER_memcpy)
#define WATCHER_memcpy memcpy
#endif  // #if !defined(WATCHER_memcpy)

// increase if you plan on calling `watcher_poll` infrequently
#if !defined(WATCHER_BUFFER_SIZE)
#define WATCHER_BUFFER_SIZE (4 * 1024)
#endif  // #if !defined(WATCHER_BUFFER_SIZE)

// overwrite to set a custom timeout period on the poll thread in ms
#if !defined(WATCHER_POLL_THREAD_FREQ_MS)
#define WATCHER_POLL_THREAD_FREQ_MS (50)
#endif  // #if !defined(WATCHER_POLL_THREAD_FREQ_MS)

// allows using a custom implementation of bool and true/false
#if !defined(__bool_true_false_are_defined)
#include <stdbool.h>
#endif  // #if !defined(__bool_true_false_are_defined)

// overwrite to provide a custom error logging mechanism
// it is expected that it will handle printf formatting
#if !defined(WATCHER_ERROR_PRINT)
#include <stdio.h>
#define WATCHER_ERROR_PRINT(msg, ...) fprintf(stderr, msg, ##__VA_ARGS__)
#endif  // #if !defined(WATCHER_ERROR_PRINT)

// guard for no stdlib.h include
#if !defined(NULL)
#define NULL ((void*)0)
#endif

#if defined(__cplusplus)
#define WATCHER_volatile
#else
#define WATCHER_volatile volatile
#endif

// platform specific includes
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(__APPLE__)
#include <pthread.h>
#error "macos support is currntly not implemented in watcher.h"
#elif defined(__linux__)
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>
#else
#error "watcher.h does not support this platform"
#endif  // #if defined(_WIN32)

// definitions
#if defined(__cplusplus)
extern "C" {
#endif  // #if defined(__cplusplus)

/*
    Callback invoked when a watched file or directory changes.

    # Parameters
        - path: The path of the file/directory that triggered the event, a NULL value
            signals some missed/leaked events, in most cases you will want to re scan
            everything if this happens.
        - user_data: The opaque pointer originally passed to `watcher_watch()`.
*/
typedef void (*watcher_callback)(const char* path, void* user_data);

/*
    The core watcher type, the consumer does not need to manage this struct,
    but it is left in the public api for any specific cases you might have

    NOTE: This struct contains platform specific fields and fields that might change
        between versions, the two fields deliberately exposed to the user are:
            - watcher_callback cb: use to exchange the callback for this watcher on the fly
            - void* ud: use to exchange the user data for this watcher on the fly

        all other fields are mostly considered internal and using them may have
        unforeseen consequences
*/
typedef struct Watcher {
    int path_len;
    char* path;
    watcher_callback cb;
    void* ud;

    // platform specific data
#if defined(_WIN32)
    HANDLE hDir;
    OVERLAPPED overlapped;
    BYTE buffer[WATCHER_BUFFER_SIZE];
    DWORD bytesReturned;
    BOOL waiting;
#elif defined(__APPLE__)

#elif defined(__linux__)
    int inotify_fd;
    int watch_fd;
    char buffer[WATCHER_BUFFER_SIZE];
    int bytesReturned;
#endif  // #if defined(_WIN32)
} Watcher;

/*
    Frees all memory allocated for any watchers, invalidates all Watcher* pointers,
    and stops all watchers. If the poll thread is running, this will also stop it.

    Most of the time this should be called at the end of a program in the quit procedure.
    If not, be careful of invalid Watcher* pointers.
*/
extern void watcher_cleanup();

/*
    Begins watching a file or directory for changes.

    The watcher is managed by the library and will be freed when stopping the watch
    or quitting the library.

    # Parameters
        - path: The path of the file or directory to watch.
        - cb: The callback to invoke when a change is detected.
        - user_data: An opaque pointer passed through to the callback.

    # Returns
        A pointer to the created Watcher, or NULL on failure.
*/
extern Watcher* watcher_watch(const char* path, watcher_callback cb, void* user_data);

/*
    Stops a previously created watcher and frees its resources.

    # Parameters
        - watcher: The Watcher* returned by `watcher_watch()`.

    # Returns
        `true` if the watcher was found, stopped and freed, `false` otherwise.
*/
extern bool watcher_stop(Watcher* watcher);

/*
    Polls data from the os for watcher changes.

    This must be called periodically if not using the background poll thread.
*/
extern void watcher_poll();

/*
    Starts a background thread that calls `watcher_poll()` automatically.

    After calling this, you do not need to manually call `watcher_poll()`.
    Use `watcher_stop_poll_thread()` to shut it down.
*/
extern void watcher_run_poll_thread();

/*
    Stops the background poll thread started by `watcher_run_poll_thread()`.

    This blocks until the thread has fully exited.
*/
extern void watcher_stop_poll_thread();

#if defined(__cplusplus)
}
#endif  // #if defined(__cplusplus)

// c++ specific definitions
#if defined(__cplusplus)

#endif  // #if defined(__cplusplus)

// implementation
#if defined(WATCHER_IMPLEMENTATION)

static int watcher_strlen(const char* s) {
    const char* p = s;
    while (*p)
        p++;
    return p - s;
}

// internal implementations

typedef enum { ERR = -1, EMPTY = 0, FULL = 1 } PollResult;

// using ll here instead of da since we need stable pointers without any reallocation
typedef struct {
    Watcher w;
    void* prev;
    void* next;
} WatcherNode;

static WatcherNode* watcher_list;

static WatcherNode* create_node(const char* path, watcher_callback cb, void* user_data) {
    int path_len = watcher_strlen(path) + 1;
    WatcherNode* node = (WatcherNode*)WATCHER_malloc(sizeof(WatcherNode));
    if (!node) return NULL;

    WATCHER_memset(node, 0, sizeof(WatcherNode));
    node->w.path = (char*)WATCHER_malloc(path_len);
    if (!node->w.path) {
        WATCHER_free(node);
        return NULL;
    }

    WATCHER_memcpy(node->w.path, path, path_len);
    node->w.path_len = (int)(path_len - 1);
    node->w.cb = cb;
    node->w.ud = user_data;
    node->prev = NULL;
    node->next = NULL;

    return node;
}

static void destroy_node(WatcherNode* node) {
    if (!node) return;
    if (node->w.path) WATCHER_free(node->w.path);
    WATCHER_free(node);
}

static void append_watcher(WatcherNode* node) {
    if (!watcher_list) {
        watcher_list = node;
        return;
    }

    WatcherNode* head = watcher_list;
    while (head->next) {
        head = (WatcherNode*)head->next;
    }
    head->next = node;
    node->prev = head;
}

// c implementations
#if defined(_WIN32)

void watcher_platform_teardown(Watcher* w) {
    if (!w) return;

    if (w->hDir != INVALID_HANDLE_VALUE && w->hDir != NULL) {
        CancelIoEx(w->hDir, &w->overlapped);
        CloseHandle(w->hDir);
        w->hDir = INVALID_HANDLE_VALUE;
    }
    if (w->overlapped.hEvent) {
        CloseHandle(w->overlapped.hEvent);
        w->overlapped.hEvent = NULL;
    }
    w->waiting = FALSE;
}

bool watcher_platform_setup(Watcher* w) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, w->path, -1, NULL, 0);
    if (wlen == 0) return false;

    WCHAR* path_w = (WCHAR*)WATCHER_malloc(wlen * sizeof(WCHAR));
    if (!path_w) return false;

    MultiByteToWideChar(CP_UTF8, 0, w->path, -1, path_w, wlen);

    DWORD abs_len = GetFullPathNameW(path_w, 0, NULL, NULL);
    if (abs_len == 0) {
        WATCHER_free(path_w);
        return false;
    }

    WCHAR* abs_path_w = (WCHAR*)WATCHER_malloc(abs_len * sizeof(WCHAR));
    if (!abs_path_w) {
        WATCHER_free(path_w);
        return false;
    }

    if (!GetFullPathNameW(path_w, abs_len, abs_path_w, NULL)) {
        WATCHER_free(abs_path_w);
        WATCHER_free(path_w);
        return false;
    }

    WATCHER_free(path_w);
    path_w = abs_path_w;

    int abs_utf8_len = WideCharToMultiByte(CP_UTF8, 0, path_w, -1, NULL, 0, NULL, NULL);
    if (abs_utf8_len > 0) {
        char* abs_utf8 = (char*)WATCHER_malloc(abs_utf8_len);
        if (abs_utf8) {
            WideCharToMultiByte(CP_UTF8, 0, path_w, -1, abs_utf8, abs_utf8_len, NULL, NULL);
            WATCHER_free(w->path);
            w->path = abs_utf8;
            w->path_len = abs_utf8_len - 1;
        }
    }

    w->hDir = CreateFileW(path_w,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    WATCHER_free(path_w);

    if (w->hDir == INVALID_HANDLE_VALUE) {
        WATCHER_ERROR_PRINT("Failed to open path '%s': %lu\n", w->path, GetLastError());
        return false;
    }

    w->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!w->overlapped.hEvent) {
        CloseHandle(w->hDir);
        w->hDir = INVALID_HANDLE_VALUE;
        return false;
    }

    w->waiting = FALSE;
    return true;
}

bool watcher_platform_start_read(Watcher* w) {
    if (w->waiting) return true;

    DWORD filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                   FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
                   FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;

    BOOL ok = ReadDirectoryChangesW(w->hDir,
        w->buffer,
        WATCHER_BUFFER_SIZE,
        FALSE,
        filter,
        &w->bytesReturned,
        &w->overlapped,
        NULL);

    if (!ok && GetLastError() != ERROR_IO_PENDING) {
        WATCHER_ERROR_PRINT(
            "ReadDirectoryChangesW failed for '%s': %lu\n", w->path, GetLastError());
        return false;
    }

    w->waiting = TRUE;
    return true;
}

void watcher_platform_process_results(Watcher* w) {
    if (w->bytesReturned == 0) {
        if (w->cb) w->cb(NULL, w->ud);
        return;
    }

    FILE_NOTIFY_INFORMATION* info = (FILE_NOTIFY_INFORMATION*)w->buffer;
    while (true) {
        WCHAR* name = info->FileName;
        int nameLen = info->FileNameLength / sizeof(WCHAR);

        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, name, nameLen, NULL, 0, NULL, NULL);
        if (utf8Len > 0) {
            int need_sep = (w->path_len > 0 && w->path[w->path_len - 1] != '\\' &&
                               w->path[w->path_len - 1] != '/')
                               ? 1
                               : 0;
            int fullLen = w->path_len + need_sep + utf8Len;

            char* fullPath = (char*)WATCHER_malloc(fullLen + 1);
            if (fullPath) {
                WATCHER_memcpy(fullPath, w->path, w->path_len);
                int pos = w->path_len;
                if (need_sep) { fullPath[pos++] = '\\'; }
                WideCharToMultiByte(CP_UTF8, 0, name, nameLen, fullPath + pos, utf8Len, NULL, NULL);
                fullPath[fullLen] = '\0';

                if (w->cb) w->cb(fullPath, w->ud);

                WATCHER_free(fullPath);
            }
        }

        if (info->NextEntryOffset == 0) break;
        info = (FILE_NOTIFY_INFORMATION*)((BYTE*)info + info->NextEntryOffset);
    }
}

PollResult watcher_platform_poll_one(Watcher* w) {
    if (!w->hDir || w->hDir == INVALID_HANDLE_VALUE) {
        if (!watcher_platform_setup(w)) return ERR;
    }

    if (!w->waiting) {
        if (!watcher_platform_start_read(w)) return ERR;
    }

    DWORD bytes;
    BOOL ok = GetOverlappedResult(w->hDir, &w->overlapped, &bytes, FALSE);

    if (!ok) {
        DWORD err = GetLastError();
        if (err == ERROR_IO_INCOMPLETE) { return EMPTY; }
        WATCHER_ERROR_PRINT("GetOverlappedResult failed for '%s': %lu\n", w->path, err);
        w->waiting = FALSE;
        return EMPTY;
    }

    w->bytesReturned = bytes;
    w->waiting = FALSE;
    watcher_platform_process_results(w);
    return FULL;
}

#elif defined(__APPLE__)

void watcher_platform_teardown(Watcher* w) {
    (void)w;
    // TODO: macos implementation
}

void watcher_platform_process_results(Watcher* w) {
    (void)w;
    // TODO: macos implementation
}

PollResult watcher_platform_poll_one(Watcher* w) {
    (void)w;
    // TODO: macos implementation
    return FULL;
}

#elif defined(__linux__)

void watcher_platform_teardown(Watcher* w) {
    if (!w) return;
    if (w->watch_fd >= 0) {
        inotify_rm_watch(w->inotify_fd, w->watch_fd);
        w->watch_fd = -1;
    }

    if (w->inotify_fd >= 0) {
        close(w->inotify_fd);
        w->inotify_fd = -1;
    }

    w->watch_fd = -1;
}

bool watcher_platform_setup(Watcher* w) {
    w->inotify_fd = -1;
    w->watch_fd = -1;

    char* buf = (char*)WATCHER_malloc(PATH_MAX);
    char* abs_path = realpath(w->path, buf);
    if (!abs_path) {
        WATCHER_free(buf);
        WATCHER_ERROR_PRINT("realpath failed for %s : %d", w->path, errno);
        return false;
    }
    WATCHER_free(w->path);
    w->path = abs_path;
    w->path_len = watcher_strlen(w->path);

    w->inotify_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (w->inotify_fd < 0) {
        WATCHER_ERROR_PRINT("inotify_init1 failed for '%s': %d\n", w->path, errno);
        return false;
    }

    int mask = IN_MODIFY | IN_ATTRIB | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE |
               IN_DELETE_SELF | IN_MOVE_SELF;

    w->watch_fd = inotify_add_watch(w->inotify_fd, w->path, mask);
    if (w->watch_fd < 0) {
        WATCHER_ERROR_PRINT("inotify_add_watch failed for '%s': %d\n", w->path, errno);
        close(w->inotify_fd);
        w->inotify_fd = -1;
        return false;
    }

    w->bytesReturned = 0;
    return true;
}

void watcher_platform_process_results(Watcher* w) {
    if (w->bytesReturned <= 0) {
        if (w->cb) w->cb(NULL, w->ud);
        return;
    }

    size_t offset = 0;
    while (offset < (size_t)w->bytesReturned) {
        struct inotify_event* event = (struct inotify_event*)(w->buffer + offset);

        if (event->mask & IN_Q_OVERFLOW) {
            if (w->cb) w->cb(NULL, w->ud);
            break;  // it's empty either way so might as well break
        } else if (event->len > 0) {
            int nameLen = watcher_strlen(event->name);

            int need_sep = (w->path_len > 0 && w->path[w->path_len - 1] != '/' &&
                               w->path[w->path_len - 1] != '\\')
                               ? 1
                               : 0;
            int fullLen = w->path_len + need_sep + nameLen;

            char* fullPath = (char*)WATCHER_malloc(fullLen + 1);
            if (fullPath) {
                WATCHER_memcpy(fullPath, w->path, w->path_len);
                int pos = w->path_len;
                if (need_sep) { fullPath[pos++] = '/'; }
                WATCHER_memcpy(fullPath + pos, event->name, nameLen);
                fullPath[fullLen] = '\0';

                if (w->cb) w->cb(fullPath, w->ud);

                WATCHER_free(fullPath);
            }
        } else {
            if (w->cb) w->cb(w->path, w->ud);
        }

        offset += sizeof(struct inotify_event) + event->len;
    }
}

PollResult watcher_platform_poll_one(Watcher* w) {
    if (w->inotify_fd < 0) {
        if (!watcher_platform_setup(w)) return ERR;
    }

    ssize_t bytes = read(w->inotify_fd, w->buffer, WATCHER_BUFFER_SIZE);
    if (bytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return EMPTY; }
        WATCHER_ERROR_PRINT("read failed for '%s': %d\n", w->path, errno);
        return ERR;
    }

    if (bytes == 0) { return EMPTY; }

    w->bytesReturned = bytes;
    watcher_platform_process_results(w);
    return FULL;
}

#endif  // #if defined(_WIN32)

// thread api implementations
#if defined(_WIN32)
static HANDLE watcher_poll_thread = NULL;
static WATCHER_volatile bool watcher_poll_thread_running = false;

DWORD WINAPI watcher_poll_thread_proc(LPVOID lpParam) {
    (void)lpParam;
    while (watcher_poll_thread_running) {
        watcher_poll();
        Sleep(WATCHER_POLL_THREAD_FREQ_MS);
    }
    return 0;
}

void watcher_run_poll_thread() {
    if (watcher_poll_thread) return;

    watcher_poll_thread_running = true;

    watcher_poll_thread = CreateThread(NULL, 0, watcher_poll_thread_proc, NULL, 0, NULL);
}

void watcher_stop_poll_thread() {
    if (!watcher_poll_thread) return;

    watcher_poll_thread_running = false;

    WaitForSingleObject(watcher_poll_thread, INFINITE);
    CloseHandle(watcher_poll_thread);

    watcher_poll_thread = NULL;
}

#elif defined(__APPLE__) || defined(__linux__)

static pthread_t* watcher_poll_thread = NULL;
static WATCHER_volatile bool watcher_poll_thread_running = false;

static void* watcher_poll_thread_proc(void* arg) {
    (void)arg;
    while (watcher_poll_thread_running) {
        watcher_poll();
        struct timespec ts = {0, WATCHER_POLL_THREAD_FREQ_MS * 1000 * 1000};
        nanosleep(&ts, NULL);
    }
    return NULL;
}

void watcher_run_poll_thread() {
    if (watcher_poll_thread) return;

    watcher_poll_thread_running = true;

    pthread_t* thread = (pthread_t*)WATCHER_malloc(sizeof(pthread_t));
    if (thread) {
        pthread_create(thread, NULL, watcher_poll_thread_proc, NULL);
        watcher_poll_thread = thread;
    }
}

void watcher_stop_poll_thread() {
    if (!watcher_poll_thread) return;

    watcher_poll_thread_running = false;

    pthread_t* thread = watcher_poll_thread;
    pthread_join(*thread, NULL);
    WATCHER_free(thread);

    watcher_poll_thread = NULL;
}

#endif  // #if defined(_WIN32)

// api implementations
void watcher_cleanup() {
    watcher_stop_poll_thread();

    WatcherNode* head = watcher_list;
    if (watcher_list) {
        while (head) {
            WatcherNode* next = (WatcherNode*)head->next;
            watcher_platform_teardown(&head->w);
            destroy_node(head);
            head = next;
        }

        watcher_list = NULL;
    }
}

Watcher* watcher_watch(const char* path, watcher_callback cb, void* user_data) {
    if (!path || !cb) return NULL;

    WatcherNode* node = create_node(path, cb, user_data);
    if (!node) return NULL;

    if (!watcher_platform_setup(&node->w)) {
        destroy_node(node);
        return NULL;
    }

    append_watcher(node);
    return &node->w;
}

bool watcher_stop(Watcher* watcher) {
    if (!watcher_list || !watcher) return false;

    WatcherNode* head = watcher_list;
    while (head) {
        if (&head->w == watcher) {
            if (head == watcher_list) { watcher_list = (WatcherNode*)head->next; }
            if (head->prev) ((WatcherNode*)(head->prev))->next = head->next;
            if (head->next) ((WatcherNode*)(head->next))->prev = head->prev;

            watcher_platform_teardown(&head->w);
            destroy_node(head);
            return true;
        }
        head = (WatcherNode*)head->next;
    }
    return false;
}

void watcher_poll() {
    WatcherNode* head = watcher_list;
    while (head) {
        WatcherNode* next = (WatcherNode*)head->next;
        PollResult result = watcher_platform_poll_one(&head->w);
        if (result == ERR) {
            WATCHER_ERROR_PRINT("watcher for '%s' failed to poll events", head->w.path);
            // TODO: probably remove failing watchers since in most cases they will keep failing
        }
        head = next;
    }
}

// c++ specific implementations
#if defined(__cplusplus)
#if defined(_WIN32)

#elif defined(__APPLE__)

#elif defined(__linux__)

#endif  // #if defined(_WIN32)
#endif  // #if defined(__cplusplus)

#endif  // #if defined(WATCHER_IMPLEMENTATION)

#endif  // #ifndef WATCHER_H
