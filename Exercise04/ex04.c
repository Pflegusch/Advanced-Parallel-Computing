#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

// Global vars
volatile int global_var = 0;
char lock = 0;
u_int32_t var = 0;
pthread_mutex_t fz_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

// Struct that hold the number of incrementations and amount of threads
struct args {
    int C;
    int N;
};

#define fetch_and_store(ptr, v)                                 \
({                                                              \
    typeof(*ptr) _v = (v);                                      \
    volatile uint64_t *_ptr = (volatile uint64_t *)(ptr);       \
    __asm__ __volatile__(                                       \
        "xchgq %0, %1\n\t"                                      \
        : "+r" (_v),                                            \
            "+m" (*_ptr)                                        \
        :                                                       \
        : "memory", "cc");                                      \
    _v;                                                         \
})

#define compare_and_store(ptr, cmp, swap)                       \
({                                                              \
    unsigned char result;                                       \
    typeof(*(ptr)) _cmp = (cmp);                                \
    typeof(*(ptr)) _swap = (swap);                              \
    volatile uint64_t *_p = (volatile uint64_t *)(ptr);         \
    __asm__ __volatile__ (                                      \
        "lock ; cmpxchgq %3,%1 \n\t"                            \
        "setz %2;\n\t"                                          \
        : "+a" (_cmp),                                          \
            "+m" (*_p),                                         \
            "=r" (result)                                       \
        : "r" (_swap)                                           \
        : "cc", "memory");                                      \
    result;                                                     \
})


// Wait for all threads to reach the same state in the program flow and then increment the
// global var using a mutex
void* increment_with_mutex(void* args) {
    pthread_barrier_wait(&barrier);
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        pthread_mutex_lock(&fz_mutex);
        global_var++;
        pthread_mutex_unlock(&fz_mutex);
    }

    return NULL;
}

// Use atomic operations, an atomic operation doesnt need locks because the operation
// itself is elementar
void* increment_atomic(void* args) {
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        __sync_add_and_fetch(&var, 1);
    }

    return NULL;
}

void* increment_own_mechanism(void* args) {
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        while (!__sync_bool_compare_and_swap(&lock, 0, 1)) {
            __asm("");
        }
        global_var++;
        __sync_sub_and_fetch(&lock, 1);
    }
    return NULL;
}

struct _mcslock_node {
    struct _mcslock_node *volatile next;
    volatile char wait;
} mcslock_node;

typedef struct {
    struct _mcslock_node *volatile tail;
} mcslock_t;

mcslock_t mlock;

static inline void mcs_lock_init(mcslock_t *lock)
{
   lock->tail = NULL; 
}

static inline void mcs_lock(mcslock_t *lock, struct _mcslock_node *mynode)
{
    struct _mcslock_node *pre;
    mynode->next = NULL;
    mynode->wait = 1;
    
    pre = fetch_and_store(&lock->tail, mynode);
    if (pre == NULL) {
        return;              
    }
    
    pre->next = mynode;
    while (mynode->wait) {
        __asm("");
    }
}

static inline void mcs_unlock(mcslock_t *lock, struct _mcslock_node *mynode)
{
    if (mynode->next == NULL) { 
        if (compare_and_store(&lock->tail, mynode, NULL)) {
            return;
        }
        while ( mynode->next == NULL) {
            __asm(""); 
        }
    }
    mynode->next->wait = 0;
    return;
}

void* increment_mcs(void* args) {
    struct _mcslock_node my_lock_node;
    for (int i = 0; i <= (((struct args *)args)->C / ((struct args *)args)->N) - 1; i++) {
        mcs_lock(&mlock, &my_lock_node);
        global_var++;
        mcs_unlock(&mlock, &my_lock_node);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 3) return -1;

    // Initialize cli args
    const int increment_count = atoi(argv[1]);
    const int num_threads = atoi(argv[2]);

    // Create array of pthreads
    pthread_t threads[num_threads];
 
    // Create struct which holds information for each thread
    struct args *thread_args = (struct args *)malloc(sizeof(struct args));
    thread_args->C = increment_count;
    thread_args->N = num_threads;

    clock_t begin_mutex = clock();
    // Initialize a barrier for the threads and run the threads using mutex and the barrier
    pthread_barrier_init (&barrier, NULL, num_threads);
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, increment_with_mutex, (void *)thread_args);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_mutex = clock();
    double time_spent_mutex = (double)(end_mutex - begin_mutex) / CLOCKS_PER_SEC;
    double up_per_sec_mutex = increment_count / time_spent_mutex;

    printf("Global var with mutex and barrier: %i\n", global_var);
    printf("Total execution time of the atomic lock mechanism %f\n", time_spent_mutex);
    printf("Updates per second with mutext %.0f\n\n", up_per_sec_mutex);

    clock_t begin_atomic = clock();
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, increment_atomic, (void *)thread_args);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_atomic = clock();
    double time_spent_atomic = (double)(end_atomic - begin_atomic) / CLOCKS_PER_SEC;
    double up_per_sec_atomic = increment_count / time_spent_atomic;
    
    printf("Global atomic var: %i\n", var);
    printf("Total execution time of the atomic lock mechanism %f\n", time_spent_atomic);
    printf("Updates per second atomic %.0f\n\n", up_per_sec_atomic);

    global_var = 0;

    clock_t begin_own = clock();
    // Run the threads using own locking mechanism
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, increment_own_mechanism, (void *)thread_args);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_t end_own = clock();
    double time_spent_own = (double)(end_own - begin_own) / CLOCKS_PER_SEC;
    double up_per_sec_own = increment_count / time_spent_own;

    printf("Global var own lock mechanism: %i\n", global_var);
    printf("Total execution time of our own lock mechanism %f\n", time_spent_own);
    printf("Updates per second own implementation %.0f\n\n", up_per_sec_own);

    // Reset global var
    global_var = 0;

    begin_own = clock();
    // Run the threads using own locking mechanism
    // Init the mcs lock
    mcs_lock_init(&mlock);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, increment_mcs, (void *)thread_args);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    end_own = clock();
    time_spent_own = (double)(end_own - begin_own) / CLOCKS_PER_SEC;
    up_per_sec_own = increment_count / time_spent_own;

    printf("Global var mcs lock mechanism: %i\n", global_var);
    printf("Total execution time of mcs lock mechanism %f\n", time_spent_own);
    printf("Updates per second mcs implementation %.0f\n\n", up_per_sec_own);
    assert(global_var == increment_count);

    return 0;
}
