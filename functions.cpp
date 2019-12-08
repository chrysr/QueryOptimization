#include "functions.h"

void relation::print()
{
    for(uint64_t i=0;i<this->num_tuples;i++)
    {
        std::cout<<this->tuples[i].key<<". "<<this->tuples[i].payload<<std::endl;
    }
}

relation::relation()
{
    num_tuples = 0;
    tuples = NULL;
}

relation::~relation()
{
    if (tuples != NULL)
        delete [] tuples;
}

InputArray::InputArray(uint64_t rowsNum, uint64_t columnsNum) {
    this->rowsNum = rowsNum;
    this->columnsNum = columnsNum;
    this->columns = new uint64_t*[columnsNum];
    for (uint64_t i = 0; i < columnsNum; i++) {
        this->columns[i] = new uint64_t[rowsNum];
    }
}

InputArray::InputArray(uint64_t rowsNum) : InputArray(rowsNum, 1) {
    for (uint64_t i = 0; i < rowsNum; i++) {
        columns[0][i] = i;
    }
}

InputArray::~InputArray() {
    for (uint64_t i = 0; i < columnsNum; i++) {
        delete[] this->columns[i];
    }
    delete[] this->columns;
}

InputArray* InputArray::filterRowIds(uint64_t fieldId, int operation, uint64_t numToCompare, InputArray* pureInputArray) {
    InputArray* newInputArrayRowIds = new InputArray(rowsNum);
    uint64_t newInputArrayRowIndex = 0;

    for (uint64_t i = 0; i < rowsNum; i++) {
        uint64_t inputArrayRowId = columns[0][i];
        uint64_t inputArrayFieldValue = pureInputArray->columns[fieldId][inputArrayRowId];
        bool filterApplies = false;
        switch (operation)
        {
        case 0: // '>'
            filterApplies = inputArrayFieldValue > numToCompare;
            break;
        case 1: // '<'
            filterApplies = inputArrayFieldValue < numToCompare;
            break;
        case 2: // '='
            filterApplies = inputArrayFieldValue == numToCompare;
            break;
        default:
            break;
        }

        if (!filterApplies)
            continue;

        newInputArrayRowIds->columns[0][newInputArrayRowIndex++] = inputArrayRowId;
    }

    newInputArrayRowIds->rowsNum = newInputArrayRowIndex; // update rowsNum because the other rows are useless

    return newInputArrayRowIds;
}

InputArray* InputArray::filterRowIds(uint64_t field1Id, uint64_t field2Id, InputArray* pureInputArray) {
    InputArray* newInputArrayRowIds = new InputArray(rowsNum);
    uint64_t newInputArrayRowIndex = 0;

    for (uint64_t i = 0; i < rowsNum; i++) {
        uint64_t inputArrayRowId = columns[0][i];
        uint64_t inputArrayField1Value = pureInputArray->columns[field1Id][inputArrayRowId];
        uint64_t inputArrayField2Value = pureInputArray->columns[field2Id][inputArrayRowId];
        bool filterApplies = inputArrayField1Value == inputArrayField2Value;

        if (!filterApplies)
            continue;

        newInputArrayRowIds->columns[0][newInputArrayRowIndex++] = inputArrayRowId;
    }

    newInputArrayRowIds->rowsNum = newInputArrayRowIndex; // update rowsNum because the other rows are useless

    return newInputArrayRowIds;
}

void InputArray::extractColumnFromRowIds(relation& rel, uint64_t fieldId, InputArray* pureInputArray) {
    // printf("gg\n");
    rel.num_tuples = rowsNum;
    rel.tuples=new tuple[rel.num_tuples];
    for(uint64_t i = 0; i < rel.num_tuples; i++)
    {
        uint64_t inputArrayRowId = columns[0][i];
        // printf("i: %lu\n", i);
        rel.tuples[i].key = inputArrayRowId;
        rel.tuples[i].payload = pureInputArray->columns[fieldId][inputArrayRowId];
        // printf("aaaaaa\n", i);
    }
}

IntermediateArray::IntermediateArray(uint64_t columnsNum, uint64_t sortedByInputArrayId, uint64_t sortedByFieldId) {
    this->rowsNum = 0;
    this->sortedByFieldId = sortedByFieldId;
    this->sortedByInputArrayId = sortedByInputArrayId;
    this->columnsNum = columnsNum;
    this->results = new uint64_t*[columnsNum];
    for (uint64_t j = 0; j < columnsNum; j++) {
        this->results[j] = NULL;
    }
    this->inputArrayIds = new int[columnsNum];
}

IntermediateArray::~IntermediateArray() {
    for (uint64_t i = 0; i < columnsNum; i++) {
        if (this->results[i] != NULL) {
            delete[] this->results[i];
        }
    }
    delete[] this->results;
    delete[] this->inputArrayIds;
}

void IntermediateArray::extractFieldToRelation(relation* resultRelation, InputArray* inputArray, int inputArrayId, uint64_t fieldId) {
    resultRelation->num_tuples = rowsNum;
    resultRelation->tuples = new tuple[resultRelation->num_tuples];

    uint64_t columnIndex = 0;
    for (uint64_t j = 0; j < columnsNum; j++) {
        if (inputArrayIds[j] == inputArrayId) {
            columnIndex = j;
            break;
        }
    }

    for (uint64_t i = 0; i < rowsNum; i++) {
        uint64_t inputArrayRowId = this->results[columnIndex][i];
        
        resultRelation->tuples[i].key = i; // row id of this intermediate array
        resultRelation->tuples[i].payload = inputArray->columns[fieldId][inputArrayRowId];
    }
}

// intermediateResult has always 2 columns and by convention: 
// - if prevIntermediateArray != NULL, then the 1st column of intermediateResult contains row ids of this IntermediateArray and the 2nd column contains row ids of the first-time-used input array
// - if prevIntermediateArray == NULL, then both the 1st and 2nd column of intermediateResult contains row ids of 2 first-time-used input arrays
void IntermediateArray::populate(uint64_t** intermediateResult, uint64_t resultRowsNum, IntermediateArray* prevIntermediateArray, int inputArray1Id, int inputArray2Id) {
    this->rowsNum = resultRowsNum;

    for (uint64_t j = 0; j < columnsNum; j++) {
        results[j] = new uint64_t[resultRowsNum];
    }

    if (prevIntermediateArray == NULL) {
        // first time creating an IntermediateArray
        inputArrayIds[0] = inputArray1Id;
        inputArrayIds[1] = inputArray2Id;
        for (uint64_t i = 0; i < resultRowsNum; i++) {
            results[0][i] = intermediateResult[0][i];
            results[1][i] = intermediateResult[1][i];
        }
        return;
    }

    for (uint64_t j = 0; j < columnsNum; j++) {
        for (uint64_t i = 0; i < resultRowsNum; i++) {
            if (j == columnsNum - 1) {
                uint64_t inputArrayRowId = intermediateResult[1][i];
                results[j][i] = inputArrayRowId;
            } else {
                uint64_t prevIntermediateArrayRowId = intermediateResult[0][i];
                results[j][i] = prevIntermediateArray->results[j][prevIntermediateArrayRowId];
            }
        }
        if (j == columnsNum - 1) {
            inputArrayIds[j] = inputArray2Id;
        } else {
            inputArrayIds[j] = prevIntermediateArray->inputArrayIds[j];
        }
    }
}

bool IntermediateArray::hasInputArrayId(int inputArrayId) {
    for (uint64_t j = 0; j < columnsNum; j++) {
        if (inputArrayIds[j] == inputArrayId) {
            return true;
        }
    }
    return false;
}

bool IntermediateArray::shouldSort(int nextQueryInputArrayId, uint64_t nextQueryFieldId) {
    return ! (this->sortedByInputArrayId == nextQueryInputArrayId && this->sortedByFieldId == nextQueryFieldId);
}

void IntermediateArray::print() {
    printf("input array ids: ");
    for (uint64_t j = 0; j < columnsNum; j++) {
        std::cout << inputArrayIds[j] << " ";
    }
    std::cout << std::endl;

    for (uint64_t i = 0; i < rowsNum; i++) {
        for (uint64_t j = 0; j < columnsNum; j++) {
            std::cout << results[j][i] << " ";
        }
        std::cout << std::endl;
    }
}

uint64_t IntermediateArray::findColumnIndexByInputArrayId(int inputArrayId) {
    for (uint64_t j = 0; j < columnsNum; j++) {
        if (inputArrayIds[j] == inputArrayId)
            return j;
    }
    return -1;
}

IntermediateArray* IntermediateArray::selfJoin(int inputArray1Id, int inputArray2Id, uint64_t field1Id, uint64_t field2Id, InputArray* inputArray1, InputArray* inputArray2) {
    IntermediateArray* newIntermediateArray = new IntermediateArray(columnsNum, 0, 0);

    newIntermediateArray->rowsNum = rowsNum;
    for (uint64_t j = 0; j < newIntermediateArray->columnsNum; j++) {
        newIntermediateArray->results[j] = new uint64_t[newIntermediateArray->rowsNum];
    }

    for (uint64_t j = 0; j < newIntermediateArray->columnsNum; j++) {
        newIntermediateArray->inputArrayIds[j] = inputArrayIds[j];
    }

    uint64_t columnIndexArray1 = findColumnIndexByInputArrayId(inputArray1Id);
    uint64_t columnIndexArray2 = findColumnIndexByInputArrayId(inputArray2Id);

    uint64_t newIntermediateArrayRowIndex = 0;
    for (uint64_t i = 0; i < rowsNum; i++) {
        uint64_t inputArray1RowId = results[columnIndexArray1][i];
        uint64_t inputArray2RowId = results[columnIndexArray2][i];
        uint64_t inputArray1FieldValue = inputArray1->columns[field1Id][inputArray1RowId];
        uint64_t inputArray2FieldValue = inputArray2->columns[field2Id][inputArray2RowId];

        bool filterApplies = inputArray1FieldValue == inputArray2FieldValue;

        if (!filterApplies)
            continue;

        for (uint64_t j = 0; j < columnsNum; j++) {
            newIntermediateArray->results[j][newIntermediateArrayRowIndex] = results[j][i];
        }
        newIntermediateArrayRowIndex++;
    }

    newIntermediateArray->rowsNum = newIntermediateArrayRowIndex; // update rowsNum because the other rows are useless
    //newIntermediateArray->print();
    return newIntermediateArray;
}

uint64_t hashFunction(uint64_t payload, int shift) {
    return (uint64_t)((payload >> (8 * shift)) & 0xFF);
}

result* join(relation* R, relation* S,uint64_t**rr,uint64_t**ss,int rsz,int ssz,int joincol)
{
    int64_t samestart=-1;
    int lstsize=1024*1024;
    //list* lst=new list(lstsize,rsz+ssz-1);
    list*lst=new list(lstsize,2);
    for(uint64_t r=0,s=0,i=0;r<R->num_tuples&&s<S->num_tuples;)
    {
        //std::cout<<"checking: R:"<<R->tuples[r].payload<<" S:"<<S->tuples[s].payload<<std::endl;
        int64_t dec=R->tuples[r].payload-S->tuples[s].payload;
        if(dec==0)
        {
            //std::cout<<R->tuples[r].payload<<" same"<<std::endl; 
            //char x[lstsize+2];
            //x[0]='\0';
            lst->insert(R->tuples[r].key);
            lst->insert(S->tuples[s].key);
            /*for(int i=0;i<rsz;i++)
            {
                //sprintf(x,"%s%ld ",x,rr[i][R->tuples[r].key]);
                lst->insert(rr[i][R->tuples[r].key]);
            }
            for(int i=0;i<ssz;i++)
            {
                if(joincol==i)
                    continue;
                //sprintf(x,"%s%ld ",x,ss[i][S->tuples[s].key]);
                lst->insert(ss[i][S->tuples[s].key]);
            }*/
            //sprintf(x,"%s\n",x);
            //printf("%s",x);
            //sprintf(x,"%ld %ld\n",R->tuples[r].key,S->tuples[s].key);
        //lst->insert(x);
            //std::cout<<i<<". "<<R->tuples[r].key<<" "<<S->tuples[s].key<<std::endl;
            //std::cout<<x;
            i++;
            //std::cout<<"Matching: R:"<<R->tuples[r].key<<" S:"<<S->tuples[s].key<<std::endl;
            /*array[i][0]=R->tuples[r].key;
            array[i][1]=R->tuples[s].key;
            i++;*/
            switch(samestart)
            {
                case -1:
                    if(s+1<S->num_tuples&&S->tuples[s].payload==S->tuples[s+1].payload)
                        samestart=s;
                default:
                    if(S->tuples[samestart].payload!=S->tuples[s].payload)
                        samestart=-1;
            }
            if(s+1<S->num_tuples&&S->tuples[s].payload==S->tuples[s+1].payload)
            {
                s++;
            }
            else
            {
                r++;
                if(samestart>-1)
                {
                    s=samestart;
                    samestart=-1;
                }
            }
            continue;
        }
        else if(dec<0)
        {
            //std::cout<<R->tuples[r].payload<<" <(r)"<<std::endl;
            r++;
            /*if(samestart!=-1)
            {
                s=samestart;
                samestart=-1;
            }*/
            continue;
        }
        else
        {
            //std::cout<<R->tuples[r].payload<<" >(s)"<<std::endl;
            s++;
            continue;
        }
    }
    /*std::cout<<"got out"<<std::endl;
    for(int i=0;i<50000;i++)
    {
        if(array[i][0]==-1&&array[i][1]==-1)
            break;
        std::cout<<i<<". "<<array[i][0]<<" "<<array[i][1]<<std::endl;
    }*/
    // std::cout<<std::endl;
    result* rslt=new result;
    rslt->lst=lst;
    return rslt;
    //lst->print();
    //lst->print();
    //evala Return gia na mhn vgazei Warning
    return NULL;
}

uint64_t** create_hist(relation *rel, int shift)
{
    int x = pow(2,8);
    uint64_t **hist = new uint64_t*[3];
    for(int i = 0; i < 3; i++)
        hist[i] = new uint64_t[x];
    uint64_t payload, mid;
    for(int i = 0; i < x; i++)
    {
        hist[0][i]= i;
        hist[1][i]= 0;
        hist[2][i]= shift;
    }

    for (uint64_t i = 0; i < rel->num_tuples; i++)
    {
        //mid = (0xFFFFFFFF & rel->tuples[i].payload) >> (8*shift);
        payload = hashFunction(rel->tuples[i].payload, 7-shift);

        if (payload > 0xFF)
        {
            std::cout << "ERROR " << payload << std::endl;
            return NULL;
        }
        hist[1][payload]++;
    }
    return hist;
}

uint64_t** create_psum(uint64_t** hist, uint64_t size)
{
    uint64_t count = 0;
    uint64_t x = size;
    uint64_t **psum = new uint64_t*[3];
    for(int i = 0; i < 3; i++)
        psum[i] = new uint64_t[x];

    for (uint64_t i = 0; i < x; i++)
    {
        psum[0][i] = hist[0][i];
        psum[1][i] = (uint64_t)count;
        psum[2][i] = hist[2][i];
        count+=hist[1][i];
    }
    return psum;
}

void pr(uint64_t** a, uint64_t array_size)
{
    uint64_t i;
    for (i = 0; i < array_size; i++)
    {
        if (a[1][i] != 0 || (i < array_size -1 && a[2][i] < a[2][i+1]))
        {
            for (int l = 0; l < a[2][i]; l++)
                std::cout << "  ";
            std::cout << a[0][i] << ". " << a[1][i] << " - " << a[2][i] << std::endl;
        }
    }
}

uint64_t** combine_hist(uint64_t** big, uint64_t** small, uint64_t position, uint64_t big_size)   //big_size == size of row in big
{
    uint64_t x = pow(2,8), i, j;    //size of small == pow(2,8)

    uint64_t **hist = new uint64_t*[3];
    for(i = 0; i < 3; i++)
        hist[i] = new uint64_t[x + big_size];

    /*for (i = 0; i < position; i++) { hist[0][i] = big[0][i]; hist[1][i] = big[1][i]; hist[2][i] = big[2][i]; }*/
    memcpy(hist[0], big[0], sizeof(big[0][0]) * position);
    memcpy(hist[1], big[1], sizeof(big[1][0]) * position);
    memcpy(hist[2], big[2], sizeof(big[2][0]) * position);
    i = position;
    hist[0][i] = big[0][i];
    hist[1][i] = 0;//big[1][i];
    hist[2][i] = big[2][i];
    i++;
    memcpy(&hist[0][i], small[0], sizeof(small[0][0]) * x);
    memcpy(&hist[1][i], small[1], sizeof(small[1][0]) * x);
    memcpy(&hist[2][i], small[2], sizeof(small[2][0]) * x);
    /*for (j = 0; j < x; j++) { hist[0][i] = small[0][j]; hist[1][i] = small[1][j]; hist[2][i] = small[2][j]; i++; }*/
    memcpy(&hist[0][position + 1 + x], &big[0][position + 1], sizeof(big[0][0]) * (big_size - position));
    memcpy(&hist[1][position + 1 + x], &big[1][position + 1], sizeof(big[1][0]) * (big_size - position));
    memcpy(&hist[2][position + 1 + x], &big[2][position + 1], sizeof(big[2][0]) * (big_size - position));
    /*for (i = position + 1; i < big_size; i++) { hist[0][i + x] = big[0][i]; hist[1][i + x] = big[1][i]; hist[2][i + x] = big[2][i]; }*/

    delete [] big[0];
    delete [] big[1];
    delete [] big[2];
    delete [] big;
    delete [] small[0];
    delete [] small[1];
    delete [] small[2];
    delete [] small;
    return hist;
}

uint64_t find_shift(uint64_t **hist, uint64_t hist_size, uint64_t payload)
{
    uint64_t i, shift, j, flag;
    uint64_t hash;
    uint64_t **last = new uint64_t*[3];
    for(i = 0; i < 3; i++)
        last[i] = new uint64_t[8];
    shift = 0;
    for (i = 0; i < hist_size; i++)
    {
        //std::cout << payload << ": " << hashFunction(payload, 7 - hist[2][i]) << " : " << hist[0][i] << std::endl;
        if (i < hist_size - 1 && hist[2][i] < hist[2][i+1])
        {
            last[0][shift] = hist[0][i];
            last[1][shift] = hist[1][i];
            last[2][shift] = hist[2][i];
            shift++;
        }
        if (i < hist_size - 1 && hist[2][i] > hist[2][i+1])
            shift--;

        if (hist[1][i] != 0 && hashFunction(payload, 7 - hist[2][i]) == hist[0][i])
        {
            flag = 1;
            for(j = 0; j < hist[2][i]; j++)
                if (hashFunction(payload, 7 - last[2][j]) != last[0][j])
                    flag = 0;
            if (flag)
            {
                delete [] last[0];
                delete [] last[1];
                delete [] last[2];
                delete [] last;
                return i;
                //return hist[2][i];
            }
        }
    }
    delete [] last[0];
    delete [] last[1];
    delete [] last[2];
    delete [] last;
    pr(hist, hist_size);
    std::cout << "NOT FOUND: " << payload << " HASH: " << hashFunction(payload, 7 - 7) << std::endl;
    return 0;
}
/*
uint64_t find_shift(uint64_t **hist, uint64_t hist_size, uint64_t payload)
{
    uint64_t i, shift, j, flag;
    uint64_t hash;
    for (i = 0; i < hist_size; i++)
    {
        //std::cout << payload << ": " << hashFunction(payload, 7 - hist[2][i]) << " : " << hist[0][i] << std::endl;
        if (hist[1][i] != 0 && hashFunction(payload, 7 - hist[2][i]) == hist[0][i])
        {
            j = i;
            shift = hist[2][i];
            flag = 1;
            while (j >= 0 && shift != 0)
            {
                if (hist[2][j] == shift-1)
                {
                    if (hashFunction(payload, 7 - hist[2][j]) != hist[0][j])
                    {
                        flag = 0;
                    }
                    shift--;
                }   
                j--;
            }
            if (flag)
                return i;
                //return hist[2][i];
        }
    }

    pr(hist, hist_size);
    std::cout << "NOT FOUND: " << payload << " HASH: " << hashFunction(payload, 7 - 7) << std::endl;
    return 0;
}
*/

void print_psum_hist(uint64_t** psum, uint64_t** hist, int array_size)
{
    int i;
    std::cout << "<<<<<<<" << std::endl;
    for (int i = 0; i < array_size; i++)
        if (i == array_size-1 || psum[1][i] != psum[1][i+1])
            std::cout << psum[0][i] << " " << psum[1][i] << " " << psum[2][i] << std::endl;
    std::cout << "<<<<<<<" << std::endl;
    pr(hist, array_size);
    std::cout << "<<<<<<<" << std::endl;
}

relation* re_ordered(relation *rel, relation* new_rel, int no_used)
{
    int shift = 0;
    uint64_t x = pow(2, 8), array_size = x;
    relation *temp = NULL;
    relation *rtn  = NULL;
    //create histogram
    uint64_t** hist = create_hist(rel, shift), **temp_hist = NULL;
    //create psum
    uint64_t** psum = create_psum(hist, x), **temp_psum = NULL;
    uint64_t payload;
    uint64_t i, j, y;
    bool clear;

    /*for (i = 0; i < rel->num_tuples; i++)
        flag[i] = false;
    */
    uint64_t** tempPsum = new uint64_t*[3];
    for (uint64_t i = 0; i < 3; i++) {
        tempPsum[i] = new uint64_t[x];
        memcpy(tempPsum[i], psum[i], x);
    }
    
    i = 0;
    while(i < rel->num_tuples)
    {
        // if (i%10000==0) {
        //     std::cout<<"loop1 i: "<<i<<std::endl;
        // }
        //hash
        //payload = (0xFFFFFFFF & rel->tuples[i].payload) >> (8*shift) & 0xFF;
        payload = hashFunction(rel->tuples[i].payload, 7 - shift);
        //find hash in psum = pos in new relation
        
        // uint64_t next_i = psum[1][payload];

        //key++ until their is an empty place
        // while ((next_i < rel->num_tuples) && flag[next_i])
        //     next_i++;

        // if (next_i < rel->num_tuples)
        // {
            new_rel->tuples[tempPsum[1][payload]].payload = rel->tuples[i].payload;
            new_rel->tuples[tempPsum[1][payload]++].key = rel->tuples[i].key;
            // flag[next_i] = true;
        // }
        i++;
    }

    for (uint64_t i = 0; i < 3; i++) {
        delete[] tempPsum[i];
    }
    delete[] tempPsum;

    clear = false; //make a full loop with clear == false to end
    i = 0;
    while (i < array_size)
    {
        if ((hist[1][i] > TUPLES_PER_BUCKET) && (hist[2][i] < 7))
        {
            clear = true;
            //new relation from psum[1][i] to psum[1][i+1]
            if (rel == NULL)
                rel = new relation();
            uint64_t first = psum[1][i];
            uint64_t last = new_rel->num_tuples;
            if (i != array_size - 1)
                last = psum[1][i+1];
            rel->num_tuples = last - first;
            if(rel->tuples == NULL)
                rel->tuples = new tuple[new_rel->num_tuples];
            y = 0;
            for (j = first; j < last; j++)
            {
                rel->tuples[y] = new_rel->tuples[j];
                y++;
            }
            temp_hist = create_hist(rel, hist[2][i] + 1);
            temp_psum = create_psum(temp_hist, x);
            
            /*for (j = 0; j < rel->num_tuples; j++)
                flag[j] = false;
            */

            hist = combine_hist(hist, temp_hist, i, array_size);
            array_size+=x;
            //array_size-=1; //??????????????????????
            delete [] psum[0];
            delete [] psum[1];
            delete [] psum[2];
            delete [] psum;
            psum = create_psum(hist, array_size);
            delete [] temp_psum[0];
            delete [] temp_psum[1];
            delete [] temp_psum[2];
            delete [] temp_psum;
            /*for (j = 0; j < new_rel->num_tuples; j++)
            {
                flag[j] = false;
            }*/
            j = 0;
            if (rel == NULL)
                rel = new relation();
            if (sizeof(*rel->tuples) != sizeof(*new_rel->tuples))
            {
                delete [] rel->tuples;
                rel->tuples = new tuple[new_rel->num_tuples];
            }
            rel->num_tuples = new_rel->num_tuples;

            uint64_t** tempPsum = new uint64_t*[3];
            for (uint64_t i = 0; i < 3; i++) {
                tempPsum[i] = new uint64_t[array_size];
                memcpy(tempPsum[i], psum[i], array_size);
            }
            
            while(j < new_rel->num_tuples)
            {
                // if (j%10000==0) {
                //     std::cout<<"loop2 i: "<<j<<std::endl;
                // }
                //hash
                payload = find_shift(hist, array_size, new_rel->tuples[j].payload);//hashFunction(new_rel->tuples[j].payload, 7 - find_shift(hist, array_size, new_rel->tuples[j].payload));
                //find hash in psum = pos in new relation
                
                // uint64_t next_i = psum[1][payload];

                //key++ until their is an empty place
                // while ((next_i < new_rel->num_tuples) && flag[next_i])
                //     next_i++;

                // if (next_i < new_rel->num_tuples)
                // {
                rel->tuples[tempPsum[1][payload]].payload = new_rel->tuples[j].payload;
                rel->tuples[tempPsum[1][payload]++].key = new_rel->tuples[j].key;
                //     flag[next_i] = true;
                // }
                j++;
            }
            for (uint64_t i = 0; i < 3; i++) {
                delete[] tempPsum[i];
            }
            delete[] tempPsum;

            tuple *temp_tuple = rel->tuples;
            rel->tuples = new_rel->tuples;
            new_rel->tuples = temp_tuple;
            j = rel->num_tuples;
            rel->num_tuples = new_rel->num_tuples;
            new_rel->num_tuples = j;
        }
        
        if (hist[1][i] <= TUPLES_PER_BUCKET || hist[2][i] > 7)
        {
            if (hist[1][i] > 0)
            {
                if (i + 1 < array_size)
                    sortBucket(new_rel, psum[1][i], psum[1][i+1] - 1);
                else
                    sortBucket(new_rel, psum[1][i], rel->num_tuples - 1);
            }
        }
        i++;
        if (i == array_size && clear)
        {
            i = 0;
            clear = false;
        }
    }
    //testing
    //print_psum_hist(psum, hist, array_size);
    
    delete [] hist[0];
    delete [] hist[1];
    delete [] hist[2];

    delete [] hist;
    
    delete [] psum[0];
    delete [] psum[1];
    delete [] psum[2];

    delete [] psum;

    return new_rel;
}

relation* re_ordered_2(relation *rel, relation* new_rel, int shift)
{
    int x = pow(2, 8);
    // relation *new_rel = new relation();
    relation *temp;
    relation *rtn;
    //create histogram
    uint64_t** hist = create_hist(rel, shift);
    //create psum
    uint64_t** psum = create_psum(hist, x);
    uint64_t payload;
    uint64_t i, j, y;

    bool *flag = new bool[rel->num_tuples];
    for (i = 0; i < rel->num_tuples; i++)
        flag[i] = false;

    i = 0;
    while(i < rel->num_tuples)
    {
        //hash
        //payload = (0xFFFFFFFF & rel->tuples[i].payload) >> (8*shift) & 0xFF;
        payload = hashFunction(rel->tuples[i].payload, 7 - shift);
        //find hash in psum = pos in new relation
        
        uint64_t next_i = psum[1][payload];

        //key++ until their is an empty place
        while ((next_i < rel->num_tuples) && flag[next_i])
            next_i++;

        if (next_i < rel->num_tuples)
        {
            new_rel->tuples[next_i].payload = rel->tuples[i].payload;
            new_rel->tuples[next_i].key = rel->tuples[i].key;
            flag[next_i] = true;
        }
        i++;
    }

    //testing
    /*for (int i = 0; i < x; i++)
        if (i == x-1 || psum[1][i] != psum[1][i+1])
            std::cout << psum[0][i] << " " << psum[1][i] << std::endl;
    std::cout << "<<<<<<<" << std::endl;*/

    i = 0;
    while (i < x)
    {
        if ((hist[1][i] > TUPLES_PER_BUCKET) && (shift  < 7))
        {
            // new rel to re_order
            temp = new relation();
            temp->num_tuples = hist[1][i];
            temp->tuples = (new_rel->tuples + psum[1][i]);
            rtn = re_ordered_2(temp, rel, shift + 1);
            j = psum[1][i];
            y = 0;
            while (j < x)
            {
                if (j >= psum[1][i+1])
                    break;
                new_rel->tuples[j].payload = rtn->tuples[y].payload;
                new_rel->tuples[j].key = rtn->tuples[y].key;
                j++;
                y++;
            }
            std::free(temp); // free only relation's pointer because the tuples are not taking additional space
        }
        else if (hist[1][i] > 0)
        {
                //print bucket before sort
                /*std::cout << "{" << std::endl;
                if (i + 1 < x)
                {
                    for (int l = psum[1][i]; l < psum[1][i+1]; l++)
                        std::cout << new_rel->tuples[l].payload << ". " << new_rel->tuples[l].key << std::endl;
                }
                else
                {
                    for (int l = psum[1][i]; l < new_rel->num_tuples; l++)
                        std::cout << new_rel->tuples[l].payload << ". " << new_rel->tuples[l].key << std::endl;
                }
                std::cout << "}" << std::endl;
            std::cout << std::endl;*/
            if (i + 1 < x)
            {
                //std::cout << "-sort- " << psum[1][i] << " " << psum[1][i+1] << std::endl;
                sortBucket(new_rel, psum[1][i], psum[1][i+1] - 1);
            }
            else
            {
                //std::cout << "-sort- " << psum[1][i] << " " << new_rel->num_tuples << std::endl;
                sortBucket(new_rel, psum[1][i], rel->num_tuples - 1);
            }
        }
        
        //print buckets
        /*if (hist[1][i] > 0)
        {
            if (i + 1 < x)
            {
                for (int l = psum[1][i]; l < psum[1][i+1]; l++)
                    std::cout << new_rel->tuples[l].payload << ". " << new_rel->tuples[l].key << std::endl;
            }
            else
            {
                for (int l = psum[1][i]; l < new_rel->num_tuples; l++)
                    std::cout << new_rel->tuples[l].payload << ". " << new_rel->tuples[l].key << std::endl;
            }
            std::cout << std::endl;
        }*/
        
        i++;
    }

    // std::cout << "before sort" << std::endl;
    // new_rel->print();

    // sortBucket(new_rel, 0, 4);

    // std::cout << "after sort" << std::endl;
    // new_rel->print();

    delete [] hist[0];
    delete [] hist[1];

    delete [] hist;
    
    delete [] psum[0];
    delete [] psum[1];

    delete [] psum;
    delete [] flag;

    return new_rel;
}

void swap(tuple* tuple1, tuple* tuple2)
{
    uint64_t tempKey = tuple1->key;
    uint64_t tempPayload = tuple1->payload;

    tuple1->key = tuple2->key;
    tuple1->payload = tuple2->payload;

    tuple2->key = tempKey;
    tuple2->payload = tempPayload;
}

int randomIndex(int startIndex, int stopIndex) {
    srand(time(NULL));

    return rand()%(stopIndex - startIndex + 1) + startIndex;
}

int partition(tuple* tuples, int startIndex, int stopIndex)
{ 
    int pivotIndex = randomIndex(startIndex, stopIndex);

    uint64_t pivot = tuples[pivotIndex].payload;

    swap(&tuples[pivotIndex], &tuples[stopIndex]);

    int i = startIndex - 1;  // index of smaller element 
  
    for (int j = startIndex; j < stopIndex; j++) 
    {
        if (tuples[j].payload < pivot) 
        { 
            // if current element is smaller than the pivot 
            i++;    // increment index of smaller element 
            
            swap(&tuples[i], &tuples[j]);
        }
    }
    swap(&tuples[i + 1], &tuples[stopIndex]);
    return (i + 1);
}

// (startIndex, stopIndex) -> inclusive
void quickSort(tuple* tuples, int startIndex, int stopIndex)
{
    if (startIndex < stopIndex) 
    { 
        int partitionIndex = partition(tuples, startIndex, stopIndex); 
  
        quickSort(tuples, startIndex, partitionIndex - 1); 
        quickSort(tuples, partitionIndex + 1, stopIndex); 
    } 
}

// (startIndex, stopIndex) -> inclusive
void sortBucket(relation* rel, int startIndex, int stopIndex) {
    // stopIndex--;
    quickSort(rel->tuples, startIndex, stopIndex);
}

void extractcolumn(relation& rel,uint64_t **array, uint64_t column)
{
    // printf("gg\n");
    rel.tuples=new tuple[rel.num_tuples];
    for(uint64_t i=0;i<rel.num_tuples;i++)
    {
        // printf("i: %lu\n", i);
        rel.tuples[i].key=i;
        rel.tuples[i].payload=array[column][i];
        // printf("aaaaaa\n", i);
    }
}

InputArray** readArrays() {
    InputArray** inputArrays = new InputArray*[MAX_INPUT_ARRAYS_NUM]; // size is fixed
    // printf("1\n");
    size_t fileNameSize = MAX_INPUT_FILE_NAME_SIZE;
    char fileName[fileNameSize];

    unsigned int inputArraysIndex = 0;
    while (fgets(fileName, fileNameSize, stdin) != NULL) {
        fileName[strlen(fileName) - 1] = '\0'; // remove newline character
        
        if (strcmp(fileName, "Done") == 0)
            break;

        // printf("loop 1\n");
        uint64_t rowsNum, columnsNum, cellValue;
        FILE *fileP;
        // printf("loop 2\n");

        fileP = fopen(fileName, "rb");
        // printf("loop 3\n");

        fread(&rowsNum, sizeof(uint64_t), 1, fileP);
        fread(&columnsNum, sizeof(uint64_t), 1, fileP);
        // printf("rows num: %lu, columns num: %lu\n", rowsNum, columnsNum);

        inputArrays[inputArraysIndex] = new InputArray(rowsNum, columnsNum);

        for (uint64_t i = 0; i < columnsNum; i++) {
            for (uint16_t j = 0; j < rowsNum; j++) {
                fread(&inputArrays[inputArraysIndex]->columns[i][j], sizeof(uint64_t), 1, fileP);
            }
        }

        fclose(fileP);

        inputArraysIndex++;
    }

    return inputArrays;
}

void InputArray::print() {
    for (uint64_t i = 0; i < rowsNum; i++) {
        for (uint64_t j = 0; j < columnsNum; j++) {
            std::cout << columns[j][i] << " ";
        }
        std:: cout << std::endl;
    }
    std:: cout << std::endl;
}

char** readbatch(int& lns)
{
    lns=0;
    char ch;
    list* l=new list(1024,0);
    int flag=0;
    int lines=0;
    while(1)
    {
        ch=getchar();
        if(ch==EOF)
            return NULL;
        if(ch=='\n'&&flag)
            continue;
        l->insert(ch);
        if(ch=='F'&&flag)
        {
            while(1)
            {
                ch=getchar();
                if(ch=='\n')
                    break;
                if(ch==EOF)
                    break;
            }
            break;
        }
        if(ch=='\n')
        {
            flag=1;
            lines++;
        }
        else flag=0;
    }
    char* arr=l->lsttocharr();
    char** fnl=new char*[lines];
    int start=0;
    int ln=0;
    for(int i=0;arr[i]!='\0';i++)
    {
        if(arr[i]=='\n')
        {
            fnl[ln]=new char[i-start+1];
            memcpy(fnl[ln],arr+start,i-start);
            fnl[ln][i-start]='\0';
            ln++;
            start=i+1;
        }
    }
    /*std::cout<<"\n";
    for(int i=0;i<ln;i++)
    {
        std::cout<<fnl[i]<<std::endl;
    }*/
    delete[] arr;
    delete l;
    lns=ln;    
    return fnl;
}
char** makeparts(char* query)
{
    //std::cout<<query<<std::endl;
    int start=0;
    char** parts;
    parts=new char*[3];
    for(int part=0,i=0;query[i]!='\0';i++)
    {
        if(query[i]=='|')
        {
            query[i]='\0';
            parts[part]=query+start;
            start=i+1;
            part++;
        }
    }
    parts[2]=query+start;
    return parts;
}
void handlequery(char** parts,InputArray** allrelations)
{
    /*for(int i=0;i<3;i++)
    {
        std::cout<<parts[i]<<std::endl;
    }*/
    // std::cout<<std::endl;
    int relationIds[MAX_INPUT_ARRAYS_NUM];
    int relationsnum;
    loadrelationIds(relationIds, parts[0], relationsnum);
    IntermediateArray* result=handlepredicates(allrelations,parts[1],relationsnum, relationIds);
    handleprojection(result,allrelations,parts[2], relationIds);
    // std::cout<<std::endl;
    

}
void loadrelationIds(int* relationIds, char* part, int& relationsnum)
{
    // std::cout<<"LOADRELATIONS: "<<part<<std::endl;
    int cntr=1;
    uint64_t*** relations;
    for(int i=0;part[i]!='\0';i++)
    {
        if(part[i]==' ')
            cntr++;
    }

    char tempPart[strlen(part) + 1];
    strcpy(tempPart, part);
    int i = 0;
    char* token = strtok(tempPart, " ");
    while (token) {
        relationIds[i++] = atoi(token);
        token = strtok(NULL, " ");
    }
    // for (int i = 0; i < MAX_INPUT_ARRAYS_NUM; i++) {
    //     printf("%d ", relationIds[i]);
    // }
    // relations=new uint64_t**[cntr];
    relationsnum=cntr;
    //std::cout<<cntr<<" relations"<<std::endl;
}

IntermediateArray* handlepredicates(InputArray** inputArrays,char* part,int relationsnum, int* relationIds)
{
    // std::cout<<"HANDLEPREDICATES: "<<part<<std::endl;
    int cntr;
    uint64_t** preds=splitpreds(part,cntr);
    // for(int i=0;i<cntr;i++)
    // {
    //     for(int j=0;j<5;j++)
    //     {
    //         std::cout<<"  "<<preds[i][j];
    //     }
    //     std::cout<<std::endl;
    // }
    preds=optimizepredicates(preds,cntr,relationsnum,relationIds);
    // std::cout<<std::endl;
    // for(int i=0;i<cntr;i++)
    // {
    //     for(int j=0;j<5;j++)
    //     {
    //         std::cout<<"  "<<preds[i][j];
    //     }
    //     std::cout<<std::endl;
    // }

    // InputArray* inputArray1RowIds = new InputArray(inputArray1->rowsNum, 1);
    // InputArray* inputArray2RowIds = new InputArray(inputArray2->rowsNum, 1);
    InputArray** inputArraysRowIds = new InputArray*[relationsnum];
    for (int i = 0; i < relationsnum; i++) {
        // printf("................ %d\n", inputArrays[i]->rowsNum);
        inputArraysRowIds[i] = new InputArray(inputArrays[relationIds[i]]->rowsNum);
    }

    IntermediateArray* curIntermediateArray = NULL;

    // filters and inner-joins are first and regular joins follow
    for(int i=0;i<cntr;i++)
    {
        bool isFilter = preds[i][3] == (uint64_t) - 1;
        int predicateArray1Id = preds[i][0];
        int predicateArray2Id = preds[i][3];
        int inputArray1Id = relationIds[predicateArray1Id];
        int inputArray2Id = isFilter ? -1 : relationIds[predicateArray2Id];
        InputArray* inputArray1 = inputArrays[inputArray1Id];
        InputArray* inputArray2 = isFilter ? NULL : inputArrays[inputArray2Id];
        // InputArray* inputArray1RowIds = new InputArray(inputArray1->rowsNum, 1);
        // InputArray* inputArray2RowIds = new InputArray(inputArray2->rowsNum, 1);
        InputArray* inputArray1RowIds = inputArraysRowIds[predicateArray1Id];
        InputArray* inputArray2RowIds = isFilter ? NULL : inputArraysRowIds[predicateArray2Id];

        uint64_t field1Id = preds[i][1];
        uint64_t field2Id = preds[i][4];
        int operation = preds[i][2];
        // printf("inputArray1Id: %d, inputArray2Id: %d, field1Id: %lu, field2Id: %lu, operation: %d\n", inputArray1Id, inputArray2Id, field1Id, field2Id, operation);

        if (isFilter) {
            // printf("filter\n");
            uint64_t numToCompare = field2Id;
            InputArray* filteredInputArrayRowIds = inputArray1RowIds->filterRowIds(field1Id, operation, numToCompare, inputArray1);
            delete inputArray1RowIds;
            inputArraysRowIds[predicateArray1Id] = filteredInputArrayRowIds;
            continue;
        }

        switch (operation)
        {
        case 2: // '='
                // if (isFilter) {
                //     // handle filter
                //     uint64_t numToCompare = field2Id;
                //     InputArray* filteredInputArrayRowIds = inputarra
                // }
                //std::cout<<"case 2 1"<<std::endl;
                if (inputArray1Id == inputArray2Id) {
                    // self-join of InputArray
                    InputArray* filteredInputArrayRowIds = inputArray1RowIds->filterRowIds(field1Id, field2Id, inputArray1);
                    delete inputArray1RowIds;
                    inputArraysRowIds[predicateArray1Id] = filteredInputArrayRowIds;
                    continue;
                }
                                //std::cout<<"case 2 2"<<std::endl;

                if ((curIntermediateArray != NULL && curIntermediateArray->hasInputArrayId(inputArray1Id))
                    && curIntermediateArray != NULL && curIntermediateArray->hasInputArrayId(inputArray2Id)) {
                    // self-join of IntermediateArray
                    IntermediateArray* filteredIntermediateArray = curIntermediateArray->selfJoin(inputArray1Id, inputArray2Id, field1Id, field2Id, inputArray1, inputArray2);
                    delete curIntermediateArray;
                    curIntermediateArray = filteredIntermediateArray;
                    continue;
                }

                {
                                    //std::cout<<"case 2 3"<<std::endl;

                    // printf("hi %d\n", inputArray1Id);
                    // InputArray* inputArray1 = inputArrays[inputArray1Id];
                    // InputArray* inputArray2 = inputArrays[inputArray2Id];
                                        // printf("bye\n");
                    // if (/* ... */) {
                    relation rel1, rel2;
                    bool rel2ExistsInIntermediateArray = false;

                    // fill rel1
                    if (curIntermediateArray == NULL || !curIntermediateArray->hasInputArrayId(inputArray1Id)) {
                                                // printf("------------1\n");

                        // rel1.num_tuples = inputArray1RowIds->rowsNum;
                                                                        // printf("1.5\n");

                        // extractcolumn(rel1, inputArray1->columns, field1Id); ////////////////////////////////////////
                        inputArray1RowIds->extractColumnFromRowIds(rel1, field1Id, inputArray1);
                                                                        // printf("2\n");
                                                                                                // printf("rel1\n");

                        // rel1.print();
                    } else {
                        // printf("else\n");
                        // rel1.num_tuples = curIntermediateArray->rowsNum;
                                                // printf("rowsNum = %lu\n", rel1.num_tuples);
                        curIntermediateArray->extractFieldToRelation(&rel1, inputArray1, inputArray1Id, field1Id);
                        // rel1.print();
                    }
                //std::cout<<"case 2 4"<<std::endl;

                    // fill rel2
                    if (curIntermediateArray == NULL || !curIntermediateArray->hasInputArrayId(inputArray2Id)) {
                                                                        // printf("------------2\n");

                        // rel2.num_tuples = inputArray2->rowsNum;
                        // extractcolumn(rel2, inputArray2->columns, field2Id);
                        inputArray2RowIds->extractColumnFromRowIds(rel2, field2Id, inputArray2);
                        // printf("rel2\n");
                        // rel2.print();
                    } else {
                                                // printf("else\n");
                        rel2ExistsInIntermediateArray = true;
                        // rel2.num_tuples = curIntermediateArray->rowsNum;
                        curIntermediateArray->extractFieldToRelation(&rel2, inputArray2, inputArray2Id, field2Id);
                    }
                                    //std::cout<<"case 2 5"<<std::endl;

                    relation* newRel1 = new relation();
                    newRel1->num_tuples = rel1.num_tuples;
                    newRel1->tuples = new tuple[rel1.num_tuples];
                    //std::cout<<"before"<<std::endl;
                    relation* reorderedRel1 = re_ordered(&rel1, newRel1, 0);
                    //std::cout<<"after"<<std::endl;
                                    //std::cout<<"case 2 6"<<std::endl;

                    // std::cout<<"\n";
                    // ro_R->print();
                    relation* newRel2 = new relation();
                    newRel2->num_tuples = rel2.num_tuples;
                    newRel2->tuples = new tuple[rel2.num_tuples];                    
                    //std::cout<<"before"<<std::endl;
                    relation* reorderedRel2 = re_ordered(&rel2, newRel2, 0);
                    //std::cout<<"after"<<std::endl;

                                    //std::cout<<"case 2 7"<<std::endl;

                    // std::cout<<"\n";
                    // ro_S->print();
                    // std::cout<<"\n";

                    
                    result* rslt = join(rel2ExistsInIntermediateArray ? reorderedRel2 : reorderedRel1, rel2ExistsInIntermediateArray ? reorderedRel1 : reorderedRel2, inputArray1->columns, inputArray2->columns, inputArray1->columnsNum, inputArray2->columnsNum, 0);
                    // rslt->lst->print();
                    //std::cout<<"\n";
                    uint64_t** resultArray=rslt->lst->lsttoarr();
                    // int fnlx=rslt->lst->rowsz;
                    // int fnly=rslt->lst->rows;
                    // if(resultArray!=NULL)
                    // {
                    //     for(int i=0;i<fnly;i++)
                    //     {
                    //         for(int j=0;j<fnlx;j++)
                    //         {
                    //             std::cout<<resultArray[j][i]<<" ";
                    //         }
                    //         std::cout<<"\n";
                    //     }
                    //     for(int i=0;i<fnlx;i++)
                    //     {
                    //         delete[] resultArray[i];
                    //     }
                    //     delete[] resultArray;
                    // }
                    if (rslt->lst->rows == 0) {
                        // no results
                        return NULL;
                    }

                    if (curIntermediateArray == NULL) {
                        // first join
                        curIntermediateArray = new IntermediateArray(2, 0, 0);
                        curIntermediateArray->populate(resultArray, rslt->lst->rows, NULL, inputArray1Id, inputArray2Id);
                        // printf("haaa\n");
                        // printf("new IntermediateArray:\n");
                        // curIntermediateArray->print();

                    } else {
                        // printf("previous IntermediateArray:\n");
                        // curIntermediateArray->print();
                        IntermediateArray* newIntermediateArray = new IntermediateArray(curIntermediateArray->columnsNum + 1, 0, 0);
                        newIntermediateArray->populate(resultArray, rslt->lst->rows, curIntermediateArray, -1, rel2ExistsInIntermediateArray ? inputArray1Id : inputArray2Id);
                        delete curIntermediateArray;
                        curIntermediateArray = newIntermediateArray;
                        // printf("new IntermediateArray:\n");
                        // curIntermediateArray->print();
                    }
                //}
                //else
                }
            break;
        default:
            break;
        }
        
        /*******TO ANTONIS******************/
        //kathe grammi edo einai ena predicate olokliro
        //preds[i][0]=sxesi1
        //preds[i][1]=stili1
        //preds[i][2]=praxi opou
            //0  einai to >
            //1  einai to <
            //2  einai to =
        //preds[i][3]=sxesi2
            //sigrine me (uint64_t)-1 to opoio bgazei 18446744073709551615 kai theoroume an einai isa tote exoume filtro
        //preds[i][4]=stili2 
            //opou an h sxesi 2 einai isi me -1 opos eipa apo pano tote to stili 2 periexei to filtro
        /***********END***************************/
    }

    // printf("Results rows number: %lu\n", curIntermediateArray->rowsNum);
    // if (curIntermediateArray != NULL && curIntermediateArray->rowsNum > 0) {
    //     curIntermediateArray->print();
    // }

    return curIntermediateArray != NULL && curIntermediateArray->rowsNum > 0 ? curIntermediateArray : NULL;



}
void handleprojection(IntermediateArray* rowarr,InputArray** array,char* part, int* relationIds)
{
    // std::cout<<"HANDLEPROJECTION: "<<part<<std::endl;
    uint64_t projarray,projcolumn;
    for(int i=0,start=0;(i==0)||(i>0&&part[i-1])!='\0';i++)
    {
        if(part[i]=='.')
        {
            part[i]='\0';
            projarray=relationIds[atoi(part+start)];
            part[i]='.';
            start=i+1;
        }
        if(part[i]==' '||part[i]=='\0')
        {
            int flg=0;
            if(part[i]==' ')
            {
                part[i]='\0';
                flg=1;
            }
            projcolumn=atoi(part+start);
            if(flg)
                part[i]=' ';
            start=i+1;
            
            uint64_t sum=0;
            if(rowarr!=NULL)
            {
                uint64_t key;
                for(uint64_t i=0;i<rowarr->columnsNum;i++)
                {
                    if(rowarr->inputArrayIds[i]==projarray)
                        key=i;
                }
                for(uint64_t i =0;i<rowarr->rowsNum;i++)
                {
                    sum+=array[projarray]->columns[projcolumn][rowarr->results[key][i]];
                }
            }
            //std::cout<<projarray<<"."<<projcolumn<<": ";
            if(sum!=0)
                std::cout<<sum;
            else
                std::cout<<"NULL";
            if(part[i]!='\0')
                std::cout<<" ";
        }
    }
}
uint64_t** splitpreds(char* ch,int& cn)
{
    int cntr=1;
    for(int i=0;ch[i]!='\0';i++)
    {
        if(ch[i]=='&')
            cntr++;
    }
    uint64_t** preds;
    preds=new uint64_t*[cntr];
    for(int i=0;i<cntr;i++)
    {
        preds[i]=new uint64_t[5];
    }
    cntr=0;
    int start=0;
    for(int i=0;ch[i]!='\0';i++)
    {
        if(ch[i]=='&')
        {
            //preds[cntr]=ch+start;
            //start=i+1;
            ch[i]='\0';
            predsplittoterms(ch+start,preds[cntr][0],preds[cntr][1],preds[cntr][3],preds[cntr][4],preds[cntr][2]);
            cntr++;
            start=i+1;
        }
    }
    predsplittoterms(ch+start,preds[cntr][0],preds[cntr][1],preds[cntr][3],preds[cntr][4],preds[cntr][2]);
    cntr++;
    cn=cntr;
    return preds;
}
bool notin(uint64_t** check, uint64_t* in, int cntr)
{
    
    for(int j=0;j<cntr;j++)
    {
        if(check[j]==in)
            return false;
    }
    
    return true;
}
uint64_t** optimizepredicates(uint64_t** preds,int cntr,int relationsnum,int* relationIds)
{
    //filters first
    uint64_t** result=new uint64_t*[cntr];
    /*for(int i=0;i<relationsnum;i++)
    {
        for(int j=i+1;j<relationsnum;j++)
        {
            if(relationIds[i]==relationIds[j])
            {
                for(int k=0;k<relationsnum;k++)
                {
                    for(int l=0;l<5;l++)
                    {
                        if(preds[k][l]==j)
                            preds[k][l]=i;
                    }
                }
            }
        }
    }    */
    int place=0;
    for(int i=0;i<relationsnum;i++)
    {
        for(int j=0;j<cntr;j++)
        {
            if(preds[j][0]==i&&(preds[j][3]==(uint64_t)-1||preds[j][3]==i))
            {
                if(notin(result,preds[j],cntr))
                {
                    result[place]=preds[j];
                    place++;
                }
            }
        }
    }
    for(int i=0;i<relationsnum;i++)
    {
        for(int j=0;j<cntr;j++)
        {
            if(preds[j][0]==i)
            {
                if(notin(result,preds[j],cntr))
                {
                    result[place]=preds[j];
                    place++;
                }
            }
        }
    }
    delete[] preds;
    return result;
    


}

void predsplittoterms(char* pred,uint64_t& rel1,uint64_t& col1,uint64_t& rel2,uint64_t& col2,uint64_t& flag)
{
    char buffer[1024];
    rel1=col1=rel2=col2=flag=-1;
    for(int start=0,end=0,i=0,j=0,term=0;(i==0)||(i>0&&pred[i-1])!='\0';i++,j++)
    {
        if(pred[i]=='.')
        {
            buffer[j]='\0';
            if(term==0)
                rel1=atoll(buffer);
            else
                rel2=atoll(buffer);
            j=-1;
            term++;
        }
        else if(pred[i]=='>'||pred[i]=='<'||pred[i]=='=')
        {
            if(pred[i]=='>')
                flag=0;
            else if(pred[i]=='<')
                flag=1;
            else if(pred[i]=='=')
                flag=2;
            buffer[j]='\0';
            col1=atoll(buffer);
            term++;
            j=-1;
        }
        else if(pred[i]=='\0')
        {
            buffer[j]='\0';
            col2=atoll(buffer);
        }
        else
            buffer[j]=pred[i];

    }
}

