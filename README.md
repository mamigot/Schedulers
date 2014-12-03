Schedulers
==========
**Miguel Amigot**
<br>
*Operating Systems, Fall 2014*


Implementation of the following scheduling algorithms for processes in C: First Come First Serve (FCFS), Round Robin (RR), Uniprogramming, Shortest Job First (SJF). The purpose of the program is to simulate the runtime behavior of each algorithm.

The program accepts an input consisting of a series of processes wherein each is classified by an arrival time, a CPU burst, a total amount of CPU time it requires and an IO burst (A, B, C, IO).

### Compile
The default option is a 32-bit machine. If necessary, modify the given Makefile to switch to 64-bit.
```
make main
```

### Execute
Either provide the text-based input as the first and only command line argument, or precede it by "--verbose" (options below). See sample [input files](inputs/).
```
./schedulers inputs/input-4.txt
./schedulers --verbose inputs/input-4.txt
```



####Upcoming fixes
* Linked-list functions should be completely abstracted from the file.
* Not every function is properly commented.