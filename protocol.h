#ifndef PROTOCOL_PSZ
#define PROTOCOL_PSZ


#define MAX_N  1000
#define BUF_SIZE 100

#define SHM_NAME "/pszbubble_sort"
#define GO_SIGNAL_PREFIX "/go_signal_"
#define SORT_FLAG_MUTEX "/sort_flag_mutex"
#define END_FLAG_MUTEX "/end_flag_mutex"
#define WORKING_MUTEX "/working_mutex"
#define END_MUTEX "/end_mutex"
#define SEM_NEW_ITERATION "/sem_new_iteration"
#define SEM_END_OF_ITERATION "/sem_end_iteartion"

// no need to use struct
#define SORT_FLAG_POSITION   (2*N)
#define WORKING_POSITION     ((2*N)+1)
#define END_FLAG_POSITION    ((2*N)+2)
#define ADDITIONAL_INFO      3
#define SORT_FLAG            mapped_mem[SORT_FLAG_POSITION]
#define WORKING              mapped_mem[WORKING_POSITION]
#define END_FLAG             mapped_mem[END_FLAG_POSITION]

#endif
