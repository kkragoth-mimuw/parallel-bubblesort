#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */

#include "protocol.h"
#include "err.h"

int main(int argc, char *argv[]) {
    char buffer[BUF_SIZE] = { 0 };
    memset(buffer, 0, BUF_SIZE);

    if (argc != 3) {
        printf("pass index, N as arguments");
        exit(1);
    }

    int index = atoi(argv[1]);
    int N = atoi(argv[2]);

    int *mapped_mem;
    int fd_memory = -1;
    int flags, prot;

    fd_memory = shm_open(SHM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_memory == -1)
        syserr("shm_open");

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED;
    mapped_mem = (int *) mmap(NULL, (2*N + 2) * sizeof(int), prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    sem_t *sem_my_go_signal, *sem_prev_go_signal, *sem_next_go_signal;
    if ((index % 2) == 0) { // Process A
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index + 1);
        sem_next_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
        if (sem_next_go_signal == SEM_FAILED)
            syserr("sem_sort_flag failed");

        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index-1);
        sem_prev_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
        if (sem_prev_go_signal == SEM_FAILED)
            syserr("sem_sort_flag failed");
    }
    else { // Process B
         sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index);
        sem_my_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
        if (sem_my_go_signal == SEM_FAILED)
            syserr("sem_sort_flag failed");
    }

    sem_t *sem_sort_flag;
    sem_sort_flag = sem_open(SORT_FLAG_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");

    sem_t *sem_end_flag;
    sem_end_flag = sem_open(END_FLAG_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_end_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");

    sem_t *sem_working_mutex;
    sem_working_mutex = sem_open(WORKING_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_working_mutex == SEM_FAILED)
        syserr("sem_working failed");

    sem_t *sem_new_iteration;
    sem_new_iteration = sem_open(SEM_NEW_ITERATION, O_RDWR, S_IRUSR | S_IWUSR, N);
    if (sem_new_iteration == SEM_FAILED)
        syserr("sem_sort_flag failed");

    sem_t *sem_end;
    sem_end = sem_open(END_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_end == SEM_FAILED)
        syserr("sem_end failed");


    bool working = true;
    while (working) {
        if ((index % 2) == 0) { // Process A
            if (sem_wait(sem_new_iteration))
                syserr("sem new iteration");
        }
        else  { // Process B
            for (int i = 0; i < 2; i++) {
                if (sem_wait(sem_my_go_signal))
                    syserr("waiting for go signal");
            }
        }

        if (sem_wait(sem_end_flag)) {
            syserr("sem end flag mutex");
        }
        working = (END_FLAG != 1);

        if (sem_post(sem_end_flag)) {
            syserr("sem end post");
        }

        if (working && (mapped_mem[index] > mapped_mem[index + 1])) {
            int tmp = mapped_mem[index];
            mapped_mem[index] = mapped_mem[index + 1];
            mapped_mem[index + 1] = tmp;

            if (sem_wait(sem_sort_flag))
                syserr("sem sort flag");

            SORT_FLAG = 0;

            if (sem_post(sem_sort_flag))
                syserr("sem post sort flag");
        }

        if ((index % 2) == 0) { // Process A
            if (sem_post(sem_prev_go_signal))
                syserr("sem prev");
            if (sem_post(sem_next_go_signal))
                syserr("sem next");
        }
        else if (working) { // Process B
            if (sem_wait(sem_working_mutex)) {
                syserr("sem_working_mutex");
            }

            WORKING -= 1;

            if (WORKING == 0) {
                if (sem_wait(sem_sort_flag)) {
                    syserr("sort flag wait");
                }

                if (SORT_FLAG == 1) {
                    if (sem_wait(sem_end_flag)) {
                        syserr("sem end flag mutex");
                    }

                    END_FLAG = 1;

                    if (sem_post(sem_end_flag)) {
                        syserr("sem end post");
                    }
                }
                else {
                    SORT_FLAG = 1;
                    WORKING = N - 1;
                }

                if (sem_post(sem_sort_flag)) {
                    syserr("sort flag post");
                }

                for (int i = 0; i < N; i++) {
                    if (sem_post(sem_new_iteration)) {
                        syserr("sem new iteration post");
                    }
                }
            }

            if (sem_post(sem_working_mutex)) {
                syserr("sem_working post");
            }
        }
    }

    sem_close(sem_end);
    sem_unlink(END_MUTEX);
    sem_close(sem_sort_flag);
    sem_unlink(SORT_FLAG_MUTEX);

    close(fd_memory);
    shm_unlink(SHM_NAME);
}
