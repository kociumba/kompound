// basic test for watcher.h — includes file creation / deletion events
// compile with: gcc -o watcher_test test.c -lpthread (POSIX)
// or: cl.exe test.c (MSVC)

#include <stdio.h>
#include <string.h>

#define WATCHER_IMPLEMENTATION
#include "watcher.h"

#if defined(_WIN32)
#include <windows.h>
static void sleep_ms(int ms) { Sleep(ms); }
static bool create_temp_file(const char* name) {
    HANDLE h = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    const char* data = "hello";
    DWORD written;
    WriteFile(h, data, (DWORD)strlen(data), &written, NULL);
    CloseHandle(h);
    return true;
}
static bool delete_temp_file(const char* name) {
    return DeleteFileA(name) != 0;
}
#else
#include <unistd.h>
#include <fcntl.h>
static void sleep_ms(int ms) { usleep(ms * 1000); }
static bool create_temp_file(const char* name) {
    FILE* f = fopen(name, "w");
    if (!f) return false;
    fprintf(f, "hello");
    fclose(f);
    return true;
}
static bool delete_temp_file(const char* name) {
    return remove(name) == 0;
}
#endif

static int g_callback_count = 0;

void on_change(const char* path, void* user_data) {
    (void)user_data;
    printf("  [callback] change detected: %s\n", path);
    g_callback_count++;
}

int main(void) {
    printf("=== watcher.h event test ===\n");
    printf("watcher lib version: %d\n\n", WATCHER_VERSION);

    const char* tmp_name = "watcher_test_tmp.txt";
    delete_temp_file(tmp_name);

    Watcher* w = watcher_watch(".", on_change, NULL);
    if (!w) {
        printf("watcher_watch returned NULL — failed to create watcher.\n");
        return 0;
    }

    printf("watching path: %s\n", w->path);

    watcher_run_poll_thread();

    if (!create_temp_file(tmp_name)) {
        printf("failed to create temp file.\n");
    } else {
        sleep_ms(300);
    }

    if (!delete_temp_file(tmp_name)) {
        printf("failed to delete temp file.\n");
    } else {
        sleep_ms(300);
    }

    watcher_stop_poll_thread();

    bool ok = watcher_stop(w);
    printf("\nwatcher_stop: %s\n", ok ? "ok" : "error");

    watcher_cleanup();

    printf("callbacks fired: %d\n", g_callback_count);

    if (g_callback_count > 0) {
        printf("Test passed\n");
        return 0;
    } else {
        printf("Test failed\n");
        return 0;
    }
}
