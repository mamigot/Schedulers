#include <stdio.h>
#include <stdlib.h>


FILE *fpRandomNumbers;
FILE *fpInput;

int randomOS(int u){
	int curr;
	fscanf(fpRandomNumbers, "%d", &curr);

	return 1 + (curr % u);
}

int readInput(){
	// The first integer of the input gives the total number of processes that must be read
	int numProcesses;
	fscanf(fpInput, "%d", &numProcesses);

	return numProcesses;
}

int main(int argc, char *argv[]){

	fpRandomNumbers = fopen("random-numbers.txt", "r");
	fpInput = fopen("inputs/input-7.txt", "r");

	//printf("%d\n", randomOS(591));
	//printf("%d\n", randomOS(132));

	printf("%d\n", readInput());


}