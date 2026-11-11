#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SHM_NAME L"Local\\Lab6_SharedMemory"
#define SEM_NAME L"Local\\Lab6_Semaphore"
#define TARGET_COUNT 1000

typedef struct {
    int counter;
} SharedData;

int main(void) {
    DWORD pid = GetCurrentProcessId();
    srand((unsigned int)(time(NULL) ^ pid));

    HANDLE hSem = CreateSemaphoreW(NULL, 1, 1, SEM_NAME);
    if (hSem == NULL) {
        printf("[PID %lu] Eroare la CreateSemaphore: %lu\n", pid, GetLastError());
        return 1;
    }

    HANDLE hMapFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE,
        NULL,
        PAGE_READWRITE,
        0,
        sizeof(SharedData),
        SHM_NAME
    );

    if (hMapFile == NULL) {
        printf("[PID %lu] Eroare la CreateFileMapping: %lu\n", pid, GetLastError());
        CloseHandle(hSem);
        return 1;
    }

    SharedData *data = (SharedData *)MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(SharedData)
    );

    if (data == NULL) {
        printf("[PID %lu] Eroare la MapViewOfFile: %lu\n", pid, GetLastError());
        CloseHandle(hMapFile);
        CloseHandle(hSem);
        return 1;
    }

    printf("[PID %lu] Conectat la memoria mapata si semafor.\n", pid);

    while (1) {
        WaitForSingleObject(hSem, INFINITE);

        if (data->counter >= TARGET_COUNT) {
            ReleaseSemaphore(hSem, 1, NULL);
            break;
        }

        while ((rand() % 2 == 1) && (data->counter < TARGET_COUNT)) {
            data->counter++;
            printf("[PID %lu] Scrie: %d\n", pid, data->counter);
            fflush(stdout);
        }

        ReleaseSemaphore(hSem, 1, NULL);
        Sleep(1);
    }

    printf("[PID %lu] Final atins (%d). Inchidere...\n", pid, data->counter);

    UnmapViewOfFile(data);
    CloseHandle(hMapFile);

    return 0;
}
