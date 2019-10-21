#include <iostream>
#include <cstdlib>
#include <atomic>
#include <dlfcn.h>
#include <cinttypes>

//make this easy - typedefs make things clear
typedef void* (*real_malloc_t)(size_t);
typedef void (*real_free_t)(void *);
typedef void* (*real_realloc_t)(void *, size_t);
typedef void* (*real_calloc_t)(size_t, size_t);

//for storing the actual methods
static real_malloc_t real_malloc = 0;
static real_free_t real_free = 0;
static real_realloc_t real_realloc = 0;
static real_calloc_t real_calloc = 0;

//allocated memory
std::atomic<long long> allocated (0);
std::atomic<long long> callocated (0);
std::atomic<long long> rallocated (0);

//init lock
std::atomic<bool> init_flag(false);
std::atomic_flag lock = ATOMIC_FLAG_INIT;

//TODO: need to add a way to record the deletions (hashmap or potentially simply write in another thread )
std::atomic<long long> freed (0);

static FILE *memlog = 0;

//helper functions 
static void initialize_symbols()
{
    //return is another thread initialises it,
    //otherwise grab the lock and initialise
    do
    {
        if ( init_flag.load() )
            return;
    } while(std::atomic_flag_test_and_set_explicit(&lock, std::memory_order_acquire));

    

             ; // spin until the lock is acquired
    memlog = fdopen(200, "w");
    if (!memlog)
    {
        memlog = stderr;
    }
    fprintf(memlog, "Setting symbols\n");
    real_malloc = reinterpret_cast<real_malloc_t>( dlsym(RTLD_NEXT, "malloc") );
    real_calloc = reinterpret_cast<real_calloc_t>( dlsym(RTLD_NEXT, "calloc") );
    real_free = reinterpret_cast<real_free_t>( dlsym(RTLD_NEXT, "free") );
    real_realloc = reinterpret_cast<real_realloc_t> (dlsym(RTLD_NEXT, "realloc") );
    init_flag = true;
    std::atomic_flag_clear_explicit(&lock, std::memory_order_release);
}


//runs when shared lib is loaded
//https://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work
void
    __attribute__((constructor))
    _init(void)
{
    initialize_symbols();
}

void
    __attribute__((destructor))
    _dtor(void)
{
    if (memlog)
    {
        fprintf(memlog, "malloc %16" PRId64 " bytes\n", allocated.load());
        fprintf(memlog, "calloc %16" PRId64 " bytes\n", callocated.load());
        fprintf(memlog, "realloc %15" PRId64 " bytes\n", rallocated.load());
        //fprintf(memlog, "free %15" PRId64 " bytes\n", freed.load()); 
    }
}

//define the actual overrides for the methods we want
//version 1 is simply logging all the memory allocated and freed 
//I use 
void *malloc(size_t size)
{
    void *p;
    while(!init_flag.load())
    {
        initialize_symbols();
        if (!real_malloc)
            return NULL;
    }
    p = real_malloc(size);
    allocated.fetch_add(size);
    return p;
}

//allocate and zero initialize
void *calloc(size_t nmemb, size_t size)
{
    void *p;
    while(!init_flag.load())
    {
        initialize_symbols();
        if (!real_calloc)
            return NULL;
    }
    p = real_calloc(nmemb, size);
    callocated.fetch_add(size);
    return p;
}

void free(void *ptr)
{
    while(!init_flag.load())
    {
        initialize_symbols();
        if (!real_free)
            return;
    }
    real_free(ptr);
    //freed.fetch_sub(sizeof(ptr));
}

void *realloc(void *ptr, size_t size)
{
    void *p;
    while(!init_flag.load())
    {
        initialize_symbols();
        if (!real_realloc)
            return NULL;
    }
    p = real_realloc(ptr, size);
    rallocated.fetch_add(size);
    return p;
}
