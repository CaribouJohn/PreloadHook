#!/usr/bin/env python
import re
#the following script is assessing memory usage
#run ltrace -tt -C -n 2 -x malloc+free+realloc+calloc+brk+sbrk+mmap+munmap -o malloc_override.log correlator

#open the log file and generate a table of allocations and deallocations.

mainlog = open( 'malloc_override.log','r')

#libstdc++.so.6->malloc(36) 
malloc_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+malloc.*\((\d+).*=\s+(.+)')
#14:10:36.130338       malloc@libc.so.6(5876 <unfinished ...>
malloc2_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+malloc.*\((\d+)\s+<unfinished')
#14:10:36.130610       <... malloc resumed> )                                                                                                  = 0x4a4ad90
malloc3_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+).+malloc.*resumed.*=\s(.+)')
calloc_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+calloc.*\((\d+),\s+(\d+).*=\s+(.+)')
realloc_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+realloc.*\(([^,]+),\s+(\d+).*=\s(.+)')
sbrk_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+sbrk.+\((\d+)')

free_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+free.*\((0x.+)\)')
#16:03:50.364206   ObjectPool::freeVariableSize(void*)(0x4c881a0, 0x4c882b0,
free2_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+).+freeVariableSize.*\((0x[^,]+),\s*(0x[^,]+)')

mmap_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+mmap.*\((\d+),\s+0x(\d+).*=\s+(.+)')
munmap_pat = re.compile(r'(\d\d:\d\d:\d\d\.\d+)\s+munmap.+\((0x[^,]+),\s*[0x]*(\d+)')

total_allocated = 0
total_sbrk = 0
total_mmap = 0
memorydb = {}

#"graphs" - req , mall , sbrk , mmap
results = {} 
counter = 0
results[counter] = (total_allocated,total_sbrk,total_mmap)
interrupted_malloc = 0

for line in mainlog:
    matches = re.match(mmap_pat,line)
    if matches:
        print( "mmap incremented : {} ".format(matches.groups()[2]))
        total_mmap += int(matches.groups()[2]) 

    matches = re.match(munmap_pat,line)
    if matches:
        print( "mmap decremented : {} ".format(matches.groups()[2]))
        total_mmap -= int(matches.groups()[2]) 

    #sbrk is a running total of requested increases in program memory
    matches = re.match(sbrk_pat,line)
    if matches:
        #print( "changed : {} ".format(matches.groups()[1]))
        total_sbrk = int(matches.groups()[1]) 

    #Record total at each point of allocation
    matches = re.match(malloc_pat,line)
    if matches:
        #print( "allocated : {} at {}".format(matches.groups()[1],matches.groups()[2]))
        size = int(matches.groups()[1])
        memorydb[matches.groups()[2]] = size
        total_allocated += size

    #Record total at each point of allocation
    matches = re.match(malloc2_pat,line)
    if matches:
        #print( "allocated : {} ".format(matches.groups()[1]))
        interrupted_malloc = int(matches.groups()[1])

    #Record total at each point of allocation
    matches = re.match(malloc3_pat,line)
    if matches and interrupted_malloc > 0:
        #print( "interupted allocated : {} at {}".format(interrupted_malloc,matches.groups()[1],))
        memorydb[matches.groups()[1]] = interrupted_malloc
        total_allocated += interrupted_malloc
        interrupted_malloc = 0 #reset - will need to find malloc again before this has an effect


    matches = re.match(realloc_pat,line)
    if matches:
        #print( line )
        #print( "(re)allocated : from {} to {} at {}".format(matches.groups()[1],matches.groups()[2],matches.groups()[3]))
        newsize = int(matches.groups()[2])
        oldsize = memorydb.get(matches.groups()[1])
        memorydb[matches.groups()[3]] = newsize
        if matches.groups()[1] != matches.groups()[3]:
            memorydb[matches.groups()[1]] = 0

        total_allocated += newsize
        if oldsize != None:
            total_allocated -= oldsize

    matches = re.match(calloc_pat,line)
    if matches:
        size = int(matches.groups()[1])*int(matches.groups()[2])
        #print( "(c)allocated : {} at {}".format(size,matches.groups()[3]))
        memorydb[matches.groups()[3]] = size
        total_allocated += size

    matches = re.match(free_pat,line)
    if matches:
        if matches.groups()[1] not in memorydb : 
            print( "cannot free {} check malloc_override.log".format(matches.groups()[1]))
            continue
        oldsize = memorydb.get(matches.groups()[1]) 
        memorydb[matches.groups()[1]] = 0
        total_allocated -= oldsize
        #print( "free : {} from {} ".format(oldsize,matches.groups()[1]))

    matches = re.match(free2_pat,line)
    if matches:
        if matches.groups()[2] not in memorydb : 
            #in this case we can have pooled addresses so ignore failures
            #print( "cannot free {} check malloc_override.log".format(matches.groups()[1]))
            continue
        oldsize = memorydb.get(matches.groups()[2]) 
        memorydb[matches.groups()[2]] = 0
        total_allocated -= oldsize
        #print( "free : {} from {} ".format(oldsize,matches.groups()[1]))
    
    #running tots
    counter += 1
    results[counter] = (total_allocated,total_sbrk,total_mmap)

mainlog.close()

print "--usage data--"

print "seq,requested,sbrk,mmap"
for key in sorted(results):
    print("{},{},{},{}".format(key,results[key][0],results[key][1],results[key][2]))

print "--memory not freed (may not be a leak - check malloc_override.log for the address)--"

leaked = 0
for key in memorydb:
    if memorydb[key] > 0:
        print("{} not released {}".format(key,memorydb[key]))
        leaked += memorydb[key]

print "--Unaccounted for memory that got 'allocated'--"
print ('total leaked = ' + str(leaked))