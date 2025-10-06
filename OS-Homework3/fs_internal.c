#include <stdio.h>
#include <stdlib.h>
#include "buf.h"
#include "fs.h"
#include "string.h"

void SetInodeBitmap(int inodeno)
{
    int byteIndex = inodeno / 8;
    int bitIndex = inodeno % 8;
    char *pData = malloc(BLOCK_SIZE);
    BufRead(INODE_BITMAP_BLOCK_NUM, pData);
    pData[byteIndex] |= (1 << bitIndex);
    BufWrite(INODE_BITMAP_BLOCK_NUM, pData);

    free(pData);
}


void ResetInodeBitmap(int inodeno)
{
    int byteIndex = inodeno / 8;
    int bitIndex = inodeno % 8;
    char *pData = malloc(BLOCK_SIZE);
    BufRead(INODE_BITMAP_BLOCK_NUM, pData);
    pData[byteIndex] &= ~(1 << bitIndex);
    BufWrite(INODE_BITMAP_BLOCK_NUM, pData);

    free(pData);
}


void SetBlockBitmap(int blkno)
{
    int byteIndex = blkno / 8;
    int bitIndex = blkno % 8;
    char *pData = malloc(BLOCK_SIZE);
    BufRead(BLOCK_BITMAP_BLOCK_NUM, pData);
    pData[byteIndex] |= (1 << bitIndex);
    BufWrite(BLOCK_BITMAP_BLOCK_NUM, pData);

    free(pData);
}


void ResetBlockBitmap(int blkno)
{
    int byteIndex = blkno / 8;
    int bitIndex = blkno % 8;
    char *pData = malloc(BLOCK_SIZE);
    BufRead(BLOCK_BITMAP_BLOCK_NUM, pData);
    pData[byteIndex] &= ~(1 << bitIndex);
    BufWrite(BLOCK_BITMAP_BLOCK_NUM, pData);

    free(pData);
}


void PutInode(int inodeno, Inode* pInode)
{
    int blkIndex = inodeno / NUM_OF_INODE_PER_BLOCK + 3;
    int inodeIndex = inodeno % NUM_OF_INODE_PER_BLOCK;
    void *pData = malloc(BLOCK_SIZE);

    BufRead(blkIndex, pData);
    memcpy(pData + sizeof(Inode) * inodeIndex, pInode, sizeof(Inode));
    BufWrite(blkIndex, pData);

    free(pData);
}


void GetInode(int inodeno, Inode* pInode)
{
    int blkno = inodeno / NUM_OF_INODE_PER_BLOCK + 3;
    int inodeIndex = inodeno % NUM_OF_INODE_PER_BLOCK;

    void *pData = malloc(BLOCK_SIZE);
    BufRead(blkno, pData);
    
    memcpy(pInode, (char*)pData + sizeof(Inode) * inodeIndex, sizeof(Inode));

    free(pData);
}


int GetFreeInodeNum(void)
{
    void *pData = malloc(BLOCK_SIZE);
    BufRead(1, pData);

    for (int i = 0; i < BLOCK_SIZE; i++) 
    {
        unsigned char byte = ((unsigned char *)pData)[i / 8];
        if (!(byte & (1 << (i % 8))))
        {
            free(pData);
            return i;
        }
    }

    free(pData);

    return -1;
}


int GetFreeBlockNum(void)
{
    void *pData = malloc(BLOCK_SIZE);
    BufRead(2, pData);

    for (int i = INODELIST_BLOCK_FIRST + INODELIST_BLOCKS; i < BLOCK_SIZE; i++) 
    {
        unsigned char byte = ((unsigned char *)pData)[i / 8];
        if (!(byte & (1 << (i % 8))))
        {
            free(pData);
            return i;
        }
    }

    free(pData);
    return -1;
}