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

#define QUANTUM_RR 2

#define isFinished() ((unstarted.size || ready.size || running.size || blocked.size) == 0)


typedef struct Process{
	int A, B, C, IO;
	int cCtr;

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

typedef struct{
	int finishingTime;
	int totCpuTime;
	int totIoTime;
} SummaryData;


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

void doReady(schedulingAlgo){
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

		chosen->remBurst = randomOS(chosen->B);

		chosen->status = IS_RUNNING;
		insertEnd(&running, chosen);
	}
}

void doBlocked(schedulingAlgo){
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
				temp = proc->next;

				removeFromList(&blocked, ctr);
				proc->status = IS_READY;

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

void doRunning(schedulingAlgo){
	if(running.size){
		sd->totCpuTime++;

		// Only leave the list when the job is done
		Process *runner = running.first;
		runner->cCtr--;

		if( runner->cCtr ){
			// Still have CPU time left
			runner->remBurst--;

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

		}else{
			// Done with the CPU job!
			runner->remBurst = 0;
			cpuIsFree = 1;

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

	printf("This detailed printout gives the state and remaining burst for each process\n\n");

	while( !isFinished() ){

		printList("blocked list", blocked);
 		doBlocked(schedulingAlgo);

		doRunning(schedulingAlgo);
		doUnstarted();
		printList("ready list", ready);

		doReady(schedulingAlgo);




		updateWaitingTimes();

		if( !isFinished() )
			printCycle();

		if(sysClock >= 937)
			break;

		sysClock++;
	}

	sd->finishingTime = sysClock - 2;
	printReport();
	
	free(processes);
	free(sd);
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

	printf("(%d %d %d %d -rem: %d -stat: %d)", proc.A, proc.B, proc.C, proc.IO, proc.cCtr, proc.status);
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
	fpInput = fopen("inputs/input-4.txt", "r");


	runSchedule(USE_FCFS);


	fclose(fpRandomNumbers);
	fclose(fpInput);

}