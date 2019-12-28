#ifndef _FUNCTIONS_H
#define _FUNCTIONS_H
#include <iostream>
#include <cstdlib>
#include <cstring>
#include "list.h"
#include <ctime>
#include <cstdio>
#include <math.h>
#include "JobScheduler.h"
#include <inttypes.h>


#define MAX_INPUT_FILE_NAME_SIZE 100
#define MAX_INPUT_ARRAYS_NUM 20

enum Type {serial = 0, parallel = 1}; 

typedef class list list;

extern pthread_mutex_t *predicateJobsDoneMutexes;
extern pthread_cond_t *predicateJobsDoneConds;
extern pthread_cond_t *jobsCounterConds;
// pthread_mutex_t* queryJobDoneMutexes;
// pthread_cond_t* queryJobDoneConds;
static pthread_mutex_t queryJobDoneMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queryJobDoneCond = PTHREAD_COND_INITIALIZER;

extern bool** lastJobDoneArrays;
extern bool* queryJobDoneArray;
extern char** QueryResult;
extern JobScheduler *scheduler;

class tuple
{
public:
    uint64_t key;
    uint64_t payload;
};

const unsigned long BUCKET_SIZE = 64 * pow(2, 10);  //64KB (I think)
const unsigned long TUPLE_SIZE = sizeof(tuple);
const int TUPLES_PER_BUCKET = (int)(BUCKET_SIZE / TUPLE_SIZE);  
//const int TUPLES_PER_BUCKET=10;
const uint64_t power=pow(2,8);
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
    InputArray(uint64_t rowsNum);  // initialization for storing row ids
    ~InputArray();

    InputArray* filterRowIds(uint64_t fieldId, int operation, uint64_t numToCompare,const InputArray* pureInputArray); // filtering when storing row ids
    InputArray* filterRowIds(uint64_t field1Id, uint64_t field2Id,const InputArray* pureInputArray); // inner join
    void extractColumnFromRowIds(relation& rel, uint64_t fieldId,const InputArray* pureInputArray); // column extraction from the initial input array (pureInputArray)
    void print();
};

class IntermediateArray {
    public:
    uint64_t** results; // contains columns of rowIds: one column per used input array
    int* inputArrayIds; // size = columnsNum
                        // contains ids of input arrays that correspond to each column
    int* predicateArrayIds; // size = columnsNum
                            // contains ids of predicate arrays that correspond to each column
    uint64_t columnsNum;
    uint64_t rowsNum;

    IntermediateArray(uint64_t columnsNum);
    ~IntermediateArray();

    void extractFieldToRelation(relation* resultRelation, const InputArray* inputArray, int predicateArrayId, uint64_t fieldId);
    void populate(uint64_t** intermediateResult, uint64_t rowsNum, IntermediateArray* prevIntermediateArray, int inputArray1Id, int inputArray2Id, int predicateArray1Id, int predicateArray2Id);
    bool hasInputArrayId(int inputArrayId);
    uint64_t findColumnIndexByInputArrayId(int inputArrayId);
    uint64_t findColumnIndexByPredicateArrayId(int predicateArrayId);
    IntermediateArray* selfJoin(int inputArray1Id, int inputArray2Id, uint64_t field1Id, uint64_t field2Id, const InputArray* inputArray1, const InputArray* inputArray2);
    void print();
};

typedef class histogram histogram;

class bucket
{
public:
    relation *rel;
    histogram *hist;
    int shift;
    bucket *prev;
    int index;  //keep where i am left
    ~bucket();
};

class histogram
{
public:
    uint64_t *hist;
    uint64_t *psum;
    bucket **next;
    ~histogram();
};

void handlequery(char** ,const InputArray** , int);
void tuplereorder_parallel(tuple*, tuple*, int, int, bool, int, int);
void quickSort(tuple*, int, int, int, int, bool);

class queryJob : public Job {
    
private:
    char** parts;
    const InputArray** allrelations;
    int queryIndex;

public:
    queryJob(char** parts,const InputArray** allrelations, int queryIndex) : Job() 
    { 
        this->parts = parts;
        this->allrelations = allrelations;
        this->queryIndex = queryIndex;
    }

    void run() override
    {
        // std::cout << "reorder added to queue\n";
        // std::cout<<"array:"<<array<<", offset: "<<offset<<std::endl;
        handlequery(parts, allrelations, queryIndex);
        return; 
    }
};

class trJob : public Job    //tuple reorder job
{
private:
    tuple* array;
    tuple* array2;
    int offset;
    int shift;
    bool isLastCall;
    int reorderIndex;
    int queryIndex;

public:
    trJob(tuple* array,tuple* array2, int offset,int shift, bool isLastCall, int reorderIndex, int queryIndex) : Job() 
    { 
        this->array = array;
        this->array2 = array2;
        this->offset = offset;
        this->shift = shift;
        this->isLastCall = isLastCall;
        this->reorderIndex = reorderIndex;
        this->queryIndex = queryIndex;
    }

    void run() override
    {
        // std::cout << "reorder added to queue\n";
        // std::cout<<"array:"<<array<<", offset: "<<offset<<std::endl;
        tuplereorder_parallel(array, array2, offset, shift, isLastCall, reorderIndex, queryIndex);

        pthread_mutex_lock(&jobsCounterMutexes[queryIndex]);
        jobsCounter[queryIndex]--;
        if (jobsCounter[queryIndex] == 0) {
            pthread_cond_signal(&jobsCounterConds[queryIndex]);
        }
        // std::cout<<"-- "<<jobsCounter[queryIndex]<<std::endl;
        // std::cout<<"reorder Job with id: "<<getJobId()<<" finished"<<" queryIndex: "<<queryIndex<<std::endl;

        pthread_mutex_unlock(&jobsCounterMutexes[queryIndex]);
        return; 
    }
};

class qJob : public Job    //quicksort job
{
private:
    tuple* tuples;
    int startIndex;
    int stopIndex;
    int queryIndex;
    int reorderIndex;
    bool isLastCall;

public:
    qJob(tuple* tuples, int startIndex, int stopIndex, int queryIndex, int reorderIndex, bool isLastCall) : Job()
    {
        this->tuples = tuples;
        this->startIndex = startIndex;
        this->stopIndex = stopIndex;
        this->queryIndex = queryIndex;
        this->reorderIndex = reorderIndex;
        this->isLastCall = isLastCall;
    }

    void run() override
    {
        // std::cout << "quicksort added to queue\n";
        quickSort(tuples, startIndex, stopIndex, queryIndex, reorderIndex, isLastCall);

        pthread_mutex_lock(&jobsCounterMutexes[queryIndex]);
        jobsCounter[queryIndex]--;
        if (jobsCounter[queryIndex] == 0) {
            pthread_cond_signal(&jobsCounterConds[queryIndex]);
        }
        // std::cout<<"qJob -- "<<jobsCounter[queryIndex]<<std::endl;
        // std::cout<<"quicksort Job with id: "<<getJobId()<<" finished"<<" queryIndex: "<<queryIndex<<std::endl;
        pthread_mutex_unlock(&jobsCounterMutexes[queryIndex]);
        return; 
    }
};

uint64_t hashFunction(uint64_t payload, int shift);
result* join(relation* R, relation* S,uint64_t**r,uint64_t**s,int rsz,int ssz,int joincol);
// uint64_t** create_hist(relation*, int);
// uint64_t** create_psum(uint64_t**, uint64_t);
// relation* re_ordered(relation*,relation*, int);
// relation* re_ordered_2(relation*,relation*, int); //temporary
uint64_t* psumcreate(uint64_t* hist);
uint64_t* histcreate(tuple* array,int offset,int shift);
void tuplereorder(tuple* array,tuple* array2, int offset,int shift, Type t, int reorderIndex, int queryIndex);
void tuplereorder_parallel(tuple* array,tuple* array2, int offset,int shift, bool isLastCall, int reorderIndex, int queryIndex);


// functions for bucket sort
void swap(tuple* tuple1, tuple* tuple2);
int randomIndex(int startIndex, int stopIndex);
int partition(tuple* tuples, int startIndex, int stopIndex);
void quickSort(tuple* tuples, int startIndex, int stopIndex, int queryIndex, int reorderIndex, bool isLastCall);
void sortBucket(relation* rel, int startIndex, int endIndex);
InputArray** readArrays();
char** readbatch(int& lns);
char** makeparts(char* query);
void handlequery(char** parts,const InputArray** allrelations, int queryIndex);
void loadrelationIds(int* relationIds, char* part, int& relationsnum);
bool shouldSort(uint64_t** predicates, int predicatesNum, int curPredicateIndex, int curPredicateArrayId, int curFieldId, bool prevPredicateWasFilterOrSelfJoin);
IntermediateArray* handlepredicates(const InputArray** relations,char* part,int relationsnum, int* relationIds, int queryIndex);
void handleprojection(IntermediateArray* rowarr,const InputArray** array,char* part, int* relationIds,int queryIndex);
uint64_t** splitpreds(char* ch,int& cn);
uint64_t** optimizepredicates(uint64_t** preds,int cntr,int relationsnum,int* relationIds);
void predsplittoterms(char* pred,uint64_t& rel1,uint64_t& col1,uint64_t& rel2,uint64_t& col2,uint64_t& flag);
uint64_t* histcreate(tuple* array,int offset,int shift);
uint64_t* psumcreate(uint64_t* hist);

relation* re_ordered_2(relation *rel, relation* new_rel, int no_used);
void mid_func(tuple *t1, tuple *t2, int num, int not_used);


#endif