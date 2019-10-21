# Usage

make all

LD_PRELOAD=lib/apama_malloc.so ./test

This should produce output similar to :

malloc             8020 bytes
calloc                0 bytes
realloc           20000 bytes