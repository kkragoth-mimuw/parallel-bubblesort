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

#include "err.h"

#define BUF_SIZE 100

#define SHM_NAME "/pszbubble_sort"
#define ARR_MUTEX_PREFIX "/array_position_"
#define SORT_FLAG_MUTEX "/flag_mutex"
#define WORKING_MUTEX "/working_mutex"
#define END_MUTEX "/end_mutex"


// no need to use struct
#define SORT_FLAG_POSITION (2*N)
#define WORKING_POSITION   ((2*N)+1)
#define SORT_FLAG          mapped_mem[SORT_FLAG_POSITION]
#define WORKING            mapped_mem[WORKING_POSITION]

int main(int argc, char *argv[]) {
    char buffer[BUF_SIZE] = { 0 };

    if (argc <= 2) {
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
