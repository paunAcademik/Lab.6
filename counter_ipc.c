#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <time.h>

#define SHM_NAME "/shm_lab_counter"
#define SEM_NAME "/sem_lab_counter"
#define TARGET_COUNT 1000

typedef struct {
    int counter;
} shared_data_t;

int main() {
    // Seed unic pentru generatorul random bazat pe PID si timp
    srand((unsigned int)(time(NULL) ^ getpid()));

    // 1. Deschidere / Creare Semafor POSIX (valoare initiala 1)
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Eroare la sem_open");
        exit(EXIT_FAILURE);
    }

    // 2. Deschidere / Creare Shared Memory
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Eroare la shm_open");
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    // Alocam dimensiunea necesara pentru structura in memoria partajata
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("Eroare la ftruncate");
    }

    // 3. Mapare in spatiul de adrese al procesului curent
    shared_data_t *data = (shared_data_t *)mmap(
        NULL,
        sizeof(shared_data_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (data == MAP_FAILED) {
        perror("Eroare la mmap");
        close(shm_fd);
        sem_close(sem);
        exit(EXIT_FAILURE);
    }

    printf("[PID %d] Conectat la memoria partajata si semafor.\n", getpid());

    while (1) {
        // Blocheaza semaforul (Wait / P)
        sem_wait(sem);

        // Verificam conditia de oprire
        if (data->counter >= TARGET_COUNT) {
            sem_post(sem); // Nu uita sa eliberezi semaforul pentru celalalt proces
            break;
        }

        // "Dam cu banul": rand() % 2 returneaza 0 sau 1 (1 echivaleaza cu optiunea "2")
        // Cat timp da cu banul favorabil si nu a depasit 1000
        while ((rand() % 2 == 1) && (data->counter < TARGET_COUNT)) {
            data->counter++;
            printf("[PID %d] Scrie: %d\n", getpid(), data->counter);
            fflush(stdout);
        }

        // Elibereaza semaforul (Post / V)
        sem_post(sem);

        // Pauza de 1-2 ms pentru a lasa nucleul sa schimbe contextul la celalalt proces
        usleep(1000);
    }

    printf("[PID %d] A atins/detectat finalul (%d). Inchidere...\n", getpid(), data->counter);

    // Curatare resurse locale
    munmap(data, sizeof(shared_data_t));
    close(shm_fd);
    sem_close(sem);

    return 0;
}
