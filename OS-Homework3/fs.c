#include <stdio.h>
#include <stdlib.h>
#include "buf.h"
#include "fs.h"
#include "string.h"

FileDesc pFileDesc[MAX_FD_ENTRY_MAX];
FileSysInfo* pFileSysInfo;

int     CreateFile(const char* szFileName)
{
    int inodeno = GetFreeInodeNum();

    Inode *pParentInode = malloc(sizeof(Inode));
    GetInode(0, pParentInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szFileName), "/");
    
    char *fileName;
    int parentInodeno = 0;
    int parentBlkno = 7;
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pParentInode->dirBlockPtr[directPtrIndex] != 0) {
                parentBlkno = pParentInode->dirBlockPtr[directPtrIndex];

                BufRead(pParentInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        parentInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(parentInodeno, pParentInode);

                        fileName = token;
                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            fileName = token;
            token = strtok(NULL, "/");
            break;
        }
    }

    if(token != NULL) {
        free(pParentInode);
        free(pParentDirEntry);
        return -1;
    }

    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        BOOL found = FALSE;

        if(pParentInode->dirBlockPtr[i] == 0){
            // 다이렉트 블록이 없다면 새롭게 할당
            parentBlkno = GetFreeBlockNum();
            BufRead(parentBlkno, (char*)pParentDirEntry);
            directPtrIndex = i;
            pFileSysInfo->numFreeBlocks--;
            pFileSysInfo->numAllocBlocks++;
            pParentInode->allocBlocks++;
            SetBlockBitmap(parentBlkno);

            pParentInode->dirBlockPtr[i] = parentBlkno;
            PutInode(parentInodeno, pParentInode);
        }

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (pParentDirEntry[j].name[0] == '\0') {
                strcpy(pParentDirEntry[j].name, fileName);
                pParentDirEntry[j].inodeNum = inodeno;
                found = TRUE;
                break;
            }
        }

        if(found == TRUE) break;
    }

    BufWrite(parentBlkno, (char*)pParentDirEntry);

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    pInode->type = FILE_TYPE_FILE;
    pInode->size = 0;
    pInode->allocBlocks = 1;

    PutInode(inodeno, pInode);
    SetInodeBitmap(inodeno);


    pFileSysInfo->numAllocInodes++;


    int index;
    for(index = 0; index<MAX_FD_ENTRY_MAX; index++){
        if(pFileDesc[index].bUsed == 0) break;
    }

    File *file = malloc(sizeof(File));
    file->inodeNum = inodeno;
    file->fileOffset = 0;

    pFileDesc[index].bUsed = 1;
    pFileDesc[index].pOpenFile = file;

    free(pParentDirEntry);
    free(pParentInode);
    free(pInode);

    return index;
}

int     OpenFile(const char* szFileName)
{
    Inode *pParentInode = malloc(sizeof(Inode));
    GetInode(0, pParentInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szFileName), "/");

    int parentInodeno = 0;
    
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pParentInode->dirBlockPtr[directPtrIndex] != 0) {
                BufRead(pParentInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        parentInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(parentInodeno, pParentInode);

                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            token = strtok(NULL, "/");
            break;
        }
    }
    if(token != NULL){
        free(pParentInode);
        free(pParentDirEntry);
        return -1;
    } 

    int index;
    for(index = 0; index<MAX_FD_ENTRY_MAX; index++){
        if(pFileDesc[index].bUsed == 0) break;
    }

    File *file = malloc(sizeof(File));
    file->inodeNum = parentInodeno;
    file->fileOffset = 0;

    pFileDesc[index].bUsed = 1;
    pFileDesc[index].pOpenFile = file;

    free(pParentInode);
    free(pParentDirEntry);

    return index;
}


int WriteFile(int fileDesc, char* pBuffer, int length) {
    File *file = pFileDesc[fileDesc].pOpenFile;

    int blkno;
    int inodeno = file->inodeNum;

    
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);
    

    int directptrIndex = file->fileOffset / BLOCK_SIZE;
    int blockOffset = file->fileOffset % BLOCK_SIZE;
    int remainLength = length;
    int writeOffset = 0;

    char* pBuf = malloc(BLOCK_SIZE);

    pInode->size += length;

    while (remainLength > 0) {
        if (blockOffset > 0) {
            blkno = pInode->dirBlockPtr[directptrIndex];

            BufRead(blkno, pBuf);
            int copyLength = (BLOCK_SIZE - blockOffset) > remainLength ? remainLength : (BLOCK_SIZE - blockOffset);
            memcpy(pBuf + blockOffset, pBuffer + writeOffset, copyLength);
            BufWrite(blkno, pBuf);

            remainLength -= copyLength;
            writeOffset += copyLength;
            file->fileOffset += copyLength;
            blockOffset = 0;
        } else {
            blkno = GetFreeBlockNum();

            for (int i = 0; i < NUM_OF_DIRECT_BLOCK_PTR; i++) {
                if (pInode->dirBlockPtr[i] == 0) {
                    pInode->dirBlockPtr[i] = blkno;
                    break;
                }
            }

            pInode->allocBlocks++;
            PutInode(inodeno, pInode);
            SetBlockBitmap(blkno);

            pFileSysInfo->numAllocBlocks++;
            pFileSysInfo->numFreeBlocks--;

            int copyLength = remainLength > BLOCK_SIZE ? BLOCK_SIZE : remainLength;
            memcpy(pBuf, pBuffer + writeOffset, copyLength);
            BufWrite(blkno, pBuf);

            remainLength -= copyLength;
            writeOffset += copyLength;
            file->fileOffset += copyLength;
        }

        directptrIndex = file->fileOffset / BLOCK_SIZE;
    }

    free(pBuf);
    free(pInode);

    return length;
}


int ReadFile(int fileDesc, char* pBuffer, int length) {
    File *file = pFileDesc[fileDesc].pOpenFile;
    int inodeno = file->inodeNum;

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    int logicalBlkno = file->fileOffset / BLOCK_SIZE;
    int blockOffset = file->fileOffset % BLOCK_SIZE;
    int remainLength = length;
    int readOffset = 0;

    char* pBuf = malloc(BLOCK_SIZE);

    while (remainLength > 0) {
        int blkno = pInode->dirBlockPtr[logicalBlkno];

        if (blkno == 0) {
            memset(pBuffer + readOffset, 0, remainLength);
            break;
        }

        BufRead(blkno, pBuf);

        int copyLength = (BLOCK_SIZE - blockOffset) > remainLength ? remainLength : (BLOCK_SIZE - blockOffset);
        memcpy(pBuffer + readOffset, pBuf + blockOffset, copyLength);

        remainLength -= copyLength;
        readOffset += copyLength;
        file->fileOffset += copyLength;
        blockOffset = 0;
        logicalBlkno++;
    }

    free(pInode);
    free(pBuf);

    return length - remainLength;
}



int     CloseFile(int fileDesc)
{
    File *file = pFileDesc[fileDesc].pOpenFile;
    free(file);
    pFileDesc[fileDesc].bUsed = 0;
    pFileDesc[fileDesc].pOpenFile = NULL;

    return 1;
}

int     RemoveFile(const char* szFileName)
{
    Inode *pParentInode = malloc(sizeof(Inode));
    GetInode(0, pParentInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szFileName), "/");

    int parentInodeno = 0;
    int parentBlkno = 7;
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pParentInode->dirBlockPtr[directPtrIndex] != 0) {
                parentBlkno = pParentInode->dirBlockPtr[directPtrIndex];

                BufRead(pParentInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        parentInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(parentInodeno, pParentInode);

                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            token = strtok(NULL, "/");
            break;
        }
    }
    if(token != NULL){
        free(pParentInode);
        free(pParentDirEntry);
        return -1;
    } 

    strcpy(pParentDirEntry[direntIndex].name, "\0");
    pParentDirEntry[direntIndex].inodeNum = 0;

    BufWrite(parentBlkno, (char*)pParentDirEntry);

    int blkCount = 0;

    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        int blkno = pParentInode->dirBlockPtr[i];

        if(blkno != 0){
            blkCount++;
            ResetBlockBitmap(blkno);
            char *pBuf = malloc(BLOCK_SIZE);
            BufRead(blkno, pBuf);

            memset(pBuf, 0, BLOCK_SIZE);
            
            BufWrite(blkno, pBuf);
        }
    }
    ResetInodeBitmap(parentInodeno);

    pFileSysInfo->numAllocBlocks -= blkCount;
    pFileSysInfo->numFreeBlocks += blkCount;
    pFileSysInfo->numAllocInodes--;

    int index;
    for(index = 0; index<MAX_FD_ENTRY_MAX; index++){
        if(pFileDesc[index].pOpenFile != NULL){
            if (pFileDesc[index].pOpenFile->inodeNum == parentInodeno)
            {
                pFileDesc[index].bUsed = 0;
                free(pFileDesc[index].pOpenFile);
                pFileDesc[index].pOpenFile = NULL;
                break;
            }
        }
    }

    return 1;
}


int     MakeDir(const char* szDirName)
{
    int blkno = GetFreeBlockNum();
    int inodeno = GetFreeInodeNum();

    Inode *pParentInode = malloc(sizeof(Inode));
    GetInode(0, pParentInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szDirName), "/");
    
    char *fileName;
    int parentInodeno = 0;
    int parentBlkno = 7;
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pParentInode->dirBlockPtr[directPtrIndex] != 0) {
                parentBlkno = pParentInode->dirBlockPtr[directPtrIndex];

                BufRead(pParentInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        parentInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(parentInodeno, pParentInode);

                        fileName = token;
                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            fileName = token;
            token = strtok(NULL, "/");
            break;
        }
    }
    if(token != NULL){
        free(pParentInode);
        free(pParentDirEntry);
        return -1;
    } 

    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        BOOL found = FALSE;

        if(pParentInode->dirBlockPtr[i] == 0){
            // 다이렉트 블록이 없다면 새롭게 할당
            SetBlockBitmap(blkno);
            parentBlkno = GetFreeBlockNum();
            BufRead(parentBlkno, (char*)pParentDirEntry);
            directPtrIndex = i;
            pFileSysInfo->numFreeBlocks--;
            pFileSysInfo->numAllocBlocks--;
            pParentInode->allocBlocks++;
            SetBlockBitmap(parentBlkno);
            
            pParentInode->dirBlockPtr[i] = parentBlkno;
            PutInode(parentInodeno, pParentInode);
            
        }

        for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
            if (pParentDirEntry[j].name[0] == '\0') {
                strcpy(pParentDirEntry[j].name, fileName);
                pParentDirEntry[j].inodeNum = inodeno;
                found = TRUE;
                break;
            }
        }

        if(found == TRUE) break;
    }
    
    BufWrite(parentBlkno, (char*)pParentDirEntry);

    DirEntry *pDirEntry = (DirEntry *) malloc(BLOCK_SIZE);
    memset(pDirEntry, 0, BLOCK_SIZE);

    strcpy(pDirEntry[0].name, ".");
    pDirEntry[0].inodeNum = inodeno;

    strcpy(pDirEntry[1].name, "..");
    pDirEntry[1].inodeNum = parentInodeno;

    BufWrite(blkno, (char*)pDirEntry);

    Inode *pInode = malloc(sizeof(Inode));
    GetInode(inodeno, pInode);

    pInode->dirBlockPtr[0] = blkno;
    pInode->type = FILE_TYPE_DIR;
    pInode->size = 512;
    pInode->allocBlocks = 1;

    PutInode(inodeno, pInode);

    SetBlockBitmap(blkno);
    SetInodeBitmap(inodeno);

    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    pFileSysInfo->numAllocInodes++;

    free(pDirEntry);
    free(pParentDirEntry);
    free(pParentInode);
    free(pInode);
    
    return 1;
}


int     RemoveDir(const char* szDirName)
{
    Inode *pParentInode = malloc(sizeof(Inode));
    GetInode(0, pParentInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szDirName), "/");
    
    int parentInodeno = 0;
    int parentBlkno = 7;
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pParentInode->dirBlockPtr[directPtrIndex] != 0) {
                parentBlkno = pParentInode->dirBlockPtr[directPtrIndex];

                BufRead(pParentInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        parentInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(parentInodeno, pParentInode);

                        token = strtok(NULL, "/");
                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            token = strtok(NULL, "/");
            break;
        }
    }
    if(token != NULL){
        free(pParentInode);
        free(pParentDirEntry);
        return -1;
    } 

    strcpy(pParentDirEntry[direntIndex].name, "\0");
    pParentDirEntry[direntIndex].inodeNum = 0;

    BufWrite(parentBlkno, (char*)pParentDirEntry);

    int blkCount = 0;
    pParentInode->allocBlocks = 0;
    pParentInode->size = 0;

    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        int blkno = pParentInode->dirBlockPtr[i];

        if(blkno != 0){
            blkCount++;
            ResetBlockBitmap(blkno);
            char *pBuf = malloc(BLOCK_SIZE);
            BufRead(blkno, pBuf);

            memset(pBuf, 0, BLOCK_SIZE);
            
            BufWrite(blkno, pBuf);
        }
    }
    ResetInodeBitmap(parentInodeno);
    
    pFileSysInfo->numAllocBlocks -= blkCount;
    pFileSysInfo->numFreeBlocks += blkCount;
    pFileSysInfo->numAllocInodes--;

    free(pParentInode);
    free(pParentDirEntry);

    return 1;
}

int   EnumerateDirStatus(const char* szDirName, DirEntryInfo* pDirEntry, int dirEntrys)
{
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(0, pInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szDirName), "/");
    
    int pInodeno = 0;
    
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pInode->dirBlockPtr[directPtrIndex] != 0) {

                BufRead(pInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        pInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(pInodeno, pInode);

                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            token = strtok(NULL, "/");
            break;
        }
    }

    if(token != NULL){
        free(pInode);
        free(pParentDirEntry);
        return -1;
    } 

    int count = 0;
    Inode* pTempInode = malloc(sizeof(Inode));

    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        if(pInode->dirBlockPtr[i] != 0){
            int blkno = pInode->dirBlockPtr[i];
            BufRead(blkno, (char*)pParentDirEntry);

            for (int j = 0; j < NUM_OF_DIRENT_PER_BLOCK; j++) {
                if(pParentDirEntry[j].name[0] != '\0'){
                    
                    GetInode(pParentDirEntry[j].inodeNum, pTempInode);
                    pDirEntry[count].inodeNum = pParentDirEntry[j].inodeNum;
                    strcpy(pDirEntry[count].name, pParentDirEntry[j].name);
                    pDirEntry[count].type = pTempInode->type;
                    if(++count == dirEntrys) break;
                }
            }
            
        }
    }

    free(pTempInode);
    free(pInode);
    free(pParentDirEntry);

    return count;
}


void    CreateFileSystem()
{
    FileSysInit();

    SetBlockBitmap(0);
    SetBlockBitmap(1);
    SetBlockBitmap(2);
    SetBlockBitmap(3);
    SetBlockBitmap(4);
    SetBlockBitmap(5);
    SetBlockBitmap(6);

    int blkno = GetFreeBlockNum();
    int inodeno = GetFreeInodeNum();

    DirEntry* pDirEntry = (DirEntry*)malloc(BLOCK_SIZE);
    memset(pDirEntry, 0, BLOCK_SIZE);

    strcpy(pDirEntry[0].name, ".");
    pDirEntry[0].inodeNum = 0;

    BufWrite(7, (char*)pDirEntry);

    pFileSysInfo = malloc(BLOCK_SIZE);

    pFileSysInfo->blocks = 0;
    pFileSysInfo->rootInodeNum = 0;
    pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
    pFileSysInfo->numAllocBlocks = 0;
    pFileSysInfo->numFreeBlocks = BLOCK_SIZE * 8;
    pFileSysInfo->numAllocInodes = 0;
    pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeListBlock = INODELIST_BLOCK_FIRST;
    pFileSysInfo->dataRegionBlock = INODELIST_BLOCK_FIRST + INODELIST_BLOCKS;

    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--;
    pFileSysInfo->numAllocInodes++;

    BufWrite(0, (char*)pFileSysInfo);

    SetBlockBitmap(7);
    SetInodeBitmap(0);

    Inode *pInode = malloc(sizeof(Inode));
        
    GetInode(inodeno, pInode);

    pInode->dirBlockPtr[0] = blkno;
    pInode->type = FILE_TYPE_DIR;
    pInode->size = 512;
    pInode->allocBlocks = 1;
    
    PutInode(inodeno, pInode);

    free(pDirEntry);
    free(pInode);
}


void    OpenFileSystem()
{
    DevOpenDisk();
    BufInit();
    pFileSysInfo = malloc(BLOCK_SIZE);
    BufRead(0, (char*)pFileSysInfo);
}

void     FileSysInit()
{
    DevCreateDisk();
    BufInit();

    int blocks = FS_DISK_CAPACITY / BLOCK_SIZE;
    void* pBuf = malloc(BLOCK_SIZE);
    memset(pBuf, 0, BLOCK_SIZE);

    for(int i=0; i<blocks; i++)
    {
        BufWrite(i, pBuf);
    }

    free(pBuf);
}

void    CloseFileSystem()
{
    BufWrite(0, (char*)pFileSysInfo);
    BufSync();
    DevCloseDisk();
}

int      GetFileStatus(const char* szPathName, FileStatus* pStatus){
    Inode *pInode = malloc(sizeof(Inode));
    GetInode(0, pInode);

    DirEntry *pParentDirEntry = malloc(BLOCK_SIZE);
    BufRead(7, (char*)pParentDirEntry);

    char *token = strtok(strdup(szPathName), "/");
    
    int pInodeno = 0;
    
    int direntIndex = 0;
    int directPtrIndex = 0;

    while(token != NULL) {
        BOOL found = FALSE;

        for(directPtrIndex = 0; directPtrIndex < NUM_OF_DIRECT_BLOCK_PTR; directPtrIndex++) {
            if(pInode->dirBlockPtr[directPtrIndex] != 0) {

                BufRead(pInode->dirBlockPtr[directPtrIndex], (char*) pParentDirEntry);

                for(direntIndex = 0; direntIndex < NUM_OF_DIRENT_PER_BLOCK; direntIndex++) {
                    if(strcmp(pParentDirEntry[direntIndex].name, token) == 0) {
                        pInodeno = pParentDirEntry[direntIndex].inodeNum;
                        GetInode(pInodeno, pInode);

                        token = strtok(NULL, "/");

                        found = TRUE;
                        break;
                    }
                }
                if (found == TRUE) break;
            }
        }

        if (found == FALSE) {
            token = strtok(NULL, "/");
            break;
        }
    }
    if(token != NULL){
        free(pInode);
        free(pParentDirEntry);
        return -1;
    }

    pStatus->allocBlocks = pInode->allocBlocks;
    pStatus->size = pInode->size;
    pStatus->type = pInode->type;


    for(int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++){
        pStatus->dirBlockPtr[i] = pInode->dirBlockPtr[i];
    }

    free(pInode);
    free(pParentDirEntry);

    return 1;
}

void	Sync(){
    BufSync();
}