# assign1_storage_manager

****************************************************************************************************************************************
CONTIRBUTORS - 
1. Susmitha Chintareddy - schintareddy@hawk.iit.edu
2. Susmitha Marripalapu - smarripalapu@hawk.iit.edu
3. Swetha Radhakrishnan - sradhakrishnan@hawk.iit.edu
*****************************************************************************************************************************************

The storage manager is implemented in this assignment. It contains the following files:

MakeFile,
storage_mgr.h,
storage_mgr.c,
storage_mgr.o,
dberror.c,
dberror.h,
dberror.o,
test_assign1_1.h,
test_assign1_1.c,
test_assign1_1.o,
test_helper.c

We have implemented the code to Create, Open, Close, Read and Write into the files. Implementation of each function is as given below:

-----------------------------------------
Open, Create and Close functions:
-----------------------------------------

FUNCTION: RC createPageFile (char *fileName)
1. Create a new file with single page.
2. Set the file pointer at beginning of file.
3. Assign a memory block using calloc (Empty blocks are filled with 0)
4. Write empty blocks to the file. Return the error codes accordingly.

FUNCTION: RC openPageFile (char *fileName, SM_FileHandle *fHandle)
1. Check validity of fileName, fHandle
2. Move the file pointer to end of file and calculate the filesize.
3. Update the fHandle information accordingly.
4. Return RC_OK if it is successful.

FUNCTION: RC closePageFile (SM_FileHandle *fHandle)
1. Check if the file exists.
2. Close file.
3. Return RC_OK when file close is successful.

FUNCTION: RC destroyPageFile (char *fileName)
1. Check if the file exists.
2. remove file if exists.
3. Return RC_OK if deletion is successful.

-----------------------------------------
Read functions:
-----------------------------------------

FUNCTION: RC readBlock(int pageNum, SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of pageNum, fHandle, memPage
2. If the page number is negitive or exceeds the total number of pages, return an error code
3. Move the page pointer to the page number to be read
4. Read the page to memPage
5. Return success return code

FUNCTION: int getBlockPos(SM_FileHandle* fHandle)
1. Check validity of fHandle, file pointer
2. return current page position stored in file handler

FUNCTION: RC readFirstBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of fHandle.
2. Since we already have a function that reads the block, we put page number to 0 as it is first block and sent it to the readBlock function.

FUNCTION: RC readPreviousBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of fHandle.
2. Since we already have a function that reads the block, we put page number to previous page (curPagePos - 1) as it is previous block and sent it to the readBlock function.

FUNCTION: RC readCurrentBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of fHandle.
2. Since we already have a function that reads to the block, we put page number to current page (curPagePos) and sent it to the readBlock function.

FUNCTION: RC readNextBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of fHandle.
2. Since we already have a function that reads to the block, we put page number to next page (curPagePos + 1) and sent it to the readBlock function.

FUNCTION: RC readLastBlock(SM_FileHandle* fHandle, SM_PageHandle memPage)
1. Check validity of fHandle.
2. Since we already have a function that reads the block, we put page number to (totalNumPages - 1) as it moves the file pointer to the start of the last page and sent it to the readBlock function.

-----------------------------------------
Write functions:
-----------------------------------------

FUNCTION:RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
1. Obtaining fstream from mgmtInfo to check if it is Null to determine the files location.
2. Ensure the range of pageNum is not out of bounds to prevent writing a page that doesn't exist.
3. Write the content in block and update the curPagePos.

4.Return RC_OK if it is successful.

FUNCTION: RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
1. The mgmtInfo is used to obtain the file information
2. Data is updated to the current block position with the writeBlock.
3. If the value is NULL, then RC_FILE_NOT_FOUND

FUNCTION: RC appendEmptyBlock (SM_FileHandle *fHandle)
1. Check the validity of the file handle.
2. A new page of PAGE_SIZE is initialized at the end of the existing number of pages.
3. Then the total number of pages is incremented by one and the curPagePos is also updated.
4. The Write function is used to add the page content to the newly appended page.
5. If its successful, then RC_OK is returned.

FUNCTION: RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
1. The file handle validity is verified and returned RC_FILE_HANDLE_NOT_INIT if not initialised.
2. If the total number of pages is less than the given pageNum, then additional pages are appended at the end.
3. Return RC_OK, if it is successful.

-----------------------------------------
Additional supporting Functions:
-----------------------------------------

FUNCTION: void fillFileHandle(SM_FileHandle* fHandle, char* fileName, FILE* file)

1. Get the fHandle, file name and file pointer in the arguments
2. Add the file name
3. Take the current position of the file pointer and update CurPagePos
4. Move the file pointer to the end of the page. Calculate the total number of pages in the file and update the totalNumPages
5. Add the file pointer to mgmtInfo
6. Move the file pointer back to the current page position.

FUNCTION: SM_PageHandle getEmptyPage()

1. Allocate space to a page using malloc dynamic space allocation
2. Run a loop till the size of the page and in each loop add '\0' to the file
3. Return the filled page

----------------------------------------------
**** We also added few test cases to check for multipages
