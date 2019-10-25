# Usage

make all

`LD_PRELOAD=lib/malloc_override.so ./test`

This should produce output similar to :

~~~
Loading malloc override
72736
...
120736
120736
120760
120736
120712
unloading malloc override
~~~

These numbers are the total allocated memory for the process that occurred through malloc, realloc and calloc...

N.B. there have been issues running this including recursion through the memory functions overridden that will cause segfaults... FWIW if your allication is relatively simple then I have found that it works well. However if you have a complex application that links many libraries your milage may vary - use at your own risk. 

# ltrace_memory.py 

This is a script that I wrote that can partially run through the output of ltrace and generate a similar curve 

`ltrace -tt -C -n 2 -x malloc+free+realloc+calloc+brk+sbrk+mmap+munmap -o my.log myapplication > myapp.log`

then run the script (at the time of writing the log is hardcoded but easy to change from *malloc_override.log*)

`ltrace_memory.py`

it covers :

malloc
calloc,
realloc,
free,
sbrk,
mmap,
munmap

Again milages might vary as the regexes were relatively complex and some entries cover multiple lines

produces a sequence of memory values as they changed through the log.

~~~
cannot free 0x7fa650000940 check my.log
...
--usage data--
seq,requested,sbrk,mmap
0,0,0,0
1,0,0,0
2,0,0,0
3,0,0,0
4,0,0,0
5,32,0,0
6,32,0,0
7,32,0,0
8,64,0,0
9,64,0,0
10,64,0,0
...
285856,6890521,135168,3132733
285857,6890521,135168,3132733
--memory not freed (may not be a leak - check malloc_override.log for the address)--
0x4a73a80 not released 648
0x4c8bfd0 not released 4

0x4c50b80 not released 88
--Unaccounted for memory that got 'allocated'--
total leaked = xxxx

