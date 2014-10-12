#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* General order:
	1. Blocked
	2. Running
	3. Unstarted
	4. Ready
*/
#define IS_BLOCKED 1
#define IS_RUNNING 2
#define IS_UNSTARTED 3
#define IS_READY 4

#define USE_UNIPROGRAMMING 10
#define USE_FCFS 11
#define USE_SJF 12
#define USE_RR 13

#define isFinished() (unstarted.size || ready.size || running.size || blocked.size) == 0

typedef struct {
	int A, B, C, IO;

	int status;
	int remBurst;

	int finishingTime;
	int turnaroundTime;
	int ioTime;
	int waitingTime;

	struct Process *next;
} Process;

typedef struct{
	Process *first;

	int kind;
	int size;
} ProcessList;


static FILE *fpRandomNumbers;
static FILE *fpInput;

static int numProcesses;
static Process *processes;

static int sysClock;
static ProcessList unstarted;
static ProcessList ready;
static ProcessList running;
static ProcessList blocked;


void printCycle();


Process *ProcessCreate(int A, int B, int C, int IO){
	// Parameters (time units):
	// Arrival, (CPU) Burst, CPU Needed, (I/O) Burst
	Process *proc = malloc(sizeof(Process));

	proc->A = A;
	proc->B = B;
	proc->C = C;
	proc->IO = IO;

	return proc;
}

int randomOS(int u){

	int curr;
	if(fpRandomNumbers != NULL){
		fscanf(fpRandomNumbers, "%d", &curr);
		return 1 + (curr % u);
	}

	return -1;
}

void readInput(){
	// The first integer of the file gives the total number of processes
	fscanf(fpInput, "%d", &numProcesses);

	// Create the array of processes
	processes = malloc(sizeof(Process) * numProcesses);

	int A, B, C, IO;
	Process curr;
	for(int i = 0; i < numProcesses; i++){
		fscanf(fpInput, " ( %d %d %d %d ) ", &A, &B, &C, &IO);
		curr = *ProcessCreate(A, B, C, IO);
		processes[i] = curr;
	}


	printf("The original input was: %d ", numProcesses);

	for(int i = 0; i < numProcesses; i++){
		curr = processes[i];
		printf("(%d %d %d %d) ", curr.A, curr.B, curr.C, curr.IO);
	}

	printf("\n");
}

void sortProcessesByArrivalTime(){
	// Selection sort

	Process proc;
	int i, j, minIndex;
	for(i = 0; i < numProcesses; i++){
		minIndex = i;
		for(j = i + 1; j < numProcesses; j++)
			if(processes[j].A < processes[minIndex].A)
				minIndex = j;

		proc = processes[i];
		processes[i] = processes[minIndex];
		processes[minIndex] = proc;
	}


	printf("The (sorted) input is:  %d ", numProcesses);

	for(int i = 0; i < numProcesses; i++){
		proc = processes[i];
		printf("(%d %d %d %d) ", proc.A, proc.B, proc.C, proc.IO);
	}

	printf("\n\n");
}

void freeLists(){

	free(unstarted.first);
	free(ready.first);
	free(running.first);
	free(blocked.first);
}

void initializeLists(){
	freeLists();

	int i;
	for(i = 0; i < numProcesses; i++){
		processes[i].status = IS_UNSTARTED;
		processes[i].remBurst = 0;
	}

	unstarted.first = malloc(sizeof(processes[i]) * numProcesses);
	memcpy(unstarted.first, processes, sizeof(processes[i]) * numProcesses);
	unstarted.kind = IS_UNSTARTED;
	unstarted.size = numProcesses;
	
	ready.kind = IS_READY;
	ready.size = 0;

	running.kind = IS_RUNNING;
	running.size = 0;

	blocked.kind = IS_BLOCKED;
	blocked.size = 0;
}

void moveProcess(ProcessList *from, ProcessList *to, Process proc){



}

void doBlocked(){
	if(blocked.size){

	}
}

void doRunning(){
	if(running.size){

	}
}

void doUnstarted(){
	if(unstarted.size){

	}
}

void doReady(){
	if(ready.size){

	}
}

void runSchedule(int schedulingAlgo){

	initializeLists();
	sysClock = 0;

	printf("This detailed printout gives the state and remaining burst for each process\n\n");


	while( !isFinished() ){
		
		doBlocked();
		doRunning();
		doUnstarted();
		doReady();

		printCycle();
		sysClock++;


		if(sysClock == 10)
			unstarted.size = 0;
	}
	
}

void printCycle(){
	printf("Before cycle %5d: ", sysClock);

	int i;
	Process curr;
	for(i = 0; i < numProcesses; i++){
		curr = processes[i];

		if(curr.status == IS_BLOCKED)
			printf("%11s", "blocked");

		else if(curr.status == IS_RUNNING)
			printf("%11s", "running");

		else if(curr.status == IS_UNSTARTED)
			printf("%11s", "unstarted");

		else if(curr.status == IS_READY)
			printf("%11s", "ready");


		printf("%3d", curr.remBurst);
	}

	printf(".\n");
}

int main(int argc, char *argv[]){

	fpRandomNumbers = fopen("random-numbers.txt", "r");
	fpInput = fopen("inputs/input-2.txt", "r");

	readInput();
	sortProcessesByArrivalTime();


	runSchedule(USE_UNIPROGRAMMING);


	fclose(fpRandomNumbers);
	fclose(fpInput);

	free(processes);
	freeLists();

}