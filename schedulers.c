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

#define isFinished() ( (unstarted.size || ready.size || running.size || blocked.size) == 0)
#define getBurstCPU(proc) randomOS(proc->B)
#define getBurstIO(proc) randomOS(proc->IO)


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
static int cpuIsFree = 1;

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

Process *removeFromList(ProcessList *list, int index){
	// Removes the element corresponding to the index from the list (takes O(n))

	int size = list->size;
	if(index >= size || index < 0)
		return NULL;

	Process *proc = list->first;
	int counter = 0;

	while(counter < index && proc->next != NULL){
		proc = proc->next;
		counter++;
	}
		
	// proc is the process we want to delete
	if(size == 1){
		list->first = NULL;
		list->last = NULL;

	}else if(counter == 0 && proc->next != NULL){
		// First element
		proc->next->prev = NULL;
		list->first = proc->next;

	}else if(counter == size - 1 && proc->prev != NULL){
		// Last element
		proc->prev->next = NULL;
		list->last = proc->prev;

	}else{
		// Any element in the middle
		proc->prev->next = proc->next;
		proc->next->prev = proc->prev;

	}

	proc->next = NULL;
	proc->prev = NULL;
	list->size--;

	return proc;
}

void insertBeginning(ProcessList *list, Process *proc){
	// DEBUG
	if(proc->next != NULL || proc->prev != NULL)
		printf("\n\n(!) inserting an element whose front or back pointer isn't null!\n\n");

	// Inserts the process at the beginning of the list

	int size = list->size;

	if(size == 0){
		list->first = proc;
		list->last = proc;

	}else{
		list->first->prev = proc;
		proc->next = list->first;
		list->first = proc;
	}

	proc->prev = NULL;
	list->size++;
}

void insertEnd(ProcessList *list, Process *proc){
	// DEBUG
	if(proc->next != NULL || proc->prev != NULL)
		printf("\n\n(!) inserting an element whose front or back pointer isn't null!\n\n");


	// Inserts the process at the end of the given list

	int size = list->size;

	if(size == 0){
		list->first = proc;
		list->last = proc;

	}else{
		list->last->next = proc;
		proc->prev = list->last;
		list->last = proc;
	}

	proc->next = NULL;
	list->size++;
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

void doUnstarted(){
	if(unstarted.size){
		// Go through the unstarted elements and possibly move them to the ready list
		// (they debut when sysClock == curr->A + 1)
		Process *proc = unstarted.first;
		Process *transient;
		int counter = 0;

		while(counter < unstarted.size){
			if(sysClock >= proc->A + 1){
				transient = removeFromList(&unstarted, counter);
				insertEnd(&ready, transient);
				transient->status = IS_READY;

				// Reset the count after the unstarted list is modified
				proc = unstarted.first;
				counter = 0;

			}else{
				if(proc->next != NULL)
					proc = proc->next;
				counter++;
			}
		}
	}
}

void doReady(){
	// Only mark as "running" if nobody else is running
	if(ready.size && !running.size && cpuIsFree){
		// Ready is FIFO (index 0)
		Process *chosen = removeFromList(&ready, 0);

		// Get the burst
		chosen->remBurst = getBurstCPU(chosen);
		chosen->status = IS_RUNNING;
		insertEnd(&running, chosen);
	}
}

void doBlockedUniprogramming(){
	if(blocked.size){
		// Process the burst and, once it's done, move the process back to running
		// Will only be one element here
		Process *proc = blocked.first;
		proc->remBurst--;

		if( !proc->remBurst ){
			removeFromList(&blocked, 0);
			proc->status = IS_READY;
			insertBeginning(&ready, proc);
			cpuIsFree = 1;
		}
	}
}

void doBlocked(schedulingAlgo){
	if(schedulingAlgo == USE_UNIPROGRAMMING)
		doBlockedUniprogramming();
}

void doRunningUniprogramming(){
	if(running.size){
		// Only leave the list when the job is done
		Process *runner = running.first;
		runner->C--;

		if( runner->C ){
			// Still have CPU time left
			runner->remBurst--;

			if( !runner->remBurst ){
				// Go to IO!
				// (don't add anything to running in the meantime)
				cpuIsFree = 0;

				removeFromList(&running, 0);
				runner->remBurst = getBurstIO(runner);
				runner->status = IS_BLOCKED;
				insertBeginning(&blocked, runner);
			}

		}else{
			// Done with the CPU job!
			runner->remBurst = 0;
			cpuIsFree = 1;

			removeFromList(&running, 0);
			runner->status = IS_TERMINATED;
			insertEnd(&terminated, runner);
		}

	}
}

void doRunning(schedulingAlgo){
	if(schedulingAlgo == USE_UNIPROGRAMMING)
		doRunningUniprogramming();
}

void runSchedule(int schedulingAlgo){
	readInput();
	sortProcessesByArrivalTime();

	initializeLists();
	cpuIsFree = 1;
	sysClock = 0;

	printf("This detailed printout gives the state and remaining burst for each process\n\n");

	while( !isFinished() ){

 		doBlocked(schedulingAlgo);
		doRunning(schedulingAlgo);
		doUnstarted();
		doReady();

		if( !isFinished() )
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

		else if(curr.status == IS_TERMINATED)
			printf("%11s", "terminated");



		printf("%3d", curr.remBurst);
	}

	printf(".\n");
}

void printProcess(Process proc){

	printf("(%d %d %d %d)", proc.A, proc.B, proc.C, proc.IO);
}

void printList(char* name, ProcessList list){
	int sizeCtr = list.size;
	printf("%s, size: (%d)\n", name, sizeCtr);

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