#ifndef __EXT_H__
#define __EXT_H__

#include "buf.h"

extern Buf* BufGetNewBuffer(void);
extern void BufInsert(Buf* pBuf, int blkno, BufStateList listNum);
extern void BufDelete(Buf* pBuf);
extern void UpdateOrderInLruList(Buf* pBuf);
extern Buf* BufGetVictimBuffer(void);

#endif