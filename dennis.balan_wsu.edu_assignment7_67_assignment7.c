#include "assignment7.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#define SORT_THRESHOLD      40
#define THREAD_THRESHOLD   500 
//thread_working is the function that recurively quicksorts a struct SortParams (address). It is used inside threads
void *thread_working(void *);
typedef struct _sortParams {
    char** array;
    int left;
    int right;
} SortParams;
static int maximumThreads;  /* maximum # of threads to be used */
//counter is number of threads in use
int counter;
//mtx is the mutex thread that controls when the counter for the number of threads is changed
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
/* This is an implementation of insert sort, which although it is */
/* n-squared, is faster at sorting short lists than quick sort,   */
/* due to its lack of recursive procedure call overhead.          */
static void insertSort(char** array, int left, int right) {
    int i, j;
    for (i = left + 1; i <= right; i++) {
        char* pivot = array[i];
        j = i - 1;
        while (j >= left && (strcmp(array[j],pivot) > 0)) {
            array[j + 1] = array[j];
            j--;
        }
        array[j + 1] = pivot;
    }
}
/* Recursive quick sort, but with a provision to use */
/* insert sort when the range gets small.            */
static void quickSort(void* p) {
    SortParams* params = (SortParams*) p;
    char** array = params->array;
    int size = sizeof(array)/sizeof(array[0]);
    int left = params->left;
    int right = params->right;
    int i = left, j = right;
    if (j - i > SORT_THRESHOLD) {           /* if the sort range is substantial, use quick sort */
        int m = (i + j) >> 1;               /* pick pivot as median of         */
        char* temp, *pivot;                 /* first, last and middle elements */
        if (strcmp(array[i],array[m]) > 0) {
            temp = array[i]; array[i] = array[m]; array[m] = temp;
        }
        if (strcmp(array[m],array[j]) > 0) {
            temp = array[m]; array[m] = array[j]; array[j] = temp;
            if (strcmp(array[i],array[m]) > 0) {
                temp = array[i]; array[i] = array[m]; array[m] = temp;
            }
        }
        pivot = array[m];
        for (;;) {
            while (strcmp(array[i],pivot) < 0) i++; /* move i down to first element greater than or equal to pivot */
            while (strcmp(array[j],pivot) > 0) j--; /* move j up to first element less than or equal to pivot      */
            if (i < j) {
                char* temp = array[i];      /* if i and j have not passed each other */
                array[i++] = array[j];      /* swap their respective elements and    */
                array[j--] = temp;          /* advance both i and j                  */
            } else if (i == j) {
                i++; j--;
            } else break;                   /* if i > j, this partitioning is done  */
        }
	//create a thread that will sort right side of the partition
	pthread_t thread;
	//create a check flag to see if a thread was created
       	int check = 0;	
        SortParams first;  first.array = array; first.left = left; first.right = j;
	/* sort the left partition  */
        SortParams second; second.array = array; second.left = i; second.right = right;
	//if there is space for more threads, create a thread that qsort the right,else quickSort recursively without a thread
	if((counter <= maximumThreads)){
		//set check to 1 to indicate thread is created
		check = 1;
		//lock the counter for the number of threads to prevent race conditions, and increment the thread counter to show that a thread is created
		int lock_error = pthread_mutex_lock(&mtx);
		if(lock_error == -1){
			int error = errno;
			fprintf(stderr,"Error code %d: %s\n",error,strerror(error));
		}
		counter++;
		int unlock_error = pthread_mutex_unlock(&mtx);
		if(unlock_error == -1){
			int error = errno;
			fprintf(stderr,"Errpr code %d: %s\n",error,strerror(error));
		}
		//create thread to sort the right sight of the partition, or first
		//thread_working is the function that recursovely sorts param, input is address  of first
		int thread_error = pthread_create(&thread,NULL,thread_working,&first);
		if(thread_error == -1){
			int error = errno;
			fprintf(stderr, "Error # %d = %s\n",error,strerror(error));
		}
	}
	//recursive non_thread quick_sort of right partition
	else{
        	quickSort(&first);
	}	/* sort the right partition */
	//create a thread named thread2 that will sort the left side of the partition
	pthread_t thread2;
	//create a flag variable that checks if thread2 was created
	int check2 = 0;
	//if there is space for more threads, create a thread that qsorts the left, else quicksort the left recuresively without a thread
	if((counter <= maximumThreads)){
		//set check2 to 1 to indicated thread2 is created
		check2 = 1;
		//lock the counter for the number of threadas to prevent race conditions, and increment the thread counter to show that a thread is created
		int lock_error = pthread_mutex_lock(&mtx);
		if(lock_error == -1){
			int error = errno;
			fprintf(stderr,"Error # %d = %s\n",error,strerror(error));
		}
		counter++;
		int unlock_error = pthread_mutex_unlock(&mtx);
		if(unlock_error == -1){
			int error = errno;
			fprintf(stderr,"Error $ %d = %s\n",error,strerror(error));
		}
		//thread_working is the function that recursively sorts param, its input is second 
		int thread_error = pthread_create(&thread2,NULL,thread_working,&second);
	}else{//recursive non-thread quickSort call for left side, or second
     		quickSort(&second);
	}
	//if check is 1, (thread was created),join thread and wait for it to finish/get destroyed
	if(check == 1){
		int join_error = pthread_join(thread,NULL);
		if(join_error == -1){
			int error = errno;
			fprintf(stderr,"Error %d = %s\n",error,strerror(error));
		}
	}
	//if check2 is 1 (thread2 was created), join thread and wait for it to finish/get destroyed
	if(check2 == 1){
		int join_error = pthread_join(thread2,NULL);
		if(join_error == -1){
			int error = errno;
			fprintf(stderr,"Error %d = %s\n",error, strerror(error));
		}
	}
    } else insertSort(array,i,j);           /* for a small range use insert sort */
}
//thread_working is the function that a thread uses to recursively quickSort sortParams. It also decrements the thread counter counter after a call to quickSort since the thread has stopped working and there is more space for the thread
void *thread_working(void *p){
	quickSort(p);
	//lock counter to prevent race conditions
	int lock_error = pthread_mutex_lock(&mtx);
	if(lock_error == -1){
		int error = errno;
		fprintf(stderr,"Error %d = %s\n",error, strerror(error));
	}
	counter--;
	int unlock_error = pthread_mutex_unlock(&mtx);
	if(unlock_error == -1){
		int error = errno;
		fprintf(stderr, "Error %d = %s\n",error, strerror(error));
	}
	//thread_working does nothing and returns a NULL as such
	return NULL;
}
/* user interface routine to set the number of threads sort is permitted to use*/
void setSortThreads(int count) {
    maximumThreads = count;
    counter = 0;
}
/* user callable sort procedure, sorts array of count strings, beginning at address array */
void sortThreaded(char** array, unsigned int count) {
    SortParams parameters;
    parameters.array = array; parameters.left = 0; parameters.right = count - 1;
    quickSort(&parameters);
    //destroy the mutex mtx after it has been used to prevent thread cluttering
    int mtx_error = pthread_mutex_destroy(&mtx);
    if(mtx_error == -1){
	    int error = errno;
	    fprintf(stderr,"Error %d = %s\n",error, strerror(error));
    }
}
