#include<unistd.h>
#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>

/**
 * Author: Benjamin Creem
 * Date: December 10th, 2017
 * This program sorts a file using merge sort and threads
 * The file must have each line with keys length 8 and 
 * Data that comes after the key on that same kine with length 56
 *
 * Input: argv[2] = number of threads (works with 1, 2, 4, 8)
 * Input: argv[3] = filename
 * Output: The file that the user entered, but sorted
 * Example: ./sortMerge 4 data_128
 * 
 * PreProcessing/Dependencies: 
 * Look at the #includes
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
	FILE *file = fopen(argv[2], "r+");
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


	Record *recs;
	recs = mmap(NULL, FileSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileno(file), 0);
	
	minThreadSize = nRecsPerThd;

	//We are now ready to merge sort the memory mapped file
	
	mergesort(recs, nRecs);
	
	munmap(recs, FileSize);
	fclose(file);
}


/**
 * Merge two halves of the same array in a ThdArg
 */
void* merge(ThdArg thdArg, int low, int hi, int tid)
{
	int counter = 0;
	int i = low;
	int mid = low + ((hi-low)/2);
	int j = mid + 1;
	Record *c = (Record *)malloc(((hi-low)+1) * sizeof(Record));
	
	while(i <= mid && j <= hi)
	{
		if(compare(thdArg.array[i].key, thdArg.array[j].key) < 0)
		{
			c[counter] = thdArg.array[i];
			counter++;
			i++;
		}
		else
		{
			c[counter] = thdArg.array[j];
			counter++;
			j++;
		}	
	}


	if(i == mid + 1)
	{
		while(j <= hi)
		{
			c[counter] = thdArg.array[j];
			counter++;
			j++;
		}
	}
	else
	{
		while(i <= mid)
		{
			c[counter] = thdArg.array[i];
			counter++;
			i++;
		}
	}


	i = low;
	counter = 0;
	while(i <= hi)
	{
		thdArg.array[i] = c[counter];
		i++;
		counter++;
 	}
	free(c);
}

/**
 * This function sorts array with length arrayLength
 */
void mergesort(Record *array, int arrayLength)
{
	ThdArg threadArgument;
	threadArgument.array = array;
	threadArgument.lowRec = 0;
	threadArgument.hiRec = arrayLength-1;
	//Shared data
	numThreads = 0;
	pthread_mutex_init(&lockNumThreads, NULL);
	threadArgument.tid = 0;
	//Starting primary thread
	pthread_t thread;
	pthread_create(&thread, NULL, runner, &threadArgument);
	pthread_join(thread, NULL);
}

/*
 * This is the thread that will be created when a thread needs to be split up
 * It gets passed a thread argument that is then used to sort everything
 */
void *runner(void *param)
{
	ThdArg *thdArg = (ThdArg *) param;
	int t = thdArg->tid;
	
	//If we can't create anymore threads to do work, qsort
	if(thdArg->hiRec - thdArg->lowRec <= minThreadSize)
	{
		qsort(&(thdArg->array[thdArg->lowRec]), (thdArg->hiRec - thdArg->lowRec) + 1, sizeof(Record), compare);
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
		merge(*thdArg, thdArg->lowRec, thdArg->hiRec, t);
	}
	
}

/**
 * This function is used to compare keys 
 */
int compare(const void *a, const void *b)
{
	Record recA = *(Record *)a;
	Record recB = *(Record *)b;
	return strncmp(recA.key, recB.key, KEYSIZE);
}






