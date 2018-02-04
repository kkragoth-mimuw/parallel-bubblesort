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
#include "protocol.h"

int main() {
    sem_t* sem_go_signal_index[MAX_N];
    sem_t* sem_array_position_index[MAX_N];
    char buffer[BUF_SIZE] = { 0 };
    char buffer2[BUF_SIZE] = { 0 };
    int *mapped_mem;
    int fd_memory = -1;
    int flags, prot;

    int N;
    scanf("%d", &N);

    fd_memory = shm_open(SHM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd_memory == -1)
        syserr("shm_open");

    if (ftruncate(fd_memory, (2*N + 3) * sizeof(int)) == -1)
        syserr("truncate");

    prot = PROT_READ | PROT_WRITE;
    flags = MAP_SHARED;
    mapped_mem = (int *) mmap(NULL, (2*N + 3) * sizeof(int), prot, flags, fd_memory, 0);
    if (mapped_mem == MAP_FAILED)
        syserr("mmap");

    for (int i = 0; i < 2*N; i++) {
        scanf("%d", &(mapped_mem[i]));
    }

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


    sem_t *sem_even_phase;
    sem_even_phase = sem_open(SEM_EVEN, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_even_phase == SEM_FAILED)
        syserr("sem_sort_flag failed");

    sem_t *sem_odd_phase;
    sem_odd_phase = sem_open(SEM_ODD, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_odd_phase == SEM_FAILED)
        syserr("sem_sort_flag failed");

    WORKING = 2*N;
     sem_t *sem_working_mutex;
    sem_sort_flag = sem_open(WORKING_MUTEX, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem_sort_flag == SEM_FAILED)
        syserr("sem_sort_flag failed");


    for (int i = 1; i < 2*N; i += 2) {
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "%s%d", GO_SIGNAL_PREFIX, i);
        sem_array_position_index[i] = sem_open(buffer, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
        if (sem_array_position_index[i] == SEM_FAILED)
            syserr("sem_end failed");
    }

    for (int i = 0; i < 2*N; i++) {
        memset(buffer, 0, BUF_SIZE);
        memset(buffer2, 0, BUF_SIZE);
        sprintf(buffer, "%d", i);
        sprintf(buffer2, "%d", N);

        switch(fork()) {
            case -1:
                syserr("fork");
            case 0:
                execl("./a", "a", buffer, buffer2, NULL);
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

    for (int i = 0; i < 2*N; i++) {
        memset(buffer, 0, BUF_SIZE);
        sprintf(buffer, "%s%d", ARR_MUTEX_PREFIX, i);
        sem_close(sem_array_position_index[i]);
        sem_unlink(buffer);

        if (wait(0) == -1) syserr("error in wait");
    }

    sem_close(sem_end);
    sem_unlink(END_MUTEX);
    sem_close(sem_sort_flag);
    sem_unlink(SORT_FLAG_MUTEX);

    close(fd_memory);
    shm_unlink(SHM_NAME);
}
