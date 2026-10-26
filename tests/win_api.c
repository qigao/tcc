#include <windows.h>
#include <stdio.h>
int main(void) {
    HANDLE event = CreateEventW(NULL, TRUE, FALSE, NULL);
    LARGE_INTEGER ticks;
    if (!event) return 1;
    if (WaitForSingleObject(event, 0) != WAIT_TIMEOUT) return 2;
    if (!SetEvent(event) || WaitForSingleObject(event, 5000) != WAIT_OBJECT_0) return 3;
    if (!QueryPerformanceCounter(&ticks) || !CloseHandle(event)) return 4;
    printf("win32: event, wait, counter, PID=%lu OK\n", (unsigned long)GetCurrentProcessId());
    return 0;
}
