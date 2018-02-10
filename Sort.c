#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "err.h"
#include "protocol.h"

int N = 0;
int *mapped_mem;
int fd_memory = -1;
sem_t* sem_go_signal[MAX_N];

sem_t *sem_end_of_iteration;
sem_t *sem_sort_flag;
sem_t *sem_end_flag;
sem_t *sem_working_mutex;

char buffer[BUF_SIZE] = { 0 };
char buffer2[BUF_SIZE] = { 0 };
char buffer3[BUF_SIZE] = { 0 };

void cleanup() {
    sem_close(sem_end_of_iteration);
    sem_unlink(SEM_END_OF_ITERATION);
    sem_close(sem_sort_flag);
    sem_unlink(SORT_FLAG_MUTEX);
    sem_close(sem_end_flag);
    sem_unlink(END_FLAG_MUTEX);
    sem_close(sem_working_mutex);
    sem_unlink(WORKING_MUTEX);

    for (int i = 0; i < ((2*N)-1); i++) {
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, i);
        sem_close(sem_go_signal[i]);
        sem_unlink(buffer);
    }

    close(fd_memory);
    shm_unlink(SHM_NAME);
}

void siginit_handler(int sig) {
    cleanup();
    kill(0, SIGINT);
}

int main() {
    struct sigaction action;
    sigset_t block_mask;

    sigemptyset (&block_mask);
    action.sa_handler = siginit_handler;
    action.sa_mask = block_mask;
    action.sa_flags = 0;

    if (sigaction (SIGINT, &action, 0) == -1)
        syserr("sigaction");

    int flags, prot;

    scanf("%d", &N);

    fd_memory = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_memory == -1)
        syserr("shm_open");

    if (ftruncate(fd_memory, (2*N + ADDITIONAL_INFO) * sizeof(int)) == -1)
        syserr("truncate");

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED;
    mapped_mem = (int *) mmap(NULL, (2*N + ADDITIONAL_INFO) * sizeof(int), prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    for (int i = 0; i < 2*N; i++) {
        scanf("%d", &(mapped_mem[i]));
    }

    sem_end_of_iteration = sem_open(SEM_END_OF_ITERATION, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_end_of_iteration == SEM_FAILED)
        syserr("sem_end failed");

    SORT_FLAG = 1;
    sem_sort_flag = sem_open(SORT_FLAG_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");

    END_FLAG = 0;
    sem_end_flag = sem_open(END_FLAG_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_end_flag == SEM_FAILED)
        syserr("sem_end_flag failed");

    WORKING = N - 1;
    sem_working_mutex = sem_open(WORKING_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_working_mutex == SEM_FAILED)
        syserr("sem_working_mutex failed");


    for (int i = 0; i < (2 * N) - 1; i++) {
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, i);
        int how_much_open;
        if ((i % 2 == 0)) {
            if ((i != 0) && (i != ((2*N) - 2))) {
                how_much_open = 2;
            }
            else {
                how_much_open = 1;
            }
        }
        else {
            how_much_open = 0;
        }
        sem_go_signal[i] = sem_open(buffer, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, how_much_open);
        if (sem_go_signal[i] == SEM_FAILED)
            syserr("sem_go_signal failed");
    }

    for (int i = 0; i < (2*N) - 1; i++) {
        memset(buffer, 0, BUF_SIZE);
        memset(buffer2, 0, BUF_SIZE);
        sprintf(buffer, "%d", i);
        sprintf(buffer2, "%d", N);

        switch(fork()) {
            case -1:
                cleanup();
                kill(0, SIGINT);
                syserr("fork");
            case 0:
                execl("./A", "A", buffer, buffer2, NULL);
                syserr("execl");
            default:
                break;
        }
    }

    bool new_iteration_needed = true;
    while (new_iteration_needed) {
        if (sem_wait(sem_end_of_iteration)) {
            syserr("sem_end wait");
        }

        new_iteration_needed = (END_FLAG != 1);

        for (int i = 0; i < (2*N) - 1; i += 2) {
            if (i != 0 && i < (2*N) -2) {
                if (sem_post(sem_go_signal[i]))
                    syserr("sem post");
            }
            if (sem_post(sem_go_signal[i]))
                syserr("sem post");
        }
    }

    for (int i = 0; i < 2*N; i++) {
        printf("%d\n", mapped_mem[i]);
    }

    for (int i = 0; i < ((2*N)-1); i++) {
        if (wait(0) == -1) syserr("error in wait");
    }

    cleanup();
}
