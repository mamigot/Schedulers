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
#define IS_TERMINATED 5

#define USE_UNIPROGRAMMING 10
#define USE_FCFS 11
#define USE_SJF 12
#define USE_RR 13

#define isFinished() ((unstarted.size || ready.size || running.size || blocked.size) == 0) //&& (terminated.size == numProcesses)

typedef struct Process{
	int A, B, C, IO;

	int status;
	int remBurst;

	int finishingTime;
	int turnaroundTime;
	int ioTime;
	int waitingTime;

	// Useful in a linked-list setting
	struct Process *next;
	struct Process *prev;
} Process;


typedef struct{
	Process *first;
	Process *last;

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
static ProcessList terminated;


void printCycle();
void printList();
void printProcess();


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

void moveProcess(Process *proc, ProcessList *oldList, ProcessList *newList){
	// Resolve next and prev links in the old and new lists
	// Insert it in the new list at the end
	// Update the process' status

	if(oldList->size == 0)
		return;

	if( oldList->size == 1){
		oldList->first = NULL;
		oldList->last = NULL;

	}else if( oldList->size > 1 ){
		oldList->first = proc->next;
		oldList->first->prev = NULL;
	}
	oldList->size--;


	if( !newList->size ){
		newList->first = proc;
		newList->last = proc;

		proc->next = NULL;
		proc->prev = NULL;

	}else if( newList->size >= 1 ){
		newList->last->next = proc;
		proc->prev = newList->last;
		newList->last = proc;

	}
	newList->size++;


	proc->status = newList->kind;
}

void initializeLists(){
	// DONT DUPLICATE THE MEMORY (these lists should just have pointers to the elements)

	unstarted.first = processes;
	unstarted.last = &processes[numProcesses - 1];
	unstarted.kind = IS_UNSTARTED;
	unstarted.size = numProcesses;

	int i;
	for(i = 0; i < numProcesses; i++){
		if( i > 0 )
			processes[i].prev = &processes[i - 1];
		
		if( i < numProcesses - 1 )
			processes[i].next = &processes[i + 1];

		processes[i].status = IS_UNSTARTED;
		processes[i].remBurst = 0;
	}


	ready.kind = IS_READY;
	ready.size = 0;

	running.kind = IS_RUNNING;
	running.size = 0;

	blocked.kind = IS_BLOCKED;
	blocked.size = 0;

	terminated.kind = IS_TERMINATED;
	terminated.size = 0;
}

void doRunning(){
	if(running.size){
		int sizeCtr = running.size;
		Process curr = *running.first;

		while(sizeCtr > 0){

			if( !curr.remBurst && curr.C > 0 ){
				// Generate the burst depending on its CPU-time left
				curr.remBurst = curr.C >= curr.B ? randomOS(curr.B) : randomOS(curr.C);
			}

			// Process burst (+update stats)
			curr.C--;
			curr.remBurst--;
			if( !curr.remBurst ){
				if( curr.C )
					moveProcess(&curr, &running, &blocked);
				else
					moveProcess(&curr, &running, &terminated);
			}


			sizeCtr--;
			if(sizeCtr > 0)
				curr = *(curr.next);
		}
	}
}

void doBlocked(){
	if(blocked.size){
		int sizeCtr = blocked.size;
		Process curr = *blocked.first;

		while(sizeCtr > 0){

			if(curr.remBurst == 0){
				// Generate the burst
				curr.remBurst = randomOS(curr.IO);


			}else{
				// Process burst (+update stats)

				curr.remBurst--;
				if(curr.remBurst == 0){
					// Move it to the ready list
					moveProcess(&curr, &blocked, &ready);
				}
			}


			sizeCtr--;
			if(sizeCtr > 0)
				curr = *(curr.next);
		}
	}
}

void doUnstarted(){
	if(unstarted.size){
		int sizeCtr = unstarted.size;
		Process curr = *unstarted.first;

		while(sizeCtr > 0){
			// If it's time for its debut, move it to the ready list
			if(sysClock == curr.A + 1)
				moveProcess(&curr, &unstarted, &ready);
			

			sizeCtr--;
			if(sizeCtr > 0)
				curr = *(curr.next);
		}
	}
}

void doReady(){
	if(ready.size){
		int sizeCtr = ready.size;
		Process curr = *ready.first;

		while(sizeCtr > 0){
			// Move to the running list if it's empty
			if(!running.size)
				moveProcess(&curr, &ready, &running);
			

			sizeCtr--;
			if(sizeCtr > 0)
				curr = *(curr.next);
		}
	}
}

void runSchedule(int schedulingAlgo){
	readInput();
	sortProcessesByArrivalTime();

	initializeLists();
	sysClock = 0;

	printf("This detailed printout gives the state and remaining burst for each process\n\n");

	/*
	printf("UNSTARTED: (%d)\n", unstarted.size);
	printList(unstarted);

	moveProcess(unstarted.first, &unstarted, &ready);
	moveProcess(unstarted.first, &unstarted, &ready);
	moveProcess(unstarted.first, &unstarted, &ready);

	moveProcess(ready.first, &ready, &unstarted);

	printf("UNSTARTED: (%d)\n", unstarted.size);
	printList(unstarted);

	printf("READY: (%d)\n", ready.size);
	printList(ready);
	*/


	while( !isFinished() ){
 
 		doBlocked();
		doRunning();
		doUnstarted();
		doReady();

		printCycle();
		sysClock++;
	}
	
	free(processes);
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

void printProcess(Process proc){

	printf("(%d %d %d %d)", proc.A, proc.B, proc.C, proc.IO);
}

void printList(ProcessList list){
	int sizeCtr = list.size;

	Process proc;
	if(sizeCtr > 0)
		proc = *list.first;

	while(sizeCtr > 0){
		printProcess(proc);
		printf(" ");

		sizeCtr--;
		if(sizeCtr > 0)
			proc = *(proc.next);
	}

	printf("\n\n");
}

int main(int argc, char *argv[]){

	fpRandomNumbers = fopen("random-numbers.txt", "r");
	fpInput = fopen("inputs/input-5.txt", "r");


	runSchedule(USE_UNIPROGRAMMING);


	fclose(fpRandomNumbers);
	fclose(fpInput);

}