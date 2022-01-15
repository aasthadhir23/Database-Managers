
 #include "stdlib.h"
 #include "string.h"
#include "storage_mgr.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/*
* Function: initStorageManger
* ---------------------------
* Intializing the Storage Manager, prints a message in the output stream.
*
*/
void initStorageManager (void)
{
 printf("Storage manager initialized!");
}


/*
* Function: createPageFile
* ---------------------------
* Creates a new file with single page containing '\0' bytes
*
* fileName: Name of the file to be created
*
* return: RC_OK if the write block content to file is successful
*         RC_WRITE_FAILED if the writing fails
*
*/
RC createPageFile (char *fileName)
{
	char *firstPage, *headerPage;
	FILE *fptr = fopen(fileName, "w");
	if(fptr==NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	else
		{
			firstPage = (char*)calloc(PAGE_SIZE, sizeof(char));
			headerPage = (char*)calloc(PAGE_SIZE, sizeof(char));
			fputs("1",fptr);
			fwrite(headerPage, PAGE_SIZE, 1, fptr);
			fwrite(firstPage, PAGE_SIZE, 1, fptr);
			free(headerPage);
			free(firstPage);
			fclose(fptr);
			return RC_OK;
		}
}

/*
* Function: openPageFile
* ---------------------------
* Opens the given file and updates the details in file handle
*
* fileName: Name of the file to be opened
* fHandle: file handle related to the file
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 	  RC_FILE_NOT_FOUND if the file does not exist
*         RC_OK if the file id opened and the content is updated in the file handle
*
*/
RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	FILE *fptr = fopen(fileName, "r+");
	if(fptr==NULL)
	{
		return RC_FILE_NOT_FOUND;
	}
	else
	{
		fHandle->fileName = fileName;
		char* readHeader = (char*)calloc(PAGE_SIZE,sizeof(char));
		fgets(readHeader,PAGE_SIZE,fptr);
		char* totalPage = readHeader;
		fHandle->totalNumPages = atoi(totalPage);
		fHandle->curPagePos = 0;
		fHandle->mgmtInfo = fptr;
		free(readHeader);
		return RC_OK;
	}
}

/*
* Function: closePageFile
* ---------------------------
* Closes the file using the file pointer
*
* fHandle: File handler that contains information about the file to be deleted
*
* return: RC_FILE_NOT_FOUND if the file does not exist
*         RC_OK if the page is closed
*
*/

RC closePageFile (SM_FileHandle *fHandle)
{
	// Validation
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;

	// Action
	if (fclose(fHandle->mgmtInfo) == 0)
		return RC_OK;
}

/*
* Function: destroyPageFile
* ---------------------------
* Destroys the file using the file pointer
*
* fileName: Name of the file to be destroyed
*
* return: RC_FILE_NOT_FOUND if the file does not exist
*         RC_OK if the page is destroyed
*
*/

RC destroyPageFile (char *fileName)
{
	if (fopen(fileName,"r")==NULL)
		return RC_FILE_NOT_FOUND;
	if (remove(fileName) == 0)
		return RC_OK;
}

/*
* Function: readBlock
* ---------------------------
* Reads a block from the given page number into the memory and the file handler for that file
*
* pageNum: Page number at which the page should be written
* fHandle: File Handle that contains information about the file
* memPage: A page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		  RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*	      RC_READ_NON_EXISTING_PAGE if there is such page number present in the file
*		  RC_OK if the write successful
*
*/

RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;
	if (pageNum < 0 || pageNum > fHandle->totalNumPages-1) return RC_READ_NON_EXISTING_PAGE;
	if(!fseek(fHandle->mgmtInfo,(pageNum+1)*PAGE_SIZE,SEEK_SET))
	{
		fread(memPage,sizeof(char),PAGE_SIZE,fHandle->mgmtInfo);
		fHandle->curPagePos = pageNum;
		return RC_OK;
	}
}

/*
* Function: getBlockPos
* ---------------------------
* Returns the current page position retrieved from the file handler
*
* fHandle: File handler containing information about the file
*
* return: Integer with the current page position
*
*/

int getBlockPos (SM_FileHandle *fHandle)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;
	return fHandle->curPagePos;
}

/*
* Function: readFirstBlock
* ---------------------------
* Reads the first block of the file in the disk into the memory
*
* fHandle: File handler that contains information about the file
* memPage: Page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is empty and not initiated
*
*/

RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

	RC flag = readBlock(0,fHandle,memPage);
	if(flag != RC_OK)
		return RC_READ_NON_EXISTING_PAGE;
	else
		return RC_OK;
}

/*
* Function: readPreviousBlock
* ---------------------------
* Reads the previous block from the current position of the page pointer into the memory from the file on the disk
*
* fHandle: File handler that contains information about the file
* memPage: Page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is empty and not initiated
*
*/

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	RC flag = readBlock(getBlockPos(fHandle)-1,fHandle,memPage);
	if(flag != RC_OK)
		return RC_READ_NON_EXISTING_PAGE;
	else
		return RC_OK;
}

/*
* Function: readCurrentBlock
* ---------------------------
* Reads the current block that the page pointer is pointing into the memory from the file on the disk
*
* fHandle: File handler that contains information about the file
* memPage: Page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is empty and not initiated
*
*/

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;

	RC flag = readBlock(getBlockPos(fHandle),fHandle,memPage);
	if(flag != RC_OK)
		return RC_READ_NON_EXISTING_PAGE;
	else
		return RC_OK;
}

/*
* Function: readNextBlock
* ---------------------------
* Reads the next block from the current page position into the memory from the file on the disk
*
* fHandle: File handler that contains information about the file
* memPage: Page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is empty and not initiated
*
*/

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	RC flag = readBlock(getBlockPos(fHandle)+1,fHandle,memPage);
	if(flag != RC_OK)
		return RC_READ_NON_EXISTING_PAGE;
	else
		return RC_OK;
}

/*
* Function: readLastBlock
* ---------------------------
* Reads the last block into the memory from the file on the disk
*
* fHandle: File handler that contains information about the file
* memPage: Page handler to which the data will be read into
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is empty and not initiated
*
*/

RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	RC flag = readBlock(fHandle->totalNumPages - 1,fHandle,memPage);
	if(flag != RC_OK) return RC_READ_NON_EXISTING_PAGE;
	else return RC_OK;
}

/*
* Function: writeBlock
* ---------------------------
* Writes a new block in the given page number and updated the file handler
*
* pageNum: Page number at which the page should be written
* fHandle: File Handle that contains information about the file
* memPage: A page handler whose data will be written to the file
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		  RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*	      RC_WRITE_FAILED if there is no space in the file
*		  RC_OK if the write successful
*
*/

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;
	if (pageNum < 0 || pageNum > fHandle->totalNumPages - 1) return RC_WRITE_FAILED;

	fseek(fHandle->mgmtInfo,(pageNum+1)*PAGE_SIZE,SEEK_SET);
	fwrite(memPage,PAGE_SIZE,1,fHandle->mgmtInfo);
	fHandle->curPagePos = pageNum;
	return RC_OK;

}

/*
* Function: writeCurrentBlock
* ---------------------------
* Checks if the the file is full. If it is empty writes a new block in the current page position
*
* fHandle: Name of the file to be created
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		  RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*	      RC_WRITE_FAILED if there is no space in the file
*
*/

RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	// Validation
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;
	if (fHandle->curPagePos > fHandle->totalNumPages - 1) return RC_WRITE_FAILED;

	return writeBlock(getBlockPos(fHandle),fHandle,memPage);
}

/*
* Function: appendEmptyBlock
* ---------------------------
* Add an empty block to the end of the file
*
* fHandle: File handler that holds data of the file
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		  RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*		  RC_OK if an empty block is appended to the end of the file
*
*/

RC appendEmptyBlock (SM_FileHandle *fHandle)
{
	// Validation
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;

	char * newPage = (char*)calloc(PAGE_SIZE, sizeof(char));
	fseek(fHandle->mgmtInfo,(fHandle->totalNumPages+1)*PAGE_SIZE,SEEK_SET);
	if(fwrite(newPage, PAGE_SIZE, 1, fHandle->mgmtInfo))
	{
		fHandle->totalNumPages +=1;
		fHandle->curPagePos = fHandle->totalNumPages - 1;
		fseek(fHandle->mgmtInfo,0L,SEEK_SET);
		fprintf(fHandle->mgmtInfo, "%d", fHandle->totalNumPages);
		fseek(fHandle->mgmtInfo,(fHandle->totalNumPages+1)*PAGE_SIZE,SEEK_SET);
		free(newPage);
		return RC_OK;
	}
	else
	{
		free(newPage);
		return RC_WRITE_FAILED;
	}
}

/*
* Function: ensureCapacity
* ---------------------------
* Ensures if a file contains the given number of pages
*
* numberOfPages: Total pages that a file should contain
* fHandle: Fiel handle fo the file whose pages needs to be checked
*
* return: RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		  RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*		  RC_OK if there are given number of pages in the file
*
*/

RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
	// Validation
	if (fHandle == NULL) return RC_FILE_HANDLE_NOT_INIT;
	if (fHandle->mgmtInfo == NULL) return RC_FILE_NOT_FOUND;

	// Action
	int itr = 0;
	for (itr = fHandle->totalNumPages; itr < numberOfPages; ++itr) {
		appendEmptyBlock(fHandle);
	}
	return RC_OK;
}
