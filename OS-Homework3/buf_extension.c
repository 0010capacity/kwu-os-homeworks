#include "buf_extension.h"
#include "disk.h"

Buf* BufGetNewBuffer(void)
{
    Buf* buf;

    if(CIRCLEQ_EMPTY(&freeListHead))
    {
        buf = BufGetVictimBuffer();
    }
    else
    {
        buf = CIRCLEQ_LAST(&freeListHead);
        CIRCLEQ_REMOVE(&freeListHead, buf, flist);
    }

    return buf;
}

void BufInsert(Buf* pBuf, int blkno, BufStateList listNum)
{
    pBuf->blkno = blkno;
    pBuf->state = (BufState)listNum;
    
    CIRCLEQ_INSERT_HEAD(&bufList[blkno % MAX_BUFLIST_NUM], pBuf, blist);
    CIRCLEQ_INSERT_TAIL(&stateList[listNum], pBuf, slist);
}

void BufDelete(Buf* pBuf)
{
    CIRCLEQ_REMOVE(&stateList[pBuf->state], pBuf, slist);
    CIRCLEQ_REMOVE(&bufList[pBuf->blkno % MAX_BUFLIST_NUM], pBuf, blist);
}

void UpdateOrderInLruList(Buf* pBuf)
{
    CIRCLEQ_REMOVE(&lruListHead, pBuf, llist);
    CIRCLEQ_INSERT_TAIL(&lruListHead, pBuf, llist);
}

Buf* BufGetVictimBuffer(void)
{
    Buf* buf = CIRCLEQ_FIRST(&lruListHead);

    if(buf->state == BUF_STATE_DIRTY)
    {
        DevWriteBlock(buf->blkno, buf->pMem);
    }

    CIRCLEQ_REMOVE(&lruListHead, buf, llist);
    BufDelete(buf);

    return buf;
}