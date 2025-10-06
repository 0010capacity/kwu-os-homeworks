#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "queue.h"
#include "disk.h"
#include "buf_extension.h"

struct bufList      bufList[MAX_BUFLIST_NUM];
struct stateList    stateList[MAX_BUF_STATE_NUM];
struct freeList     freeListHead;
struct lruList      lruListHead;


void BufInit(void)
{
    for (int i = 0; i < MAX_BUFLIST_NUM; i++){
        CIRCLEQ_INIT(&bufList[i]);
    }

    for (int i = 0; i < MAX_BUF_STATE_NUM; i++){
        CIRCLEQ_INIT(&stateList[i]);
    }

    CIRCLEQ_INIT(&freeListHead);
    CIRCLEQ_INIT(&lruListHead);

    for (int i = 0; i < MAX_BUF_NUM; i++){
        Buf* buf = malloc(sizeof(Buf));
        buf->blkno = BLKNO_INVALID;
        buf->state = BLKNO_INVALID;
        buf->pMem = malloc(BLOCK_SIZE);

        CIRCLEQ_INSERT_TAIL(&freeListHead, buf, flist);
    }
}

Buf* BufFind(int blkno)
{
    Buf* target = NULL;

    CIRCLEQ_FOREACH(target, &bufList[blkno % MAX_BUFLIST_NUM], blist)
    {
        if(target->blkno == blkno)
        {
            return target;
        }
    }

    return NULL;
}


void BufRead(int blkno, char* pData)
{
    Buf* buf = BufFind(blkno);

    if(buf != NULL)
    {
        memcpy(pData, buf->pMem, BLOCK_SIZE);
        UpdateOrderInLruList(buf);
        return;
    } 

    buf = BufGetNewBuffer();

    DevReadBlock(blkno, buf->pMem);
    memcpy(pData, buf->pMem, BLOCK_SIZE);

    BufInsert(buf, blkno, BUF_CLEAN_LIST);
    CIRCLEQ_INSERT_TAIL(&lruListHead, buf, llist);
}


void BufWrite(int blkno, char* pData)
{
    Buf* buf = BufFind(blkno);

    if(buf != NULL){
        memcpy(buf->pMem, pData, BLOCK_SIZE);

        if(buf->state == BUF_CLEAN_LIST)
        {
            BufDelete(buf);
            
            BufInsert(buf, blkno, BUF_DIRTY_LIST);
        }
        else if(buf->state == BLKNO_INVALID)
        {
            BufInsert(buf, blkno, BUF_DIRTY_LIST);
        }

        UpdateOrderInLruList(buf);
    }
    else
    {
        buf = BufGetNewBuffer();
        memcpy(buf->pMem, pData, BLOCK_SIZE);

        BufInsert(buf, blkno, BUF_DIRTY_LIST);
        CIRCLEQ_INSERT_TAIL(&lruListHead, buf, llist);
    }
}

void BufSync(void)
{
    Buf *buf = CIRCLEQ_FIRST(&stateList[BUF_DIRTY_LIST]);
    Buf *nextBuf = NULL;

    while(!CIRCLEQ_EMPTY(&stateList[BUF_DIRTY_LIST]))
    {    
        nextBuf = CIRCLEQ_NEXT(buf, slist);

        DevWriteBlock(buf->blkno, buf->pMem);
        BufDelete(buf);
        BufInsert(buf, buf->blkno, BUF_CLEAN_LIST);

        buf = nextBuf;
    }
}

void BufSyncBlock(int blkno)
{
    Buf* buf = BufFind(blkno);
    if(buf!=NULL){
        if(buf->state == BUF_DIRTY_LIST)
        {
            DevWriteBlock(buf->blkno, buf->pMem);
            BufDelete(buf);
            BufInsert(buf, blkno, BUF_CLEAN_LIST);
        }
    }
}


int GetBufInfoInStateList(BufStateList listnum, Buf* ppBufInfo[], int numBuf)
{
    Buf* cursor;

    int size = 0;
    CIRCLEQ_FOREACH(cursor, &stateList[listnum], slist)
    {
        if(size == numBuf) break;
        ppBufInfo[size++] = cursor;
    }

    return size;
}

int GetBufInfoInLruList(Buf* ppBufInfo[], int numBuf)
{
    Buf* cursor;

    int size = 0;
    CIRCLEQ_FOREACH(cursor, &lruListHead, llist)
    {
        if(size == numBuf) break;
        ppBufInfo[size++] = cursor;
    }

    return size;
}


int GetBufInfoInBufferList(int index, Buf* ppBufInfo[], int numBuf)
{
    Buf* cursor;

    int size = 0;
    CIRCLEQ_FOREACH(cursor, &bufList[index], blist)
    {
        if(size == numBuf) break;
        ppBufInfo[size++] = cursor;
    }

    return size;
}