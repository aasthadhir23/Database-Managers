
Group 25 - Assignment 4 - B Tree

Team Members: - Group 25

-----------------------------------------------------------
Susmitha Marripalapu

Swetha Radhakrishnan





Description

---------------------------------------------------------


To index pages according to their Key values, we use B+ Trees. The keys are inserted at their respective nodes.
If the page exists, it can be accessed by following a pointer to that node.


How to run

-----------------------------------------------------------


1. Open terminal and Clone from BitBucket to the required location.

2. Use make command to execute B Tree,

	$ make all
4. Use make command to execute test_expr,

	$ make expr
5. To clean,
	$ make clean





Solution Description

-----------------------------------------------------------

* Function: initIndexManager
* ---------------------------
* This is function is used to Initialize Index Manager.
*
*
* returns : RC_OK if index manager initializing is successful.

* Function: shutdownIndexManager
* ---------------------------
* This is function is used to shut down the index manager,
* and free allocated memory to bTreeCreate during the process
*
*
* returns : RC_OK if index manager initializing is successful.

* Function: createBtree
* ---------------------------
* This function is used to Create a B+ Tree
* and initialize attributes related to the tree.
*
* idxId: Index identifier of a BTree handle.
* keyType: DataType of the key.
* n: size of page handle.
*
* returns : RC_OK if index manager initializing is successful.

* Function: openBtree
* ---------------------------
* This function is used to open the B-Tree alread created above,
* it read the value from the page file regarding the "N"
* and stores in the BTREE structure created attributes
*
* tree: Management structure for BTreeHandle.
* idxId: Index identifier of a BTree handle.
*
* returns : RC_OK if open Btree is successful.
*					RC_FILE_NOT_FOUND if openPagefile fails.

* Function: closeBtree
* ---------------------------
* This function is used to close btree.
*
* tree: Management structure for BTree Handle.
*
* returns : RC_OK if freeing treehandle is successful.

* Function: deleteBtree
* ---------------------------
* This function is used to destroy the page file created and free all memory created for btree.
*
* idxId: Index identifier of a BTree handle.
*
* returns : RC_OK if destroyPageFile is successful.

* Function: getNumNodes
* ---------------------------
* This function is used to get the number of nodes in b+tree.
* This is caliculated by taking total number of keys inserted
* MINUS same nodes will give us the final result
*
* tree: Management structure for BTree Handle.
* result: Number of keys
*
* returns : RC_OK if calculating number of nodes is successful.

* Function: getNumEntries
* ---------------------------
* This function is used to return total number of keys in the B+Tree.
*
* tree: Management structure for BTree Handle.
* result: Number of keys
*
* returns : RC_OK if calculating number of keys is successful.

* Function: getKeyType
* ---------------------------
* This function is used get the type of Key in the tree which is inserted and stores it in the result.
*
* tree: Management structure for BTree Handle.
* result: Number of keys
*
* returns : RC_OK if getting keytype is successful.

* Function: findKey
* ---------------------------
* This function is used in searching for a key in the B+tree
*
* tree: Management structure for BTree Handle.
* result: Management structure for RecordID.
* key: Information about the key.
*
* returns : RC_OK if calculating number of nodes is successful.
* 	    RC_IM_KEY_NOT_FOUND if key is not found
*	    RC_RM_NO_PRINT_FOR_DATATYPE if datatypedoesnot match.

 * Function: insertKey
 * ---------------------------
 * This function is used to insert Keys into the B+ Tree. 
 *
 * tree: Management structure for BTree Handle.
 * key: Information about the key.
 *
 * returns : RC_OK if inserting keys is successful.
 * 					RC_IM_KEY_NOT_FOUND if key is not found
 *					RC_RM_NO_PRINT_FOR_DATATYPE if datatype does not match.
 
 * Function: deleteKey
 * ---------------------------
 * This function is used to delete B+ tree Keys. 
 *
 * tree: Management structure for BTree Handle.
 * key: Information about the key.
 *
 * returns : RC_OK if deleting the key is successful.
 * 					RC_IM_KEY_NOT_FOUND if key is not found
 *					RC_RM_NO_PRINT_FOR_DATATYPE if datatype does not match.

 * Function: OpenTreeScan
 * ---------------------------
 * This function is used to create a scan ready tree. 
 *
 * tree: Management structure for BTree Handle.
 * key: Information about the key.
 * handle: Scan Handle for performing scan.
 *
 * returns : RC_OK if creation of tree is successful.
 * 					RC_IM_KEY_NOT_FOUND if key is not found
 *					RC_RM_NO_PRINT_FOR_DATATYPE if datatype does not match.

 * Function: nextEntry
 * ---------------------------
 * This function is used to add the entries to the B+ tree.  
 *
 * handle: Scan Handle for performing scan.
 *
 * returns : RC_OK if the entry is successful.
 * 					RC_IM_NO_MORE_ENTRIES if there are no more entries to add

 * Function: closeTreeScan
 * ---------------------------
 * This function is used to close the scanned tree. 
 *
 * handle: Scan Handle for operations on scan.
 *
 * returns : RC_OK if closing the tree is successful.