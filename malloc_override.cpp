#include <iostream>
#include <cstdlib>
#include <atomic>
#include <dlfcn.h>
#include <cinttypes>
#include <malloc.h>
#include <cstdio>
#include <cstring>
#include <unistd.h>


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

int reentrancy_guard;


#define BEGIN_HOOK \
        reentrancy_guard++;

#define HOOK \
    if (reentrancy_guard == 1)

#define END_HOOK \
        reentrancy_guard--;

//allocated memory
std::atomic<long long> allocated (0);


//runs when shared lib is loaded
//https://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work
void
    __attribute__((constructor))
    _init(void)
{
    fprintf(stderr, "Loading apama malloc override\n");
}

void
    __attribute__((destructor))
    _dtor(void)
{
    fprintf(stderr, "unloading apama malloc override\n");
}

void outputLine() {
    char buffer[512];
    sprintf(buffer, "%" PRId64 "\n", allocated.load());
    write(STDERR_FILENO,buffer,strlen(buffer));
}

//define the actual overrides for the methods we want
//version 1 is simply logging all the memory allocated and freed 
//I use 
void *malloc(size_t size)
{
    void *p;
    BEGIN_HOOK
    HOOK
        real_malloc = reinterpret_cast<real_malloc_t>( dlsym(RTLD_NEXT, "malloc") );

    p = real_malloc(size);
    size_t real_size = malloc_usable_size(p);
    allocated.fetch_add(real_size);
    outputLine();
    END_HOOK

    return p;
}

//allocate and zero initialize
void *calloc(size_t nmemb, size_t size)
{
    void *p;
    BEGIN_HOOK
    HOOK
        real_calloc = reinterpret_cast<real_calloc_t>( dlsym(RTLD_NEXT, "calloc") );
    p = real_calloc(nmemb, size);
    size_t real_size = malloc_usable_size(p);
    allocated.fetch_add(real_size);
    outputLine();
    END_HOOK
    return p;
}

void free(void *ptr)
{
    BEGIN_HOOK
    HOOK
        real_free = reinterpret_cast<real_free_t>( dlsym(RTLD_NEXT, "free") );

    size_t real_size = malloc_usable_size(ptr);
    allocated.fetch_sub(real_size);
    real_free(ptr);
    outputLine();
    END_HOOK
}

void *realloc(void *ptr, size_t size)
{
    void *p;
    BEGIN_HOOK
    HOOK
        real_realloc = reinterpret_cast<real_realloc_t> (dlsym(RTLD_NEXT, "realloc") );
    size_t real_prev_size = malloc_usable_size(ptr);
    p = real_realloc(ptr, size);
    size_t real_size = malloc_usable_size(p);
    allocated.fetch_sub(real_prev_size);
    allocated.fetch_add(real_size);
    outputLine();
    END_HOOK
    return p;
}
