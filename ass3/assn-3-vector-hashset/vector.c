#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <search.h>

void VectorNew(vector *v, int elemSize, VectorFreeFunction freeFn, int initialAllocation){
	assert(initialAllocation >= 0 && elemSize > 0);
	v->elemSize = elemSize;
	v->allocLen = initialAllocation;
	if(v->allocLen == 0)v->allocLen = 1;
	v->logLen = 0;
	v->elems = malloc(v->allocLen * v->elemSize);
	v->freefn = freeFn;
}

void VectorDispose(vector *v){
	if(v->freefn != NULL){
		for(int i = 0; i < v->logLen; i++){
			v->freefn((char*)v->elems + i*v->elemSize);
		}
	}
	free(v->elems);
}

int VectorLength(const vector *v){
 	return v->logLen;
}

void *VectorNth(const vector *v, int position){
	assert(position >= 0 && position < v->logLen);
 	return (char*)v->elems + position * v->elemSize; 
}

void VectorReplace(vector *v, const void *elemAddr, int position){
	assert(position >= 0 && position < v->logLen);
	if(v->freefn != NULL)v->freefn((char*)v->elems + position * v->elemSize);
	memcpy((char*)v->elems + position * v->elemSize, elemAddr, v->elemSize);
}

void VectorInsert(vector *v, const void *elemAddr, int position){
	assert(position >= 0 && position <= v->logLen);
	if(position == v->logLen){
		VectorAppend(v, elemAddr);
	}else{
		if(v->logLen == v->allocLen){
			v->elems = realloc(v->elems, 2 * (v->allocLen + 1) * v->elemSize);
			v->allocLen = 2 * (v->allocLen + 1);
		}
		void* source = (char*)v->elems + v->elemSize * position;
		void* target = (char*)source + v->elemSize;
		memmove(target, source, (v->logLen - position) * v->elemSize);
		memcpy(source, elemAddr, v->elemSize);
		v->logLen++;
	}
}

void VectorAppend(vector *v, const void *elemAddr){
	if(v->logLen == v->allocLen){
		v->elems = realloc(v->elems, 2 * (v->allocLen + 1) * v->elemSize);
		v->allocLen = 2 * (v->allocLen + 1);
	}
	void* target = (char*)v->elems + v->elemSize * v->logLen;
	memcpy(target, elemAddr, v->elemSize);
	v->logLen++;
}

void VectorDelete(vector *v, int position){
	if(v == NULL)
	assert(position >= 0 && position < v->logLen);
	void* target = (char*)v->elems + v->elemSize * position;
	if(v->freefn != NULL)v->freefn(target);
	if(position != v->logLen - 1){
		memmove((char*)v->elems + v->elemSize * position,
					 (char*)v->elems + v->elemSize * (position + 1), (v->logLen - position - 1)*v->elemSize);
	}
	v->logLen--;
}

void VectorSort(vector *v, VectorCompareFunction compare){
	assert(compare != NULL);
	qsort(v->elems, v->logLen, v->elemSize, compare);
}

void VectorMap(vector *v, VectorMapFunction mapFn, void *auxData){
	assert(mapFn != NULL);
	for(int i = 0; i < v->logLen; i++){
		void* ptr = (char*)v->elems + i * v->elemSize;
		mapFn(ptr, auxData);
	}
}



static const int kNotFound = -1;
int VectorSearch(const vector *v, const void *key, VectorCompareFunction searchFn, int startIndex, bool isSorted){
	assert(searchFn != NULL && startIndex >= 0 && startIndex <= v->logLen && key != NULL);
	void* toSearch = (char*)v->elems + startIndex * v->elemSize;
 	if(isSorted){
 		void* found = bsearch(key, toSearch, v->logLen - startIndex, v->elemSize, searchFn);
 		if(found == NULL)return kNotFound;
 		return ((char*)found - (char*)v->elems) / v->elemSize;
 	}else{
 		size_t arr_size = v->logLen - startIndex;
 		void* elem = lfind(key, toSearch, &arr_size, v->elemSize, searchFn);
 		if(elem == NULL)return kNotFound;
 		return ((char*)elem - (char*)v->elems) / v->elemSize;
 	}
} 
