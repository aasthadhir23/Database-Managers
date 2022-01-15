#assign2_buffer_manager

***********************************************************************

CONTRIBUTORS - 
1. Susmitha Marripalapu - smarripalapu@hawk.iit.edu
2. Swetha Radhakrishan - sradhakrishnan@hawk.iit.edu

***********************************************************************

Steps to Execute: 

1. Open terminal and Clone from Github to the required location.

2. Navigate to assign2_buffer_manager folder.

3. Use 'make' command to execute FIFO and LRU page replacement strategies,
 
	$ make
	
4. To clear all the output files,
	$ make clean

************************************************************************

The buffer manager is implemented in this assignment. It contains the following files:
	
Makefile
buffer_mgr.c
buffer_mgr.h
buffer_mgr.o
buffer_mgr_stat.c
buffer_mgr_stat.h
buffer_mgr_stat.o
dberror.c
dberror.h
dberror.o
dt.h
storage_mgr.c
storage_mgr.h
storage_mgr.o
test_assign2_1.c
test_assign2_1.o
test_assign2_2.c
test_helper.h

We have implemented the code to initialise, shut down and force flush the buffer pool, 
along with other functions to access the pages and maintain records of it. 

Implementation of each function is as given below:
	
-------------------------------------------------------------------------------------------
INTERFACE POOL HANDLING:
-------------------------------------------------------------------------------------------	

FUNCTION: RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		int numPages, ReplacementStrategy strategy,
		void *stratData);	
1. Create a new buffer pool and allocate sufficient size to it. 
2. Initialize the buffer pool objects - pageFile, numPages, strategy, mgmtdata and stratdata with the parameter values called from the function. 
3. Retrun RC_OK if the bufferpool creation is successful
		

FUNCTION: RC shutdownBufferPool(BM_BufferPool *const bm);
1. This function destroys the Buffer pool.
2. Load the mgmt data of the buffer pool.
3. Write all dirty pages back to disc again, before destroying.
4. Free all the page data in the frame. 

FUNCTION: RC forceFlushPool(BM_BufferPool *const bm);
1. This function writes all the pages marked as dirty to the disc
2. Check if the page flag is dirty or not
3. If dirty, write the page to the disk based on writeBlock return values from storage_mgr.c
4. Close the page file. 
 
-------------------------------------------------------------------------------------------
INTERFACE ACCESS PAGES:
-------------------------------------------------------------------------------------------
FUNCTION: RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
1. This function marks the page as dirty if the page is already found in the buffer pool, i.e. any operations are done on the page.
2. Check if the buffer pool i not initialized.
3. Iterate through all the frames in buffer pool.
4. If the pageNum of page handle is same as the page in buffer frame, then move to the next. 
5. Else mark it down as dirty

FUNCTION: RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
1. This function is used to unpinpage after reading is completed and page is not in use.
2. Check if a page is in use or not. 
3. If the page operations are performed, then copy the modifications to the disk. 
4. Free the page frame from the buffer pool if it it successfully updated. 
5. Return RC_OK if unpinPage function is executed successfully.


FUNCTION: RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
1. This function is used to write the current content of the page back to the page file on disk.
2. Open PageFile for write operation.
3. Find the page that needs to be written back to the disk after checking if its dirty. 
4. Write the file to the location. 
5. Increment the num of writes performed and unmark its dirty status. 


FUNCTION: RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
		const PageNumber pageNum)
1. This function uses the page replacement strategies to load a page frame to the pool. 
2. Check if the pool for the file exists. 
3. FIFO and LRU replacement strategies are used to load the page into the buffer pool  
4. Return RC_OK if its pinned successfully. 


-------------------------------------------------------------------------------------------
STATISTICS INTERFACE:
-------------------------------------------------------------------------------------------
FUNCTION: PageNumber *getFrameContents (BM_BufferPool *const bm)
1. This function forms an array of page numbers where the ith element is page stored in the ith page frame. 
2. An empty page is not loaded. 
3. For every page that is not null, the contents are loaded. 
4. The values of contents of a page are returned unless there is an error 


FUNCTION: bool *getDirtyFlags (BM_BufferPool *const bm)
1. This function generates an array of boolean values where the ith element is marked TRUE if the page frame is dirty (modified). 
2. A dirtyBit contains the boolean value that specifies if a page is dirty or not.
3. When a page is found to be dirty, the flag is updated. 
4. Empty page frames are always not dirty. 
5. This returns an array of the dirtyFlag details of size numPages


FUNCTION: int *getFixCounts (BM_BufferPool *const bm)
1. This function generates an array of integers that specify how many times a page is being accessed by different users. 
2. A frame is a structure which stores information about the buffer frame.
3. When a page is executed in the buffer pool, the fixcount is incremented by one. 
4. This returns an array of the integers with fixCount values of a page frame


FUNCTION: int getNumReadIO (BM_BufferPool *const bm)
1. Gets the total number of all the ReadBlock operations performed. 
2. It uses the stored information about the buffer pool to calculate the read counts. 
3. The page that is to be read is loaded for viewing.
4.  This returns the page that has been read. 

FUNCTION: int getNumWriteIO (BM_BufferPool *const bm)
1. Gets the total number of all the writeBlock operations performed. 
2. It uses the stored information about the buffer pool to calculate the write counts. 
3. The page that is to be written is loaded.
4.  This returns the page that has been written. 

-------------------------------------------------------------------------------------------
ADDITIONAL SUPPORTING FUNCTIONS:
-------------------------------------------------------------------------------------------
FUNCTION: void createFrame(BManager *mgmt)
1. Initialise the page frame for the buffer pool.
2. The head and tail of the nodes are initialised for the nodes. 
3. These information are stored in the buffer manager records. 

FUNCTION: RC pagePresent(BM_PageHandle *const page, BManager *mgmt, const PageNumber pageNum, char *algo)
1. Find if a page is present in the buffer pool already. 
2. If the page is not present, then the page replacement algorithms are called. 
3. If the page is present, then it is pinned to perform operations. 

FUNCTION: void isEmptyBP(BM_BufferPool *const bm, const PageNumber pageNum )
1. This function checks if any slot in buffer pool is empty and assigns the frames to the buffer pool
2. check if the buffer pool has empty page frames
3. If not empty, then page replacement strategies are called to free space. 

FUNCTION: RC pinWithFIFO(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum, BManager *mgmt,BufferFrame *frame,SM_FileHandle fh )
1. This implements the First In First Out Algorithm in Page replacement.
2. If empty frames are present in the pool, add the new page in the empty frame. 
3. Else, check if the first page is in use and dirty. 
4. If dirty, write the page to disk or else, remove the page. 
5. Use ensureCapacity from storage_mgr.c to make sure that the page added to the pool does not exceed the allocated size. 

FUNCTION: RC pinWithLRU(BM_BufferPool *const bm, BM_PageHandle *const page,const PageNumber pageNum,BManager *mgmt, BufferFrame *frame, SM_FileHandle fh)
1. This function implements Least Recently Used Algorithm for page replacement. 
2. If empty frames are present in the pool, add the new page in the empty frame. 
3. Else, check if the least recently used page is in use and dirty. 
4. If dirty, write the page to disk or else, remove the page. 
5. Use ensureCapacity from storage_mgr.c to make sure that the page added to the pool does not exceed the allocated size. 

