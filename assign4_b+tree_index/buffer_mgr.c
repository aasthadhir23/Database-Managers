
#include "stdio.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "stdlib.h"
#include "string.h"

/*Structure for Page Frame inside BufferPool
	This contains pointers to the next and previous frams inside the buffer to form nodes of doubly linked list.
	Page frame infoe=rmation like pagenumber, data in page and dirty flag are maintained*. */

typedef struct PageFrame
{
	int frameNum;
	int pageNum;
	int dirtyFlag;
	int fixCount;
	int refBit;
	char *data;
	struct pageFrame *next, *prev;
}PageFrame;

/*Structure for Buffer Pool Manager
	This contains datastructures to navigate through bufferpool using linkedlists.
	Some variables to get the statistics are also declared in this structure.*/

typedef struct BManager
{
	int occupiedCount;
	void *stratData;
	PageFrame *head,*tail,*start;
	PageNumber *frameContent;
	int *fixCount;
	bool *dirtyBit;
	int numRead;
	int numWrite;
}BManager;

/*
 * Function: createFrame
 * ---------------------------
 * This function creates a buffer pool with page frames given.
 *
 * mgmt: Structure which stores information about the buffer manager.
 *
 * return: void
 *
 */

void createPageFrame(BManager *mgmt)
{
	PageFrame *frame = (PageFrame *) malloc(sizeof(PageFrame));
	frame->dirtyFlag = 0;
	frame->fixCount = 0;
	frame->frameNum = 0;
	frame->pageNum = -1;
	frame->refBit = 0;
	frame->data = calloc(PAGE_SIZE,sizeof(char*));
	mgmt->head = mgmt->start;

	if(mgmt->head != NULL)
	{
		mgmt->tail->next = frame;
		frame->prev = mgmt->tail;
		mgmt->tail = mgmt->tail->next;
	}
	else
	{
		mgmt->head = frame;
		mgmt->tail = frame;
		mgmt->start = frame;
	}
	mgmt->tail->next = mgmt->head;
	mgmt->head->prev = mgmt->tail;
}

/*
* Function: initBufferPool
* ---------------------------
* Creates a new Buffer pool
*
* bm: Structure which stores information about the buffer pool
* pagefileName: Specifies name of pageFile from which pages should be cached
* numPages: Number of pages in a buffer pool
* strategy: Specify the page replavement algorithm used.
* stratData: Used to pass any extra parameters for working of strategy
*
* return: RC_OK if the bufferpool creation is successful
*         RC_BUFFER_POOL_ALREADY_INIT if pool for the file already exixts
*         RC_FILE_NOT_FOUND when open file page is not successful
*
*/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy,void *stratData)
{

	BManager *bp_mgmt = (BManager*)malloc(sizeof(BManager));
	bp_mgmt->start = NULL;
	SM_FileHandle fHandle;
	int i = 0;
	openPageFile((char*) pageFileName,&fHandle);
	while(i<numPages)
	{
		createPageFrame(bp_mgmt);
		i++;
	}
	bp_mgmt->tail = bp_mgmt->head;
	bp_mgmt->stratData = stratData;
	bp_mgmt->occupiedCount = 0;
	bp_mgmt->numRead = 0;
	bp_mgmt->numWrite = 0;
	bm->numPages = numPages;
	bm->pageFile = (char*) pageFileName;
	bm->strategy = strategy;
	bm->mgmtData = bp_mgmt;
	closePageFile(&fHandle);
	return RC_OK;
}

/*
* Function: shutdownBufferPool
* ---------------------------
* This function destroys the Buffer pool
*
* bm: Structure which stores information about the buffer pool
*
* return: RC_OK if the bufferpool destroy is successful
*         RC_BUFFER_POOL_NOT_INIT if the bufferpool doesnt exists.
*
*/

RC shutdownBufferPool(BM_BufferPool *const bm)
{
	BManager *bp_mgmt = bm->mgmtData;
	PageFrame *pgeframe = bp_mgmt->head;
	forceFlushPool(bm);

	pgeframe= pgeframe->next;
	while(pgeframe!=bp_mgmt->head)
	{
		free(pgeframe->data);
		pgeframe= pgeframe->next;
	}

	free(pgeframe);
	bp_mgmt->start = NULL;
	bp_mgmt->head = NULL;
	bp_mgmt->tail = NULL;
	return RC_OK;
}

/*
* Function: forceFlushPool
* ---------------------------
* This function writes all the pages marked as dirty to the disc
*
* bm: Structure which stores information about the buffer pool
*
* return: RC_OK if the bufferpool destroy is successful
*         // Based on writeBlock return values from storage_mgr.c
*         RC_FILE_HANDLE_NOT_INIT if the file handle is not defined for the file
* 		    RC_FILE_NOT_FOUND if the file pointer in the fhandle points to null indicating there is no such file
*	        RC_WRITE_FAILED if there is no space in the file
*
*/

RC forceFlushPool(BM_BufferPool *const bm)
{
	BManager *bp_mgmt = bm->mgmtData;
	PageFrame *pgeframe = bp_mgmt->head;
	SM_FileHandle fh;
	RC openpageFlag = openPageFile((char *)(bm->pageFile),&fh);

	if (openpageFlag != RC_OK)
	{
		return openpageFlag;
	}
	do
	{
		if(pgeframe->fixCount == 0 && pgeframe->dirtyFlag != 0)
		{
			RC writeFlag = writeBlock(pgeframe->pageNum, &fh, pgeframe->data);
			if(writeFlag != RC_OK)
			{
				closePageFile(&fh);
				return writeFlag;
			}
			else{
				pgeframe->dirtyFlag = 0;
				bp_mgmt->numWrite++;
			}
		}
		pgeframe = pgeframe->next;
	}while(pgeframe != bp_mgmt->head);
	closePageFile(&fh);
	return RC_OK;
}

/* Function: markDirty
 * ----------------------------------
 *  This function marks the page as dirty if the page is already found in the buffer pool, i.e. any operations are done on the page.
 *
 *  bm: Structure which stores information about the buffer pool.
 *  page: Structure which stored information about buffer page handle.
 *
 *  return: RC_OK if makeDirty function is executed successfully.
 *
*/

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BManager *bp_mgmt = bm->mgmtData;
	PageFrame *pgeframe = bp_mgmt->head;

	do
	{
		//if the pageNum of page handle is same as the page in buffer frame
		if(page->pageNum != pgeframe->pageNum)
		{
			pgeframe=pgeframe->next;
		}
		else{
			pgeframe->dirtyFlag = 1;
			return RC_OK;
		}

	}while(pgeframe!=bp_mgmt->head);

	return RC_OK;
}

/* Function: unpinPage
 * ----------------------------------
 * This function is used to unpinpage after reading is completed and page is not in use.
 *
 *  bm: Structure which stores information about the buffer pool.
 *  page: Structure which stored information about buffer page handle.
 *
 *  return: RC_OK if unpinPage function is executed successfully.
 *
*/

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BManager *mgmt = bm->mgmtData;
	PageFrame *pgeFrame = mgmt->head;
	if(page->pageNum == mgmt->head->pageNum)
	{
		mgmt->head->fixCount--;
		return RC_OK;
	}
	pgeFrame = pgeFrame->next;
	while(pgeFrame!= mgmt->head)
	{
		if(page->pageNum != pgeFrame->pageNum)
		{
			pgeFrame = pgeFrame->next;
		}
		else{
			pgeFrame->fixCount--;
			return RC_OK;
		}

	}

	return RC_OK;
}

/* Function: forcePage
 * ----------------------------------
 * This function is used to write the current content of the page back to the page file on disk.
 *
 *  bm: Structure which stores information about the buffer pool.
 *  page: Structure which stored information about buffer page handle.
 *
 *  return: RC_OK if forcePage function is executed successfully.
 *          RC_FILE_NOT_FOUND if file not found.
 *
*/

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	BManager *bp_mgmt = bm->mgmtData;
	PageFrame *Frame = bp_mgmt->head;
	SM_FileHandle fh;
	RC openpageFlag = openPageFile ((char *)(bm->pageFile), &fh);
	//Open PageFile for write operation
	if (openpageFlag != RC_OK)
	{
		return RC_FILE_NOT_FOUND;
	}
	do
	{
		if(Frame->dirtyFlag == 1 && Frame->pageNum == page->pageNum )
		{
			if(writeBlock(Frame->pageNum, &fh, Frame->data) != RC_OK)
			{
				closePageFile(&fh);
				return RC_WRITE_FAILED;
			}
			bp_mgmt->numWrite++;
			Frame->dirtyFlag = 0;
			break;
		}
		Frame= Frame->next;
	}while(Frame!=bp_mgmt->head);

	closePageFile(&fh);
	return RC_OK;
}
/*
 * Function: pagePresent
 * ---------------------------
 * This function returns RC_OK if the page is already present in the buffer.
 *
 * mgmt: Structure which stores information about the buffer Manager.
 * page: Structure which stored information about buffer page handle.
 * pageNum: This is a field in buffer page handle which stored the page number.
 *
 * return: RC_OK if the page pinning to the buffer pool is successful.
 *				 RC_IM_KEY_NOT_FOUND if page is not found in buffer.
 *
 *
 */

RC pagePresent(BM_PageHandle *const page, BManager *mgmt, const PageNumber pageNum, char *algo){
	// if page is already present in the buffer pool
	PageFrame *pgeframe = mgmt->head;
	int flag = 0;
	do
	{
		//put the data onto the page and increment the fix count
		if(pgeframe->pageNum == pageNum)
		{
			page->pageNum = pageNum;
			page->data = pgeframe->data;

			pgeframe->pageNum = pageNum;
			pgeframe->fixCount++;
			if (algo == "LRU"){
				//point the head and tail for replacement
				mgmt->tail = mgmt->head->next;
				mgmt->head = pgeframe;
			}
			flag = 1;
			return RC_OK;
		}
		pgeframe = pgeframe->next;
	}while(pgeframe!= mgmt->head);
	if(flag == 0){
		return RC_IM_KEY_NOT_FOUND;
	}
}

/*
 * Function: isEmptyBP
 * ---------------------------
 * This function checks if any slot in buffer pool is empty and assigns the frames to the buffer pool
 *
 * bm: Structure which stores information about the buffer pool.
 * pageNum: This is a field in buffer page handle which stored the page number.
 *
 * return: RC_OK if the page pinning to the buffer pool is successful.
 *				 RC_IM_NO_MORE_ENTRIES if buffer is full.
 *
 *
 */

void isEmptyBP(BM_BufferPool *const bm, const PageNumber pageNum )
{
	BManager *mgmt = bm->mgmtData;
	PageFrame *pgeframe = mgmt->head;

	if(mgmt->occupiedCount < bm->numPages)
	{
		pgeframe = mgmt->head;
		pgeframe->pageNum = pageNum;
		if(pgeframe->next != mgmt->head)
		{
			mgmt->head = pgeframe->next;
		}
		pgeframe->fixCount++;
		mgmt->occupiedCount++;
	}
}

/*
 * Function: pinPage
 * ---------------------------
 * This function used any of the page replacement strategies and places the page on to the Buffer pool
 *
 * bm: Structure which stores information about the buffer pool.
 * page: Structure which stored information about buffer page handle.
 * pageNum: This is a field in buffer page handle which stored the page number.
 *
 * return: RC_OK if the page pinning to the buffer pool is successful
 					RC_BUFFER_POOL_ALREADY_INIT if bufferpool is not initialized.
 *
 *
 */

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum)
{
	SM_FileHandle fh;
	BManager *mgmt = bm->mgmtData;
	RC pageExists;
	openPageFile((char*) bm->pageFile,&fh);

	switch(bm->strategy)
	{
	case RS_FIFO:
			pageExists = pagePresent(page, mgmt,pageNum,"FIFO");
			if(pageExists == RC_OK){
				return RC_OK;
			}
			else{
			pinWithFIFO(bm, page, pageNum,mgmt,fh);
			}
			break;

	case RS_LRU:
				pageExists = pagePresent(page, mgmt,pageNum,"LRU");
				if(pageExists == RC_OK){
					return RC_OK;
				}
				else{
				pinWithLRU(bm,page,pageNum,mgmt,fh);
				}
				break;
	}
	return RC_OK;
}

/*
 * Function: pinWithFIFO
 * ---------------------------
 * This function implements First In First Out Algorithm for page replacement
 *
 * bm: Structure which stores information about the buffer pool.
 * page: Structure which stored information about buffer page handle.
 * pageNum: This is a field in buffer page handle which stored the page number.
 * mgmt: Structure which stores information about the buffer manager.
 * frame: Structure which stores information about the buffer frame.
 * fh: Structure which holds Filehandler.
 *
 * return: RC_OK if the page pinning to the buffer pool is successful.
 *				 RC_IM_KEY_NOT_FOUND if page is not found in buffer.
 *				 readBlock or writeBlock errors if the operations fail.
 *
 */

 RC pinWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum, BManager *mgmt,SM_FileHandle fh )
 {
    PageFrame *frame = mgmt->head;
		//Filling the empty frames in the  bufferpool
		if(mgmt->occupiedCount < bm->numPages)
		{
			isEmptyBP(bm,pageNum);
			printf("Inserting remaining frames in empty spaces in buffer pool with FIFO");
		}
		else
		{
			frame = mgmt->tail;
			do
			{
				// check If the page is in use?
				if(frame->fixCount == 0)
				{
					//If dirty, write the page back to the disc from disc frame.
					if(frame->dirtyFlag != 0)
					{
						ensureCapacity(frame->pageNum, &fh);
						RC writeFlag = writeBlock(frame->pageNum,&fh, frame->data);
						if(writeFlag!=RC_OK)
						{
							closePageFile(&fh);
							return writeFlag;
						}
						else{
							mgmt->numWrite++;
						}

					}
					frame->pageNum = pageNum;
					frame->fixCount++;
					mgmt->tail = frame->next;
					mgmt->head = frame;

					break;
				}
				else
				{
					frame = frame->next;
				}
			}while(frame!= mgmt->head);
		}

		//Add pages in pagefile if not sufficient
		ensureCapacity((pageNum+1),&fh);
		RC readFlag = readBlock(pageNum, &fh,frame->data);
		if(readFlag!=RC_OK)
		{
			closePageFile(&fh);
			return readFlag;
		}
		else{
			mgmt->numRead++;
		}

		page->pageNum = pageNum;
		page->data = frame->data;

		closePageFile(&fh);

		return RC_OK;
 }

 /*
  * Function: pinWithLRU
  * ---------------------------
  * This function implements Least Recently Used Algorithm for page replacement
  *
  * bm: Structure which stores information about the buffer pool.
  * page: Structure which stores information about buffer page handle.
  * pageNum: This is a field in buffer page handle which stored the page number.
  * mgmt: Structure which stores information about the buffer manager.
  * frame: Structure which stores information about the buffer frame.
  * fh: Structure which holds Filehandler.
  *
  * return: RC_OK if the page pinning to the buffer pool is successful.
  *				 RC_IM_KEY_NOT_FOUND if page is not found in buffer.
  *				 readBlock or writeBlock errors if the operations fail.
  *
  *
  */

 RC pinWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum,BManager *mgmt, SM_FileHandle fh)
 {
	 PageFrame *frame = mgmt->head;

  if(mgmt->occupiedCount < bm->numPages)
  {
 	 isEmptyBP(bm,pageNum);
 	 printf("Inserting remaining frames in empty spaces in buffer pool with LRU");
  }
  else
  {
 	 frame = mgmt->tail;
 	 do
 	 {
 		 if(frame->fixCount == 0)
 		 {
 			 //If dirty then write back to disc using writeBlock function
 			 if(frame->dirtyFlag != 0)
 			 {
 				 ensureCapacity(frame->pageNum, &fh);
 				 RC writeFlag = writeBlock(frame->pageNum,&fh, frame->data);
 				 if(writeFlag!=RC_OK)
 				 {
 					 closePageFile(&fh);
 					 return writeFlag;
 				 }
 				 else{
 					 mgmt->numWrite++;
 				 }

 			 }

 			 //Replacing the page which is least recently used.
 			 if(mgmt->tail == mgmt->head)
 			 {
 				 frame = frame->next;
 				 frame->pageNum = pageNum;
 				 frame->fixCount++;
 				 mgmt->tail = frame;
 				 mgmt->head = frame;
 				 mgmt->tail = frame->prev;
 				 break;
 			 }
 			 else
 			 {
 				 frame->pageNum = pageNum;
 				 frame->fixCount++;
 				 //mgmt->tail = frame->next;
 				 mgmt->tail = frame;
 				 mgmt->tail = frame->next;
 				 break;
 			 }
 		 }
 		 else
 		 {
 			 frame = frame->next;
 		 }
 	 }while(frame!= mgmt->tail);
  }
  //Add pages if not sufficient
  ensureCapacity((pageNum+1),&fh);
  RC readFlag = readBlock(pageNum, &fh,frame->data);
  if(readFlag!=RC_OK)
  {
 	 return readFlag;
  }
  else{
 	 mgmt->numRead++;
  }

  page->pageNum = pageNum;
  page->data = frame->data;


  closePageFile(&fh);

  return RC_OK;
 }




// Statistics Interface
/*
 * The getFrameContents function returns an array of PageNumbers (of size numPages)
 * where the ith element is the number of the page stored in the ith page frame.
 * An empty page frame is represented using the constant NO_PAGE.
 */
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
	BManager *bp_mgmt;
	bp_mgmt = bm->mgmtData;
	bp_mgmt->frameContent = (PageNumber*)malloc(sizeof(PageNumber)*bm->numPages);

	PageFrame *frame = bp_mgmt->start;
	PageNumber* frameContents = bp_mgmt->frameContent;

	int i;
	int page_count = bm->numPages;

	if(frameContents != NULL)
	{
		for(i=0;i< page_count;i++)
		{
			frameContents[i] = frame->pageNum;
			frame = frame->next;
		}
	}
	//free(bp_mgmt->frameContent);

	return frameContents;
}

/*
 * The getDirtyFlags function returns an array of bools (of size numPages)
 * where the ith element is TRUE if the page stored in the ith page frame is dirty.
 * Empty page frames are considered as clean.
 */
bool *getDirtyFlags (BM_BufferPool *const bm)
{
	BManager *bp_mgmt;
	bp_mgmt = bm->mgmtData;
	bp_mgmt->dirtyBit = (bool*)malloc(sizeof(bool)*bm->numPages);

	PageFrame *frame = bp_mgmt->start;
	bool* dirtyBit = bp_mgmt->dirtyBit;

	int i,page_count = bm->numPages;

	if(dirtyBit != NULL)
	{
		for(i=0;i< page_count;i++)
		{
			dirtyBit[i] = frame->dirtyFlag;
			frame = frame->next;
		}
	}
	//free(bp_mgmt->dirtyBit);

	return dirtyBit;
}

/*
 * The getFixCounts function returns an array of ints (of size numPages)
 * where the ith element is the fix count of the page stored in the ith page frame.
 * Return 0 for empty page frames.
 */
int *getFixCounts (BM_BufferPool *const bm)
{
	BManager *bp_mgmt;
	bp_mgmt = bm->mgmtData;
	bp_mgmt->fixCount = (int*)malloc(sizeof(int)*bm->numPages);

	PageFrame *frame = bp_mgmt->start;
	int* fixCount = bp_mgmt->fixCount;

	int i,page_count = bm->numPages;

	if(fixCount != NULL)
	{
		for(i=0;i< page_count;i++)
		{
			fixCount[i] = frame->fixCount;
			frame = frame->next;
		}
	}
	//free(bp_mgmt->fixCount);

	return  fixCount;
}

/*
 * This function gets the total number of ReadBlock operations performed
 */
int getNumReadIO (BM_BufferPool *const bm)
{
	return ((BManager*)bm->mgmtData)->numRead;
}

/*
 * This function gets the total number of writeBlock operations performed
 */
int getNumWriteIO (BM_BufferPool *const bm)
{
	return ((BManager*)bm->mgmtData)->numWrite;
}
