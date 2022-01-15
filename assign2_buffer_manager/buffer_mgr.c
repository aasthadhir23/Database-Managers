#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "buffer_mgr.h"
#include "storage_mgr.h"

/*Structure for Buffer Frame inside BufferPool
	This contains pointers to the next and previous frams inside the buffer to form nodes of doubly linked list.
	Page frame infoe=rmation like pagenumber, data in page and dirty flag are maintained*. */

typedef struct BufferFrame
{
	struct BufferFrame *next;
	struct BufferFrame *prev;
	int pageNum;
	int dirty;
	int fixCount;
	char *data;
}BufferFrame;

/*Structure for Buffer Pool Manager
	This contains datastructures to navigate through bufferpool using linkedlists.
	Some variables to get the statistics are also declared in this structure.*/

typedef struct BManager
{
	SM_FileHandle *smf;
	BufferFrame *head,*start,*tail;
	PageNumber *frameContent;
	int count;
	void *stratData;
	int *fixCount;
	bool *dirtyBit;
	int numReadIO;
	int numWriteIO;
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

void createFrame(BManager *mgmt)
{
	BufferFrame *pgeframe = (BufferFrame *) malloc(sizeof(BufferFrame));
  pgeframe->data = calloc(PAGE_SIZE,sizeof(char*));
	pgeframe->dirty = 0;	//FALSE
	pgeframe->fixCount = 0;
	pgeframe->pageNum = -1;

	//initialise the pointers
	mgmt->head = mgmt->start;


	if(mgmt->head != NULL)
	{
		mgmt->tail->next = pgeframe;
		pgeframe->prev = mgmt->tail;
		mgmt->tail = mgmt->tail->next;
	}
	else
	{
		mgmt->head = pgeframe;
		mgmt->tail = pgeframe;
		mgmt->start = pgeframe;
	}

	mgmt->tail->next = mgmt->head;
	mgmt->head->prev = mgmt->tail;

}

void freePool(BM_BufferPool* bm,BManager* md){
	//free the bufferPool
	free(md);
	//make the values NULL
	md->start = NULL;
	md->head = NULL;
	md->tail = NULL;

	//set all the values to 0 or NULL
	bm->numPages = 0;
	bm->mgmtData = NULL;
	bm->pageFile = NULL;
}

void initBPFrame(BM_BufferPool *const bm){
	BManager *mgmt = bm->mgmtData;
	BufferFrame *pgeframe = mgmt->head;
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
	//Check if the pool for the file already exists
	if (bm->mgmtData != NULL){
	  return RC_BUFFER_POOL_ALREADY_INIT;
	}
	BManager *mgmt = (BManager*)malloc(sizeof(BManager));
  mgmt->smf = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));

	int i = 1;
	RC openpageFlage = openPageFile((char*) pageFileName,mgmt->smf);

	if (openpageFlage != RC_OK){
		return openpageFlage;
	}
	else{
		while(i<=numPages)
		{
			createFrame(mgmt);
			i=i+1;
		}

		mgmt->tail = mgmt->head;
		mgmt->stratData = stratData;
		mgmt->count = 0;
		mgmt->numReadIO = 0;
		mgmt->numWriteIO = 0;
		bm->numPages = numPages;
		bm->pageFile = pageFileName;
		bm->strategy = strategy;
		bm->mgmtData = mgmt;

		closePageFile(mgmt->smf);

		return RC_OK;
	}
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
	if (bm->mgmtData == NULL){
	  return RC_BUFFER_POOL_NOT_INIT;
	}
	//load the mgmt data of the buffer pool
	BManager *mgmt = bm->mgmtData;
	BufferFrame *pgeframe = mgmt->head;

	//To write all dirty pages back to disc again, before destroying
	forceFlushPool(bm);

	pgeframe= pgeframe->next;
	//free all the page data in the frame
	while(pgeframe!=mgmt->head)
	{
		free(pgeframe->data);
		pgeframe= pgeframe->next;
	}

	free(pgeframe);
	freePool(bm,mgmt);
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
	BManager *mgmt = bm->mgmtData;
  BufferFrame *pgeframe = mgmt->head;

	SM_FileHandle* fh = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));;
	mgmt->smf = fh;
	RC openpageFlag = openPageFile((char *)(bm->pageFile),mgmt->smf);


	if (openpageFlag != RC_OK)
	{
		return openpageFlag;
	}

	do
	{
		if(pgeframe->fixCount == 0 && pgeframe->dirty != 0)
		{
			RC writeFlag = writeBlock(pgeframe->pageNum, mgmt->smf, pgeframe->data);
			if(writeFlag != RC_OK)
			{
				closePageFile(mgmt->smf);
				return writeFlag;
			}
			else{
				pgeframe->dirty = 0;
				mgmt->numWriteIO++;
			}
		}
		pgeframe = pgeframe->next;
	}while(pgeframe != mgmt->head);

	//close the page file
	closePageFile(mgmt->smf);
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
	//check if the buffer pool i not initialized
	if(bm->mgmtData == NULL)
		return RC_BUFFER_POOL_NOT_INIT;
	BManager *mgmt = bm->mgmtData;
  BufferFrame *pgeframe = mgmt->head;
	//Iterate through all the frames in buffer pool
	do
	{
		//if the pageNum of page handle is same as the page in buffer frame
		if(page->pageNum != pgeframe->pageNum)
		{
			pgeframe=pgeframe->next;
		}
		else{
			pgeframe->dirty = 1;
			return RC_OK;
		}

	}while(pgeframe!=mgmt->head);

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
	BufferFrame *pgeFrame = mgmt->head;
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
	BManager *mgmt = bm->mgmtData;

	BufferFrame *Frame = mgmt->head;
	SM_FileHandle* fh = (SM_FileHandle *)malloc(sizeof(SM_FileHandle));;
	mgmt->smf = fh;

	RC openpageFlag = openPageFile ((char *)(bm->pageFile), mgmt->smf);
	//Open PageFile for write operation
	if (openpageFlag != RC_OK)
	{
		return RC_FILE_NOT_FOUND;
	}

	//find the page that needs to be written back & check if its dirty flag is 1
	do
	{
		if(Frame->pageNum == page->pageNum && Frame->dirty == 1)
		{
			if(writeBlock(Frame->pageNum, mgmt->smf, Frame->data) != RC_OK)
			{
				closePageFile(mgmt->smf);
				return RC_WRITE_FAILED;
			}
			mgmt->numWriteIO++;	//increment the num of writes performed
			Frame->dirty = 0;	//unmark its dirty bit
			break;
		}
		Frame= Frame->next;
	}while(Frame!=mgmt->head);

	closePageFile(mgmt->smf);
	return RC_OK;
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
	//Check if the pool for the file exists
	if (bm->mgmtData == NULL){
	  return RC_BUFFER_POOL_ALREADY_INIT;
	}
	SM_FileHandle fh;
	BManager *mgmt = bm->mgmtData;
	BufferFrame *frame = mgmt->head;
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
		pinWithFIFO(bm, page, pageNum,mgmt,frame,fh);
		}
		break;

	case RS_LRU:
		pageExists = pagePresent(page, mgmt,pageNum,"LRU");
		if(pageExists == RC_OK){
			return RC_OK;
		}
		else{
		pinWithLRU(bm,page,pageNum,mgmt,frame,fh);
		}
		break;

	}
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
	BufferFrame *pgeframe = mgmt->head;
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
	BufferFrame *pgeframe = mgmt->head;

	if(mgmt->count < bm->numPages)
	{
		pgeframe = mgmt->head;
		pgeframe->pageNum = pageNum;
		if(pgeframe->next != mgmt->head)
		{
			mgmt->head = pgeframe->next;
		}
		pgeframe->fixCount++;
		mgmt->count++;
	}
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

 RC pinWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum, BManager *mgmt,BufferFrame *frame,SM_FileHandle fh )
 {

		//Filling the empty frames in the  bufferpool
		if(mgmt->count < bm->numPages)
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
					if(frame->dirty != 0)
					{
						ensureCapacity(frame->pageNum, &fh);
						RC writeFlag = writeBlock(frame->pageNum,&fh, frame->data);
						if(writeFlag!=RC_OK)
						{
							closePageFile(&fh);
							return writeFlag;
						}
						else{
							mgmt->numWriteIO++;
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
			mgmt->numReadIO++;
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

RC pinWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum,BManager *mgmt, BufferFrame *frame, SM_FileHandle fh)
{

	//Filling empty spaces in the bufferPool
	if(mgmt->count < bm->numPages)
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
				if(frame->dirty != 0)
				{
					ensureCapacity(frame->pageNum, &fh);
					RC writeFlag = writeBlock(frame->pageNum,&fh, frame->data);
					if(writeFlag!=RC_OK)
					{
						closePageFile(&fh);
						return writeFlag;
					}
					else{
						mgmt->numWriteIO++;
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
		mgmt->numReadIO++;
	}

	page->pageNum = pageNum;
	page->data = frame->data;


	closePageFile(&fh);

	return RC_OK;
}




// Statistics Interface

/*
* Function: getFrameContents
* ---------------------------
* This function returns an array of page numbers where the ith element is page stored in the ith page frame. 
*
* *bm: stores information about the buffer pool
* mgmt: Structure which stores information about the buffer manager.
* frame: Structure which stores information about the buffer frame.
* BM_BufferPool: structure that holds the buffer pool with pagefile and page frames. 	
*
* return: value of the contents of frame is returned
*        
*/

PageNumber *getFrameContents (BM_BufferPool *const bm)
{ 
	int i, page_count;
	i=0;
	page_count = bm->numPages;
	
	BufferFrame *frame = ((BManager *)bm->mgmtData)->start;	
	PageNumber* fContents = (PageNumber*)malloc(sizeof(PageNumber)*bm->numPages);

	if(fContents != NULL)
	{
		while(i<page_count)
		{
			fContents[i] = frame->pageNum;
			i++;
			frame = frame->next;
		}
	}
	return fContents;
}


/*
* Function: getDirtyFlags
* ---------------------------
* This function returns an array of boolean values where the ith element is marked TRUE if the page frame is dirty (modified). 
*
* *bm: stores information about the buffer pool
* mgmt: Structure which stores information about the buffer manager.
* frame: Structure which stores information about the buffer frame.
* BM_BufferPool: structure that holds the buffer pool with pagefile and page frames. 	
* dirtyBit: bit that contains the boolean value that specifies if a page is dirty or not. 
*
* return: returns an array of the dirtyFlag details of size numPages
*        
*/

bool *getDirtyFlags (BM_BufferPool *const bm)
{
		
	int i,page_count;
	i=0;
	page_count = bm->numPages;

	BufferFrame *frame = ((BManager *)bm->mgmtData)->start;
	
	bool* dirtyFlag = (bool*)malloc(sizeof(bool)*page_count);
	
	//Empty pages are clean
	if(dirtyFlag != NULL) 
	{
		while(i<page_count)
		{
			dirtyFlag[i] = frame->dirty;
			i++;
			frame = frame->next;
		}
	}
	return dirtyFlag;
}


/*
* Function: getFixCounts
* ---------------------------
* This function returns an array of integers that specify how many times a page is being accessed by different users. 
*
* *bm: stores information about the buffer pool
* mgmt: Structure which stores information about the buffer manager.
* frame: Structure which stores information about the buffer frame.
* BM_BufferPool: structure that holds the buffer pool with pagefile and page frames. 	
*
* return: returns an array of the integers with fixCount values of a page frame
*        
*/

int *getFixCounts (BM_BufferPool *const bm)
{
	
	if (bm->mgmtData == NULL)
	printf ("\nBuffer Pool not initialised"); 

	BufferFrame *frame = ((BManager *)bm->mgmtData)->start;
	
	int i, page_count;
	i=0;
	page_count = bm->numPages;
	
	int *fixCount = (int*) malloc (sizeof(int) * page_count);

	if(fixCount != NULL)
	{
		while(i<page_count)
		{
			fixCount[i] = frame->fixCount;
			i++;
			frame = frame->next;
		}
		
	}
	return  fixCount;
}

/*
* Function: getNumReadIO
* ---------------------------
* This function gets the number of the readBlock operations performed. 
*
* *bm: stores information about the buffer pool
* mgmt: Structure which stores information about the buffer manager.
* BM_BufferPool: structure that holds the buffer pool with pagefile and page frames. 	
* 
* return: the page that has been read
*        
*/
int getNumReadIO (BM_BufferPool *const bm)
{
	if(bm->mgmtData != NULL)
		return ((BManager *)bm->mgmtData)->numReadIO;
	else
		return 0;
}


/*
* Function: getNumWriteIO
* ---------------------------
* This function gets the total number of all the writeBlock operations performed. 
*
* *bm: stores information about the buffer pool
* mgmt: Structure which stores information about the buffer manager.
* BM_BufferPool: structure that holds the buffer pool with pagefile and page frames. 	
* 
* return: the page that has been written        
*/


int getNumWriteIO (BM_BufferPool *const bm)
{
	if(bm->mgmtData != NULL)
		return ((BManager *)bm->mgmtData)->numWriteIO;
	else
		return 0;
}
