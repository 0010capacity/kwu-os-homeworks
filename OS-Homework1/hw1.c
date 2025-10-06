#include <stdio.h>
#include <stdlib.h>
#include "hw1.h"

struct hashTable ppHashTable[HASH_TBL_SIZE];
struct list ppListHead[MAX_LIST_NUM];

void        Init(void)
{
    for (int i = 0; i < HASH_TBL_SIZE; i++){
        CIRCLEQ_INIT(&ppHashTable[i]);
    }

    for (int i = 0; i < MAX_LIST_NUM; i++){
        CIRCLEQ_INIT(&ppListHead[i]);
    }
}


void        InsertObjectToTail(Object* pObj, int objNum, List listNum)
{
    pObj->objnum = objNum;

    if(listNum != LIST3) CIRCLEQ_INSERT_TAIL(&ppHashTable[objNum % HASH_TBL_SIZE], pObj, hash);
    CIRCLEQ_INSERT_TAIL(&ppListHead[listNum], pObj, link);
}


void        InsertObjectToHead(Object* pObj, int objNum, List listNum)
{
    pObj->objnum = objNum;

    if(listNum != LIST3) CIRCLEQ_INSERT_HEAD(&ppHashTable[objNum % HASH_TBL_SIZE], pObj, hash);
    CIRCLEQ_INSERT_TAIL(&ppListHead[listNum], pObj, link);
}


Object*     FindObjectByNum(int objnum)
{
    Object* target;

    CIRCLEQ_FOREACH(target, &ppHashTable[objnum % HASH_TBL_SIZE], hash)
    {
        if(target == NULL) break;
        if(target->objnum == objnum)
        {
            return target;
        }
    }

    CIRCLEQ_FOREACH(target, &ppListHead[LIST3], link)
    {
        if(target == NULL) break;
        if(target->objnum == objnum)
        {
            return target;
        }
    }

    return NULL;
}


BOOL        DeleteObject(Object* pObj)
{
    Object* target;
    BOOL isFound = FALSE;

    if(pObj == NULL) return FALSE;

    CIRCLEQ_FOREACH(target, &ppListHead[LIST1], link){
        if(target == pObj){
            CIRCLEQ_REMOVE(&ppListHead[LIST1], target, link);
            isFound = TRUE;
            break;
        }
    }

    if(isFound == FALSE){
            CIRCLEQ_FOREACH(target, &ppListHead[LIST2], link){
            if(target == pObj){
                CIRCLEQ_REMOVE(&ppListHead[LIST2], target, link);
                isFound = TRUE;
                break;
            }
        }
    }

    if(isFound == TRUE){
        CIRCLEQ_REMOVE(&ppHashTable[pObj->objnum % HASH_TBL_SIZE], pObj, hash);
        return TRUE;
    }

    CIRCLEQ_FOREACH(target, &ppListHead[LIST3], link){
        if(target == pObj){
            CIRCLEQ_REMOVE(&ppListHead[LIST3], target, link);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL        DeleteObjectByNum(int objNum)
{
    Object* target = FindObjectByNum(objNum);
    return DeleteObject(target);
}


int         EnumberateObjectsByListNum(List listnum, Object* ppObject[], int count)
{
    *ppObject = (Object*)malloc(sizeof(Object) * count);
    Object *cursor;

    int index = 0;
    CIRCLEQ_FOREACH(cursor, &ppListHead[listnum], link)
    {
        if(cursor == NULL) break;
        if(index == count) break;
        ppObject[index++] = cursor;
    }

    return index;
}

int      EnumberateObjectsByHashIndex(int index, Object* ppObject[], int count)
{
    *ppObject = (Object*)malloc(sizeof(Object*) * count);
    Object *cursor;

    int j = 0;
    CIRCLEQ_FOREACH(cursor, &ppHashTable[index], hash)
    {
        if(cursor == NULL) break;
        if(j == count) break;
        ppObject[j++] = cursor;
    }

    return j;
}