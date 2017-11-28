#include<unistd.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

#define KEYSIZE 8
#define DATASIZE 56

typedef struct Record
{
	char key[KEYSIZE];
	char data[DATASIZE];
} Record;

struct ThdArg
{
	int thdNum; //Thread number 0, 1, 2, 3...
	Record *lowRec; //First record of group or first index of record
	Record *hiRec; //Last record of group or last index of record
};

#define RECSIZE sizeof(Record)

void *runner(void *param);

int main(int argc, char *argv[])
{
	int nThreads = atoi(argv[1]);
	FILE *file = fopen(argv[2], "r");
	if(file == NULL)
	{
		fprintf(stderr, "Can't open file\n");
		exit(1);
	}
	int FileSize;
	printf("Num Threads: %d\nFile Name: %s\n", nThreads, argv[2]);
	
	
	//Number of records
	//int nRecs = FileSize / RECSIZE;

	//Number of Records per thread
	//int nRecsPerThd = nRecs / nThreads;

	//n is the number of processors
	int n = sysconf(_SC_NPROCESSORS_ONLN);
	printf("The number of cores is: %d\n", n);

	//Pointer to Array of Thread IDs
	static pthread_t *tids;
	



	fclose(file);
}

void *runner(void *param)
{

}
