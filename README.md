# sortMerge
Final Project for Operating Systems class

This program will sort lines in a file, based on the first 8 
characters in that file called the key. The last 56 characters
in that file is called data. That is just something that is 
tagged onto the key that doesnt matter when getting sorted

This program should be run with command line arguments:
argv[2] = number of threads, (1, 2, 4, 8)
There is no check to make sure that the number of threads is valid

argv[3] = name of the file that the user wants to be sorted

The files data_128 and data_512k are sorted. The original unsorted files
are data_128_orig and data_512k_orig

NOTE: My merge function is so bad that it actually is only slightly to 
use more than 1 thread than it is to just sort with only 1 thread,
but technically it does make multiple threads and it does sort the
file in the end, and the assignment doesn't say that it necessarily
needs to be faster. It is slower if you compile it without -O, but with -O
it becomes slightly faster. 

Example of running the code: 
time ./sortMerge 4 data_128 > results.txt

You can also leave out the > results .txt to print it to the screen. 
The amount of time that the sort takes varies even when numThreads
is the same. 




