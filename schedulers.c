#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
	int A, B, C, IO;

	int currBurst;

	int finishingTime;
	int turnaroundTime;
	int ioTime;
	int waitingTime;
} Process;

typedef struct{
	Process *items;
	int size;
} ProcessList;


static FILE *fpRandomNumbers;
static FILE *fpInput;

static int numProcesses;
static Process *processes;

static int clock;
static ProcessList *unstarted;
static ProcessList *ready;
static ProcessList *running;
static ProcessList *blocked;


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

void initializeLists(){
	// Free resources from previous scheduler runs (if any)
	free(unstarted);
	free(ready);
	free(running);
	free(blocked);

	unstarted = malloc(sizeof(ProcessList));
	unstarted->items = malloc(sizeof(Process) * numProcesses);
	// Copy all saved processes onto the unstarted list
	memcpy(unstarted->items, processes, sizeof(Process) * numProcesses);
	unstarted->size = numProcesses;

	ready = malloc(sizeof(ProcessList));
	ready->size = 0;

	running = malloc(sizeof(ProcessList));
	running->size = 0;

	blocked = malloc(sizeof(ProcessList));
	blocked->size = 0;


	clock = 0;
}

void startUniprogramming(){
	initializeLists();

	
}



void printProcesses(){

	Process curr;
	for(int i = 0; i < numProcesses; i++){
		curr = processes[i];
		printf("(%d %d %d %d)\n", curr.A, curr.B, curr.C, curr.IO);
	}
}

void printOutput(){
	printf("The original input was: %d ", numProcesses);

	Process curr;
	for(int i = 0; i < numProcesses; i++){
		curr = processes[i];
		printf("(%d %d %d %d) ", curr.A, curr.B, curr.C, curr.IO);
	}

	printf("\n");
}

int main(int argc, char *argv[]){

	fpRandomNumbers = fopen("random-numbers.txt", "r");
	fpInput = fopen("inputs/input-5.txt", "r");

	readInput();
	sortProcessesByArrivalTime();

	startUniprogramming();



	fclose(fpRandomNumbers);
	fclose(fpInput);
	free(processes);

}