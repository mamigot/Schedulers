#include <stdio.h>
#include <stdlib.h>


typedef struct {
	int A, B, C, IO;
} Process;


void printProcesses();

static FILE *fpRandomNumbers;
static FILE *fpInput;

static int numProcesses;
static Process *processes;



Process *Process_create(int A, int B, int C, int IO){
	// Parameters (time units):
	// Arrival, (CPU) Burst, CPU Needed, (I/O) Burst
	Process *proc = malloc(sizeof(Process));

	proc->A = A;
	proc->B = B;
	proc->C = C;
	proc->IO = IO;

	return proc;
}

void readInput(){
	// The first integer of the file gives the total number of processes
	fscanf(fpInput, "%d", &numProcesses);

	// Create the array of processes
	processes = malloc(sizeof(Process) * numProcesses);

	int A, B, C, IO;
	Process curr;
	for(int i = 0; i < numProcesses; i++){
		fscanf(fpInput, " (%d %d %d %d) ", &A, &B, &C, &IO);
		curr = *Process_create(A, B, C, IO);
		processes[i] = curr;
	}

	printProcesses();
}

void printProcesses(){

	Process curr;
	for(int i = 0; i < numProcesses; i++){
		curr = processes[i];
		printf("(%d %d %d %d)\n", curr.A, curr.B, curr.C, curr.IO);
	}
}

int randomOS(int u){
	int curr;
	if(fpRandomNumbers != NULL){
		fscanf(fpRandomNumbers, "%d", &curr);
		return 1 + (curr % u);
	}

	return -1;
}

int main(int argc, char *argv[]){

	fpRandomNumbers = fopen("random-numbers.txt", "r");
	fpInput = fopen("inputs/input-4.txt", "r");

	readInput();



	fclose(fpRandomNumbers);
	fclose(fpInput);

	free(processes);

}