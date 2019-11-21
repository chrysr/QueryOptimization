#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "list.h"
#include <ctime>
#include <cstdio>
#include <math.h>

#define MAX_INPUT_FILE_NAME_SIZE 20
#define MAX_INPUT_ARRAYS_NUM 20

typedef class list list;


class tuple
{
public:
    uint64_t key;
    uint64_t payload;
};

const unsigned long BUCKET_SIZE = 64 * pow(2, 10);  //64KB (I think)
const unsigned long TUPLE_SIZE = sizeof(tuple);
const int TUPLES_PER_BUCKET = (int)(BUCKET_SIZE / TUPLE_SIZE);  
//each bucket must be smaller than 64KB 
//size of bucket = num_tuples * sizeof(tuples)  
//num_tuples (of each bucket) = 64KB / sizeof(tuple)

class relation
{
public:
    tuple* tuples;
    uint64_t num_tuples;
    void print();
    relation();
    ~relation();
};

class result
{
public:
    list* lst;
};

class InputArray
{
    public:
    uint64_t rowsNum, columnsNum;
    uint64_t** columns;

    InputArray(uint64_t rowsNum, uint64_t columnsNum);
    ~InputArray();
};

unsigned char hashFunction(uint64_t payload, int shift);
result* join(relation* R, relation* S,uint64_t**r,uint64_t**s,int rsz,int ssz,int joincol);
uint64_t** create_hist(relation*, int);
uint64_t** create_psum(uint64_t**);
relation* re_ordered(relation*,relation*, int);

// functions for bucket sort
void swap(tuple* tuple1, tuple* tuple2);
int randomIndex(int startIndex, int stopIndex);
int partition(tuple* tuples, int startIndex, int stopIndex);
void quickSort(tuple* tuples, int startIndex, int stopIndex);
void sortBucket(relation* rel, int startIndex, int endIndex);
void extractcolumn(relation& rel,uint64_t **array, int column);
InputArray** readArrays();
char** readbatch(int& lns);
char** makeparts(char* query);
void handlequery(char** parts,InputArray** allrelations);
InputArray** loadrelations(char* part,InputArray** allrelations);
InputArray* handlepredicates(InputArray** relations,char* part);
void handleprojection(InputArray* array,char* part);

#endif