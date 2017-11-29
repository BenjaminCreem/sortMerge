#include<unistd.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>

/**
 * Author: Benjamin Creem
 * This program sorts a file using merge sort and threads
 */

#define KEYSIZE 8
#define DATASIZE 56

typedef struct Record
{
	char key[KEYSIZE];
	char data[DATASIZE];
} Record;

typedef struct ThdArg
{
	Record *array;
	int tid; //Thread number 0, 1, 2, 3...
	int lowRec; //First record of group or first index of record
	int hiRec; //Last record of group or last index of record
} ThdArg;

#define RECSIZE sizeof(Record)

void *runner(void *param);
int compare(const void *a, const void *b);
void mergesort(Record *array, int arrayLength);

int curNumThreads;
int minThreadSize;
pthread_mutex_t threadLock;

int main(int argc, char *argv[])
{
	int nThreads = atoi(argv[1]);
	FILE *file = fopen(argv[2], "r");
	if(file == NULL)
	{
		fprintf(stderr, "Can't open file\n");
		exit(1);
	}
	fseek(file, 0L, SEEK_END);
	int FileSize = ftell(file);
	rewind(file);
	printf("Num Threads: %d\nFile Name: %s\nFile Size: %d\n", 
	nThreads, argv[2], FileSize);
	
	
	//Number of records
	int nRecs = FileSize / RECSIZE;
	printf("The number of records is: %d\n", nRecs);
	
	//Number of Records per thread
	int nRecsPerThd = nRecs / nThreads;
	printf("The number of records per thread is: %d\n", nRecsPerThd);

	//n is the number of processors
	int n = sysconf(_SC_NPROCESSORS_ONLN);
	printf("The number of cores is: %d\n", n);


	Record recs[nRecs];
	Record newRec;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int recNum = 0;
	for(int i = 0; i < nRecs; i++)
	{
		read = getline(&line, &len, file);
		strncpy(newRec.key, line, KEYSIZE);
		strncpy(newRec.data, line+KEYSIZE, KEYSIZE+DATASIZE);
		//printf("%.*s\n", KEYSIZE, newRec.key);
		recs[recNum] = newRec;
		recNum++;
	}

	printf("Before Sorting\n");
	for(int i = 0; i < nRecs; i++)
	{
		printf("%.*s%.*s \n", KEYSIZE, recs[i].key, DATASIZE,recs[i].data);
	}
	
	minThreadSize = nRecsPerThd;

	//Pointer to tid
	pthread_t tid;
	//set of thread attributes
	pthread_attr_t attr; 

	//We are now ready to divide the problem into multiple threads. 
	//The first step is to copy the file's entries into Records, and then
	//put those records into nThreads arrays. These arrays are then
	//sorted in threads using qsort, before they are joined up with other
	//threads and arrays and merged back together. This process repeats
	//until the final sorted array is stored in the 0th array. 
	
	mergesort(recs, nRecs);
	
	printf("After Sorting\n");
	for(int i = 0; i < nRecs; i++)
	{
		printf("%.*s%.*s \n", KEYSIZE, recs[i].key, DATASIZE, recs[i].data);
	}

	fclose(file);
}

Record* merge(Record *array, int low, int hi, int tid)
{
	printf("Merging\n");
	int center = 0;
	int i = low;
	int mid = low + ((hi - low)/2);
	int j = mid + 1;
	Record *c = (Record *) malloc((hi-low+1) * sizeof(Record));
	while(i <= mid && j <= hi)
	{
		if(array[i].key <= array[j].key)
		{
			j++;
			c[center] = array[j];
		}
		else
		{
			center++;
			j++;
			c[center] = array[j];
		}
	}
	

	if(i == mid + 1)
	{
		while(j <= hi)
		{
			center++;
			j++;
			c[center] = array[j];
		}
	}
	else
	{
		while(i <= mid)
		{
			center++;
			j++;
			c[center] = array[i];
		}
	}

	//Copy back
	i = low;
	center = 0;
	while(i <= hi)
	{
		i++;
		center++;
		array[i] = c[center];
	}

	free(c);
	
}

void mergesort(Record *array, int arrayLength)
{
	ThdArg threadArgument;
	threadArgument.array = array;
	threadArgument.lowRec = 0;
	threadArgument.hiRec = arrayLength;
	//Shared data
	curNumThreads = 0;
	pthread_mutex_init(&threadLock, NULL);
	threadArgument.tid = 0;

	//Starting threads
	pthread_t thread;
	printf("Initial Thread Created\n");
	pthread_create(&thread, NULL, runner, &threadArgument);
	pthread_join(thread, NULL);
}

void *runner(void *param)
{
	ThdArg *thdArg = (ThdArg *) param;
	int l = thdArg->lowRec;
	int h = thdArg->hiRec;
	int t = thdArg->tid;
	
	//If we can't create anymore threads to do work, qsort
	if(h - 1 + 1 <= minThreadSize)
	{
		qsort(thdArg->array+1, h-l+1, sizeof(ThdArg), compare);
	}
	//If the array is big enough, we can split it into smaller arrays
	//And qsort those in separate threads before returning here
	else
	{
		//First Thread Info
		int m = l + ((h-1)/2);
		ThdArg firstArg;
		firstArg.lowRec = l;
		firstArg.hiRec = h;
		firstArg.array = thdArg->array;
		pthread_mutex_lock(&threadLock);
		firstArg.tid = curNumThreads++;
		pthread_mutex_unlock(&threadLock);
		
		//First thread
		pthread_t firstThread;
		pthread_create(&firstThread, NULL, runner, &firstArg);
		//int firstThreadDone = 1;
		
		//Second Thread Info
		ThdArg secArg;
		secArg.lowRec = m + 1;
		secArg.hiRec = h;
		secArg.array = thdArg->array;
		pthread_mutex_lock(&threadLock);
		secArg.tid = curNumThreads++;
		pthread_mutex_unlock(&threadLock);

		//Second thread
		pthread_t secondThread;
		pthread_create(&secondThread, NULL, runner, &secArg);
		//int secondThreadDone = 1;

		pthread_join(firstThread, NULL);
		pthread_join(secondThread, NULL);

		merge(thdArg->array, l, h, t);	
	}

	pthread_exit(NULL);
}







int compare(const void *a, const void *b)
{
	Record *recA = (Record *)a;
	Record *recB = (Record *)b;
	return (recB->key - recA->key);
}






