#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);
static void testMultiplePageContent(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();

  testCreateOpenClose();
  testSinglePageContent();
  testMultiplePageContent();

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));
  
  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");
  
  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");
    
  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // destroy new page file
  TEST_CHECK(destroyPageFile (TESTPF));  
  
  TEST_DONE();
}



void
testMultiplePageContent(void)
{
    SM_FileHandle fh;
    SM_PageHandle ph;
    int i;

    testName = "test multiple page content";

    ph = (SM_PageHandle)malloc(PAGE_SIZE);

    // Create a new page file
    TEST_CHECK(createPageFile(TESTPF));

    // Open newly created page file
    TEST_CHECK(openPageFile(TESTPF, &fh));
    printf("created and opened file\n");

    // Write cyclic sequenece of numbers to 1st block (page=0) of page file.
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = (i % 10) + '0';
    TEST_CHECK(writeBlock(0, &fh, ph));
    printf("writing first block\n");

    // Verify current block position (page=1).
    ASSERT_TRUE(getBlockPos(&fh) == 1, "Currently at page 1");

    // Try and fail to fill 2nd block (page=1) with letter 'A'.
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = 'A';
    ASSERT_TRUE(writeBlock(1, &fh, ph) == RC_WRITE_FAILED, "Page 1 doesn't exist");

    // Append an empty block to page file (page=1).
    TEST_CHECK(appendEmptyBlock(&fh));

    // Ensure total 2 blocks present in file (total pages=2)
    TEST_CHECK(ensureCapacity(2, &fh));

    // Fill 2nd block (page=1) with letter 'A'.
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = 'A';
    TEST_CHECK(writeBlock(1, &fh, ph));

    // Try and fail reading current block i.e. 3rd block (page=2).
    ASSERT_TRUE(readCurrentBlock(&fh, ph) == RC_READ_NON_EXISTING_PAGE, "Page 2 doesn't exist");

    // Try and fail writing current block i.e 3rd block (page=2).
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = 'Z';
    ASSERT_TRUE(writeCurrentBlock(&fh, ph) == RC_WRITE_FAILED, "Page 2 doesn't exist");

    // Read last block i.e 2nd block (page=1) filled with letter 'A'.
    TEST_CHECK(readLastBlock(&fh, ph));
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 'A'), "character in page read from disk is the one we expected.");
    printf("reading last block\n");

    // Read 1st block of file (page=0) filled with sequence of numbers.
    TEST_CHECK(readFirstBlock(&fh, ph));
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    printf("reading first block\n");
    
    // Fill 2nd block (page=1) with letter 'Z'.
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = 'Z';
    TEST_CHECK(writeCurrentBlock(&fh, ph));

    // Read 2nd block (page=1) with letter 'Z'.
    TEST_CHECK(readPreviousBlock(&fh, ph));
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == 'Z'), "character in page read from disk is the one we expected.");
    printf("reading second block\n");

    // Append an empty block to page file (page=2).
    TEST_CHECK(appendEmptyBlock(&fh));

    // Ensure total 3 blocks present in file (total pages=3)
    TEST_CHECK(ensureCapacity(3, &fh));

    // Fill 3rd block (page=2) with letter '-'.
    for (i = 0; i < PAGE_SIZE; i++)
        ph[i] = '-';
    TEST_CHECK(writeBlock(2, &fh, ph));

    // Read 1st block of file (page=0) filled with sequence of numbers.
    TEST_CHECK(readBlock(0, &fh, ph));
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
    printf("reading first block\n");

    // Verify current block position (page=1).
    ASSERT_TRUE(getBlockPos(&fh) == 1, "Currently at page 1");

    // Read 3rd block (page=2) with letter '-'.
    TEST_CHECK(readNextBlock(&fh, ph));
    for (i = 0; i < PAGE_SIZE; i++)
        ASSERT_TRUE((ph[i] == '-'), "character in page read from disk is the one we expected.");
    printf("reading third block\n");

    // Try and fail reading current block i.e. 4th block (page=3).
    ASSERT_TRUE(readNextBlock(&fh, ph) == RC_READ_NON_EXISTING_PAGE, "Page 3 doesn't exist");

    // Verify current block position (page=3).
    ASSERT_TRUE(getBlockPos(&fh) == 3, "Currently at page 3");

    // Close & destroy new page file
    TEST_CHECK(closePageFile(&fh));
    TEST_CHECK(destroyPageFile(TESTPF));

    TEST_DONE();
}

                                                                                                                               
