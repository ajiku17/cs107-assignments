#include "hashset.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void HashSetNew(hashset *h, int elemSize, int numBuckets,
			HashSetHashFunction hashfn, HashSetCompareFunction comparefn, HashSetFreeFunction freefn){
	h->elemSize = elemSize;
	h->numBuckets = numBuckets;
	h->hashfn = hashfn;
	h->cmpfn = comparefn;
	h->freefn = freefn;
	h->buckets = malloc(numBuckets * sizeof(vector));
	for(int i = 0; i < numBuckets; i++){
		vector* v = h->buckets + i;
		VectorNew(v, h->elemSize, h->freefn, 0/*initial allocation*/);
	}
	h->logLen = 0;
}

void HashSetDispose(hashset *h){

	for(int i = 0; i < h->numBuckets; i++){
		VectorDispose(h->buckets + i);
	}
	free(h->buckets);
}

int HashSetCount(const hashset *h){
 	return h->logLen; 
}

void HashSetMap(hashset *h, HashSetMapFunction mapfn, void *auxData){
	assert(mapfn != NULL);
	for(int i = 0; i < h->numBuckets; i++){
		vector* bucket = h->buckets + i;
		VectorMap(bucket, mapfn, auxData);
	}
}

void HashSetEnter(hashset *h, const void *elemAddr){
	assert(elemAddr != NULL);
	int bucketN = h->hashfn(elemAddr, h->numBuckets);
	assert(bucketN >= 0 && bucketN < h->numBuckets);
	vector* bucket = h->buckets + bucketN;
	int found = VectorSearch(bucket, elemAddr, h->cmpfn, 0, false);
	if(found == -1){
		VectorAppend(bucket, elemAddr);
		h->logLen++;
	}else{
		VectorReplace(bucket, elemAddr, found);
	}
}

void *HashSetLookup(const hashset *h, const void *elemAddr){
 	assert(elemAddr != NULL);;
	int bucketN = h->hashfn(elemAddr, h->numBuckets);
	assert(bucketN >= 0 && bucketN < h->numBuckets);
	vector* bucket = h->buckets + bucketN;
	int found = VectorSearch(bucket, elemAddr, h->cmpfn, 0, false);
	if(found == -1)return NULL;
	return VectorNth(bucket, found);

}
