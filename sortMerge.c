#include<unistd.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>

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


int numThreads;
int minThreadSize;
pthread_mutex_t lockNumThreads;

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

	//printf("Before Sorting\n");
	//for(int i = 0; i < nRecs; i++)
	//{
	//	printf("%.*s%.*s \n", KEYSIZE, recs[i].key, DATASIZE,recs[i].data);
	//}
	
	minThreadSize = nRecsPerThd;

	//We are now ready to divide the problem into multiple threads. 
	//The first step is to copy the file's entries into Records, and then
	//put those records into nThreads arrays. These arrays are then
	//sorted in threads using qsort, before they are joined up with other
	//threads and arrays and merged back together. This process repeats
	//until the final sorted array is stored in the 0th array. 
	
	mergesort(recs, nRecs);
	
	//printf("After Sorting\n");
	//for(int i = 0; i < nRecs; i++)
	//{
	//	printf("%.*s%.*s \n", KEYSIZE, recs[i].key, DATASIZE, recs[i].data);
	//}

	fclose(file);
}


/**
 * Merge two halves of the same array in a ThdArg
 */






void* merge(ThdArg thdArg, int low, int hi, int tid)
{
	printf("Merging\n");
	int counter = 0;
	int i = low;
	int mid = low + ((hi-low)/2);
	int j = mid + 1;
	Record *c = (Record *)malloc((hi-low+1) * sizeof(Record));
	
	while(i <= mid && j <= hi)
	{
		if(strcmp(thdArg.array[i].key, thdArg.array[j].key))
		{
			counter++;
			i++;
			c[counter] = thdArg.array[i];
		}
		else
		{
			counter++;
			j++;
			c[counter] = thdArg.array[j];
		}	
	}


	if(i == mid + 1)
	{
		while(j <= hi)
		{
			counter++;
			j++;
			c[counter] = thdArg.array[j];
		}
	}
	else
	{
		while(i <= mid)
		{
			counter++;
			i++;
			c[counter] = thdArg.array[i];
		}
	}


	i = low;
	counter = 0;
	while(i <= hi)
	{
		i++;
		counter++;
		thdArg.array[i] = c[counter];
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
	numThreads = 0;
	pthread_mutex_init(&lockNumThreads, NULL);
	threadArgument.tid = 0;

	//Starting primary thread
	pthread_t thread;
	printf("Initial Thread Created\n");
	pthread_create(&thread, NULL, runner, &threadArgument);
	pthread_join(thread, NULL);
}

void *runner(void *param)
{
	ThdArg *thdArg = (ThdArg *) param;
	int t = thdArg->tid;
	
	//If we can't create anymore threads to do work, qsort
	//I guess minThreadSize should be called minArraySize,
	//I will change that later
	if(thdArg->hiRec - thdArg->lowRec <= minThreadSize)
	{
		//printf("%d   %d", minThreadSize ,thdArg->hiRec-thdArg->lowRec);
		//printf("\n\n\n\n\nBefore QSorting\n");
		//for(int i = thdArg->lowRec+1; i < thdArg->hiRec; i++)
		//{
		//	printf("%.*s%.*s \n", KEYSIZE, thdArg->array[i].key, DATASIZE, thdArg->array[i].data);
		//}
		printf("qsorting");
		qsort(&(thdArg->array[thdArg->lowRec]), (thdArg->hiRec - thdArg->lowRec) + 1, sizeof(Record), compare);

		//printf("\n\n\n\n\nAfter QSorting\n");
		//for(int i = thdArg->lowRec+1; i < thdArg->hiRec; i++)
		//{
		//	printf("%.*s%.*s \n", KEYSIZE, thdArg->array[i].key, DATASIZE, thdArg->array[i].data);
		//}
	}
	else //Array needs to be split into two smaller arrays in two separate threads
	{
		int mid = (thdArg->hiRec + thdArg->lowRec)/2;
		//Create two separate threads
		ThdArg firstThread;
		firstThread.lowRec = thdArg->lowRec;
		firstThread.hiRec = mid;
		firstThread.array = thdArg->array;
		//Lock number of threads so we can get the tid
		pthread_mutex_lock(&lockNumThreads);
		firstThread.tid = numThreads++;
		pthread_mutex_unlock(&lockNumThreads);
		//Make first Thread
		pthread_t thread0;
		pthread_create(&thread0, NULL, runner, &firstThread);
		//Stuff for second thread
		ThdArg secThread;
		secThread.lowRec = mid + 1;
		secThread.hiRec = thdArg->hiRec;
		secThread.array = thdArg->array;
		pthread_mutex_lock(&lockNumThreads);
		secThread.tid = numThreads++;
		pthread_mutex_unlock(&lockNumThreads);
		//Making Second Thread
		pthread_t thread1;
		pthread_create(&thread1, NULL, runner, &secThread);
		//Wait for threads		
		pthread_join(thread0, NULL);	
		pthread_join(thread1, NULL);
		printf("\nBefore Merging\n");
		for(int i = thdArg->lowRec; i <= mid; i++)
		{
			printf("%.*s%.*s \n", KEYSIZE, thdArg->array[i].key, DATASIZE, thdArg->array[i].data);
		}
		merge(*thdArg, thdArg->lowRec, thdArg->hiRec, t);
		printf("After Merging\n");
		for(int i = mid+1; i < thdArg->hiRec; i++)
                {
                        printf("%.*s%.*s \n", KEYSIZE, thdArg->array[i].key, DATASIZE, thdArg->array[i].data);

                }
	}
	
}




int compare(const void *a, const void *b)
{
	Record recA = *(Record *)a;
	Record recB = *(Record *)b;
	return strcmp(recA.key, recB.key);
}






