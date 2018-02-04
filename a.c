#include <time.h>
#include <unistd.h>
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
    char buffer1[BUF_SIZE] = { 0 };
    char buffer2[BUF_SIZE] = { 0 };
    memset(buffer1, 0, BUF_SIZE);
    memset(buffer2, 0, BUF_SIZE);

    if (argc <= 3) {
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

    sem_t *sem_sort_flag;
    sem_sort_flag = sem_open(SORT_FLAG_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");

    sem_t* sem_new_iteration = sem_open(SEM_NEW_ITERATION, O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_new_iteration == SEM_FAILED)
        syserr("new_iteration  sem_open");

    if ((index % 2) == 0) {
        if (index != 0) {
            sprintf(buffer1, "%s%d", GO_SIGNAL_PREFIX, index-1);
        }
        sprintf(buffer2, "%s%d", GO_SIGNAL_PREFIX, index+1);
    }
    else {
        sprintf(buffer1, "%s%d", GO_SIGNAL_PREFIX, index);
    }

    while (1) {
        if ((index % 2) == 0) { // Proces A;
            if (sem_wait(sem_new_iteration))
                syserr("new_iteration wait");
        }
        else { // Proces B;
            // P(signal[i])
            // if indx < ...
            //  P(signal[i])
        }

        // P(END)
        // if end
        // signal B;
        // break;

        if (mapped_memory[index] < mapped_memory[index + 1]) {
            int tmp = mapped_memory[index];
            mapped_memory[index] = mapped_memory[index + 1];
            mapped_memory[index + 1] = tmp;

            if (sem_wait(sem_sort_flag)) {
                syserr("sort flag wait");
            }

            SORT_FLAG = 1;

            if (sem_post(sem_sort_flag)) {
                syserr("sort flag post");
            }
        }

        if ((index % 2) == 0) {

        }
        else {
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

                    if (sem_post(sem_end)) {
                        syserr("sem end post");
                    }
                }
                else {
                    SORT_FLAG = 1;
                    WORKING = N;
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

        // if A[i] < A[i+1]
        // swap()
        // P(sorted)
        // sorted = false
        // V(sorted)

        // P(working)
        // working--;
        // if (working == 0)
        // if sorted :
        //  end = true;
        //  signal_end
        //
        // working = 2*n
        // P(new_generation, N)

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED;
    mapped_mem = (int *) mmap(NULL, (2*N + 2) * sizeof(int), prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    sem_t *sem_end;
    sem_end = sem_open(END_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    if (sem_end == SEM_FAILED)
        syserr("sem_end failed");

    SORT_FLAG = 1;
    sem_t *sem_sort_flag;
    sem_sort_flag = sem_open(SORT_FLAG_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");


    WORKING = 2*N;
     sem_t *sem_working_mutex;
    sem_sort_flag = sem_open(WORKING_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");

    for (int i = 0; i < 2*N; i++) {
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "%s%d", ARR_MUTEX_PREFIX, i);


    }

    for (int i = 0; i < 2*N; i++) {
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "%d", i);

        switch(fork()) {
            case -1:
                syserr("fork");
            case 0:
                execl("./a", "a", buffer, NULL);
                syserr("execl");
            default:
                break;
        }
    }

    if (sem_wait(sem_end)) {
        syserr("sem_end wait");
    }

    for (int i = 0; i < 2*N; i++) {
        printf("%d", mapped_mem[i]);
    }

    sem_close(sem_end);
    sem_unlink(END_MUTEX);
    sem_close(sem_sort_flag);
    sem_unlink(SORT_FLAG_MUTEX);

    close(fd_memory);
    shm_unlink(SHM_NAME);
}
