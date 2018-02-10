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
    mapped_mem = (int *) mmap(NULL, (2*N + ADDITIONAL_INFO) * sizeof(int), prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    sem_t *sem_my_go_signal, *sem_prev_go_signal, *sem_next_go_signal;
    sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index);
    sem_my_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_my_go_signal == SEM_FAILED)
        syserr("sem_my_go_signal open");

    if (index != ((2*N) - 2)) {
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index + 1);
        sem_next_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
        if (sem_next_go_signal == SEM_FAILED)
            syserr("sem_next_go_signal open");
    }

    if (index != 0) {
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, index-1);
        sem_prev_go_signal = sem_open(buffer, O_RDWR, S_IRUSR | S_IWUSR, 0);
        if (sem_prev_go_signal == SEM_FAILED)
            syserr("sem_prev_go_signal open");
        }

    sem_t *sem_sort_flag;
    sem_sort_flag = sem_open(SORT_FLAG_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag open");

    sem_t *sem_end_flag;
    sem_end_flag = sem_open(END_FLAG_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_end_flag == SEM_FAILED)
        syserr("sem_end_flag open");

    sem_t *sem_working_mutex;
    sem_working_mutex = sem_open(WORKING_MUTEX, O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_working_mutex == SEM_FAILED)
        syserr("sem_working_mutex open");

    sem_t *sem_end_of_iteration;
    sem_end_of_iteration = sem_open(SEM_END_OF_ITERATION, O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_end_of_iteration == SEM_FAILED)
        syserr("sem_end open");


    bool working = true;
    while (working) {
        if (index != 0 && index != (2*N) -2) {
            if (sem_wait(sem_my_go_signal))
                syserr("waiting for my go signal");
        }

        if (sem_wait(sem_my_go_signal))
            syserr("waiting for my go signal");

        if (sem_wait(sem_end_flag)) {
            syserr("sem end flag wait");
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
            if (index > 0) {
                if (sem_post(sem_prev_go_signal))
                    syserr("sem prev");
            }
            if (index < ((2*N) - 2)) {
                if (sem_post(sem_next_go_signal))
                    syserr("sem next");
            }
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

                if (sem_post(sem_end_of_iteration)) {
                    syserr("sem_end post");
                }
            }

            if (sem_post(sem_working_mutex)) {
                syserr("sem_working post");
            }
        }
    }

    sem_close(sem_my_go_signal);

    if (index != ((2*N) - 2)) {
        sem_close(sem_next_go_signal);
    }
    if (index != 0) {
        sem_close(sem_prev_go_signal);
    }
    sem_close(sem_end_of_iteration);
    sem_close(sem_sort_flag);
    sem_close(sem_end_flag);
    sem_close(sem_working_mutex);

    close(fd_memory);
}
