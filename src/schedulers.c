#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Available states
#define IS_BLOCKED 1
#define IS_RUNNING 2
#define IS_UNSTARTED 3
#define IS_READY 4
#define IS_TERMINATED 5

// Scheduling algorithms
#define USE_UNIPROGRAMMING 10
#define USE_FCFS 11
#define USE_SJF 12
#define USE_RR 13

#define QUANTUM_RR 2

#define isFinished() ((unstarted.size || ready.size || running.size || blocked.size) == 0)


typedef struct Process{
	int A, B, C, IO;
	int cCtr;
	// Used to break ties
	int startingPosition;

	int status;
	int remBurst;
	// Applicable to Round Robin (RR)
	int rrBurst;
	// Last time it was in ready state
	int readyTime;

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

typedef struct{
	int finishingTime;
	int totCpuTime;
	int totIoTime;
} SummaryData;


static char *filePath;
static FILE *fpRandomNumbers;
static FILE *fpInput;

static int numProcesses;
static Process *processes;

static SummaryData *sd;
static int sysClock;
static int cpuIsFree = 1;

static ProcessList unstarted;
static ProcessList ready;
static ProcessList running;
static ProcessList blocked;
static ProcessList terminated;

static int verbosity = 0;
void printReport();
void printCycle();
void printProcess();
void printList();

Process *ProcessCreate(int A, int B, int C, int IO){
	// Parameters (time units):
	// Arrival, (CPU) Burst, CPU Needed, (I/O) Burst
	Process *proc = malloc(sizeof(Process));

	proc->A = A;
	proc->B = B;
	proc->C = C;
	proc->cCtr = C;
	proc->IO = IO;

	return proc;
}

int randomOS(int u){
	int curr;
	if(fpRandomNumbers){
		fscanf(fpRandomNumbers, "%d", &curr);
		return 1 + (curr % u);
	}else{
		fputs("Can't read the file with the random numbers! Are you sure it's in this directory?\n", stderr);
		exit(0);
	}
}

void readInput(){
	// Professor expects the random number pointer to reset each time
	fpRandomNumbers = fopen("inputs/random-numbers.txt", "r");
	fpInput = fopen(filePath, "r");

	if(!fpInput){
		fputs("Can't read the input file!\nPlease provide its path as the first argument or as the second, if you are using \"--verbose\".\n", stderr);
		exit(0);
	}

	// The first integer of the file gives the total number of processes
	fscanf(fpInput, "%d", &numProcesses);

	// Create the array of processes
	processes = malloc(sizeof(Process) * numProcesses);

	int A, B, C, IO;
	Process curr;
	int i;
	for(i = 0; i < numProcesses; i++){
		fscanf(fpInput, " ( %d %d %d %d ) ", &A, &B, &C, &IO);
		curr = *ProcessCreate(A, B, C, IO);
		processes[i] = curr;
	}


	printf("The original input was: %d ", numProcesses);

	for(i = 0; i < numProcesses; i++){
		curr = processes[i];
		printf("(%d %d %d %d) ", curr.A, curr.B, curr.C, curr.IO);
	}

	printf("\n");
}

void closeInput(){
	fclose(fpRandomNumbers);
	fclose(fpInput);
}

void sortProcessesByArrivalTime(){
	// Insertion sort

	Process proc;
	int i, j;
	for(i = 0; i < numProcesses; i++){
		j = i;
		while(j - 1 >= 0 && processes[j - 1].A > processes[j].A){
			// Swap
			proc = processes[j];
			processes[j] = processes[j-1];
			processes[j-1] = proc;

			j--;
		}
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

Process *getNthElement(ProcessList *list, int position){

	int ctr = 0;
	Process *proc = list->first;

	while(proc != NULL){
		if(ctr == position)
			return proc;

		ctr++;
		proc = proc->next;
	}

	return NULL;
}

int getShortestJobIndex(ProcessList *list){
	// Determined by the processes' current values of C
	// (the input value C minus the number of cycles this process has run)

	Process *proc = list->first;
	int sjIndex = 0;
	int smallestC = proc->cCtr;

	int size = list->size;
	int ctr = 0;
	while( ctr < size ){

		if(proc->cCtr < smallestC){
			sjIndex = ctr;
			smallestC = proc->cCtr;
		}

		ctr++;
		if(proc->next != NULL)
			proc = proc->next;
	}

	return sjIndex;
}

int verifyOrderOfLinks(ProcessList *list){
	// Checks if the forward and backward links are correct
	Process *proc = list->first;

	if( !list->first || list->size == 0)
		return -1;

	int n = 1;
	while(proc->next != NULL){
		proc = proc->next;
		n++;
	}
	while(proc->prev != NULL){
		proc = proc->prev;
		n--;
	}

	return n == 1;
}

void swap(ProcessList *list, Process *A, Process *B){
	// Swaps two nodes in a linked-list
	if(A == NULL || B == NULL || A->startingPosition == B->startingPosition)
		return;

	int aPos = A->startingPosition;
	int bPos = B->startingPosition;
	int firstPos = list->first->startingPosition;
	int lastPos = list->last->startingPosition;

	Process *temp;

	// Special case: switching the first and last elements
	if((firstPos == aPos && lastPos == bPos) || (firstPos == bPos && lastPos == aPos)){

		list->first->prev = list->last->prev;
		list->last->next = list->first->next;
		list->first->next->prev = list->last;
		list->last->prev->next = list->first;

		temp = list->first;
		list->first = list->last;
		list->last = temp;

	}else{
		// Anything in the middle
		if(firstPos == bPos || lastPos == aPos){
			temp = B;
			B = A;
			A = temp;
		}

		if(firstPos == aPos || firstPos == bPos){
			B->next->prev = A;
			list->first = B;
		}

		if(lastPos == bPos || lastPos == aPos){
			A->prev->next = B;
			list->last = A;
		}


		if(A->prev != NULL)
			A->prev->next = B;

		B->prev = A->prev;
		A->next = B->next;

		if(B->next != NULL)
			B->next->prev = A;

		B->next = A;
		A->prev = B;
	}

	list->first->prev = NULL;
	list->last->next = NULL;
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

		processes[i].startingPosition = i;

		processes[i].status = IS_UNSTARTED;
		processes[i].remBurst = 0;
		processes[i].ioTime = 0;
		processes[i].waitingTime = 0;
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

void updateWaitingTimes(){
	if(ready.size){
		Process *proc = ready.first;
		int size = ready.size;

		while(size > 0){
			proc->waitingTime++;

			size--;
			if(proc->next != NULL){
				proc = proc->next;
			}
		}
	}
}

void sortRemainingByPosition(ProcessList *list, Process *first){
	// Sorts the ending processes (those whose bursts are <= 1)
	// by their starting position in the input
	// (index denotes the index of "first")

	if(first == NULL)
		return;

	int numIteratedElements = 1;
	Process *i, *j;
	i = first;
	j = first->next;
	while(i != NULL){
		while(j != NULL){
			if((i->remBurst == 1 && j->remBurst == 1) && (i->startingPosition > j->startingPosition)){
				swap(list, i, j);
			}

			j = j->next;
		}

		numIteratedElements++;
		i = i->next;
	}
}

void sortByReadyTime(ProcessList *list){
	if(list->size < 2)
		return;

	Process *i, *j;
	i = list->first;
	j = i->next;
	while(i != NULL){
		while(j != NULL){
			if(i->startingPosition > j->startingPosition && i->readyTime == j->readyTime)
				swap(list, i, j);

			j = j->next;
		}

		i = i->next;
	}
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
				transient->readyTime = sysClock;

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

void doReady(int schedulingAlgo){
	// Only mark as "running" if nobody else is running (single processor)
	if(ready.size && !running.size && cpuIsFree){
		Process *chosen;

		if(schedulingAlgo == USE_SJF){
			int sjIndex = getShortestJobIndex(&ready);
			chosen = removeFromList(&ready, sjIndex);

		}else{
			// FIFO (index 0)
			chosen = removeFromList(&ready, 0);
		}

		if(!chosen->remBurst)
			chosen->remBurst = randomOS(chosen->B);

		if(schedulingAlgo == USE_RR)
			chosen->rrBurst = chosen->remBurst < QUANTUM_RR ? chosen->remBurst : QUANTUM_RR;

		chosen->status = IS_RUNNING;
		insertEnd(&running, chosen);
	}
}

void doBlocked(int schedulingAlgo){
	if(blocked.size){
		sd->totIoTime++;

		// Process the burst and, once it's done, move the process to ready
		// (this could apply to multiple processes)
		Process *proc;
		Process *temp;

		proc = blocked.first;

		int ctr = 0;
		while(proc != NULL){
			proc->remBurst--;
			proc->ioTime++;

			if( !proc->remBurst ){
				if(schedulingAlgo == USE_FCFS){
					proc->remBurst++;
					proc->ioTime--;

					sortRemainingByPosition(&blocked, proc);
					proc = getNthElement(&blocked, ctr);

					proc->remBurst--;
					proc->ioTime++;
				}

				temp = proc->next;
				removeFromList(&blocked, ctr);
				proc->status = IS_READY;
				proc->readyTime = sysClock;

				if(schedulingAlgo == USE_UNIPROGRAMMING)
					insertBeginning(&ready, proc);
				else
					insertEnd(&ready, proc);

				cpuIsFree = 1;
				proc = temp;

			}else{
				proc = proc->next;
				ctr++;
			}

		}

	}
}

void doRunning(int schedulingAlgo){
	if(running.size){
		sd->totCpuTime++;

		// Only leave the list when the job is done
		Process *runner = running.first;
		runner->cCtr--;

		if( runner->cCtr ){
			// Still have CPU time left
			runner->remBurst--;

			if( schedulingAlgo == USE_RR )
				runner->rrBurst--;

			if( !runner->remBurst ){
				// Go to IO!
				if(schedulingAlgo == USE_UNIPROGRAMMING)
					cpuIsFree = 0;
				else
					cpuIsFree = 1;

				removeFromList(&running, 0);
				runner->remBurst = randomOS(runner->IO);
				runner->status = IS_BLOCKED;

				if(schedulingAlgo == USE_FCFS)
					insertEnd(&blocked, runner);
				else
					insertBeginning(&blocked, runner);
			}

			else if( schedulingAlgo == USE_RR ){
				if( !runner->rrBurst ){
					removeFromList(&running, 0);
					runner->status = IS_READY;
					runner->readyTime = sysClock;
					insertEnd(&ready, runner);
				}
			}

		}else{
			// Done with the CPU job!
			runner->remBurst = 0;
			cpuIsFree = 1;
			if(schedulingAlgo == USE_RR)
				runner->rrBurst = 0;

			removeFromList(&running, 0);
			runner->status = IS_TERMINATED;
			runner->finishingTime = sysClock - 1;
			insertEnd(&terminated, runner);
		}

	}
}

void runSchedule(int schedulingAlgo){
	readInput();
	sortProcessesByArrivalTime();

	initializeLists();
	sd = calloc(1, sizeof(SummaryData));
	cpuIsFree = 1;
	sysClock = 0;

	if(verbosity)
		printf("This detailed printout gives the state and remaining burst for each process\n\n");

	while( !isFinished() ){

		doBlocked(schedulingAlgo);
		doRunning(schedulingAlgo);
		doUnstarted();

		if(schedulingAlgo == USE_RR)
			sortByReadyTime(&ready);
		doReady(schedulingAlgo);

		updateWaitingTimes();
		if( verbosity && !isFinished() )
			printCycle(schedulingAlgo);

		sysClock++;
	}

	sd->finishingTime = sysClock - 2;
	printReport();

	free(processes);
	free(sd);

	closeInput();
}

void printReport(){

	printf("\n");

	int totTurnaroundTime = 0;
	int totWaitingTime = 0;

	Process proc;
	int i;
	for(i = 0; i < numProcesses; i++){
		proc = processes[i];

		printf("Process %d:\n", i);

		printf("\t (A,B,C,IO) = (%d, %d, %d, %d)\n", proc.A, proc.B, proc.C, proc.IO);

		printf("\t Finishing time: %d\n", proc.finishingTime);
		printf("\t Turnaround time: %d\n", proc.finishingTime - proc.A);
		printf("\t I/O time: %d\n", proc.ioTime);
		printf("\t Waiting time: %d\n", proc.waitingTime);

		printf("\n");

		totTurnaroundTime += proc.finishingTime - proc.A;
		totWaitingTime += proc.waitingTime;
	}

	printf("Summary Data:\n");
	printf("\t Finishing time: %d\n", sd->finishingTime);
	printf("\t CPU Utilization: %f\n", ( sd->totCpuTime * 1.0 / sd->finishingTime ));
	printf("\t I/O Utilization: %f\n", ( sd->totIoTime * 1.0 / sd->finishingTime ));
	printf("\t Throughput: %f processes per hundred cycles\n", (100.0 * numProcesses / sd->finishingTime ));
	printf("\t Average turnaround time: %f\n", ( totTurnaroundTime * 1.0 / numProcesses ));
	printf("\t Average waiting time: %f\n", ( totWaitingTime * 1.0 / numProcesses ));
}

void printCycle(int schedulingAlgo){
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


		if(schedulingAlgo == USE_RR && curr.status != IS_BLOCKED){
			printf("%3d", curr.rrBurst);
		}else
			printf("%3d", curr.remBurst);
	}

	printf(".\n");
}

void printProcess(Process proc){

	printf("(%d %d %d %d -rt: %d -position: %d)", proc.A, proc.B, proc.C, proc.IO, proc.readyTime, proc.startingPosition);
}

void printList(char* name, ProcessList list){

	int sizeCtr = list.size;
	printf("%s, size: (%d)\n", name, sizeCtr);

	Process *proc = list.first;

	while(proc != NULL){
		printProcess(*proc);
		printf(" ");
		proc = proc->next;
	}

	printf("\n\n");
}

int main(int argc, char *argv[]){

	if(strcmp(argv[1], "--verbose") == 0){
		verbosity = 1;
		filePath = argv[2];

	}else{
		verbosity = 0;
		filePath = argv[1];
	}

	printf("FIRST COME FIRST SERVE:\n");
	runSchedule(USE_FCFS);
	printf("\n\n\n");

	printf("ROUND ROBIN:\n");
	runSchedule(USE_RR);
	printf("\n\n\n");

	printf("UNIPROGRAMMING:\n");
	runSchedule(USE_UNIPROGRAMMING);
	printf("\n\n\n");

	printf("SHORTEST JOB FIRST:\n");
	runSchedule(USE_SJF);

}
