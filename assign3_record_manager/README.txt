Assignment 3 - Record Manager
Team Members: - Group 25
-----------------------------------------------------------

Susmitha Marripalapu
Swetha Radhakrishnan


Description
---------------------------------------------------------

The record manager is used to insert records, delete records, update records, and scan through the records in a table.



How to run
-----------------------------------------------------------

1. Open terminal and Clone from BitBucket to the required location.

2. Navigate to assign3_record_manager folder.

3. Use make command to execute record manager, test_assign3_1,
	$ make all
4. Use make command to execute test expr, test_expr,
	$ make expr
5. To clean,
	$ make clean



Solution 
-----------------------------------------------------------

RM_TableInfo - Management structure for maintaining TABLE metadata.

RM_RecordMgmt - Management structure for maintaining RECORD MANGER metadata.

RM_ScanMgmt - Management structure for maintaining RECORD SCAN MANGER metadata.

updatePageInfo - This method is used to update the page information by calling makeDirty, unpinPage and forcePage functions in buffer manager.

attributeOffset  - This method calculates the offset associated with each attribute by taking size of each attribute datatype.

Added error codes in dberror.h
-----------------------------------
RC_TABLE_ALREADY_EXISTS - 400
RC_RM_UPDATE_NOT_POSSIBLE_ON_DELETED_RECORD - 401
RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE - 402

Additional Functions
----------------------

BONUS IMPLEMENTATION: TOMBSTONE FOR DELETION OF RECORDS and DESERIALIZER FUNCTIONS 
-----------------------------------------------------------------------------------
$ A Tombstone flag called 'deleteFlag' has been implemented for every record deleted and this is attached as a prefix to the deleted record. 
Eg: 
	Record before Deletion: [8-0] (a:8,b:hhhh,c:3)
	Record after Deletion: DEL[8-0] (a:8,b:hhhh,c:3)

$ Two functions are included to deserialize the serialized schema and records into their required structures. 

Schema *deserializeSchema(char *serializedSchemaData)

 * This function is used to deserilaize the serialized data. 
 *
 * serializedSchemaData : The serialized schema that has to deserialized to schema
 *
 * returns : deserialized schema once successful
 

Record* deserializeRecord(char *deserialize_record_str, Schema *schema)

* This function is used to deserilaize the token strings. 
*
* deserialize_record_str : The token string to be deserialized into the record strings
* schema: schema with the strings
* 
* returns : record after deserialization is successful


Table and Manager
------------------
initRecordManager (void *mgmtData)

* This method initializes the record manager and allocates required memory.
*
* returns : RC_OK as initialing is done and nothing is left to initialize.

shutdownRecordManager ()

* This method shuts down the record manager.
*
* returns : RC_OK as memory is made free during allocation.

createTable (char *name, Schema *schema)

* This function is used to Create a Table.
* Create the underlying page file and store information about the schema, free-space, ...
* and so on in the Table Information pages.
*
* name: Name of the relation/table.
* schema: Schema of the table.
*
* returns : RC_FILE_NOT_FOUND if pagefile creation of opening fails.
*					 RC_WRITE_FAILED if write operation for writing serialized data fails.
* 				 RC_OK if all steps are executed and table is created.

openTable (RM_TableData *rel, char *name)

* This function is used to Open a table which is already created with name. This should have a pageFile created.
* For any operation to be performed, the table has to be opened first.
*
* name: Name of the relation/table.
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK if all steps are executed and table is opened.

closeTable (RM_TableData *rel)

* The table is closed after all the operations are completed.
* All the memory allocations are de-allocated to avoid memory leaks.
*
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK after all memory allocations are de-allocated and table is closed.

deleteTable (char *name)

* This function deletes the table using destroyPageFile function from buffermanager.
*
* name: Name of the relation/table.
*
* returns : RC_OK if destroy pagefile is successful.
*					RC_FILE_NOT_FOUND if file is pagefile is not found.

getNumTuples (RM_TableData *rel)

* This function is used to get number of tuples/rows in the table.
*
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK if destroy getRecord is successful.


Records handling in a table
----------------------------
insertRecord (RM_TableData *rel, Record *record)

* This function is used to insert a new record into the table.
* When a new record is inserted the record manager should assign an
* RID to this record and update the record parameter passed to insertRecord .
*
* rel: Management Structure for a Record Manager to handle one relation.
* record: Management Structure for Record which has rid and data of a tuple.
*
* returns : RC_OK if destroy getRecord is successful.

deleteRecord (RM_TableData *rel, RID id)

* This function is used to delete a record from the table.
*
* rel: Management Structure for a Record Manager to handle one relation.
* id: rid to be deleted.
*
* returns : RC_OK if delete record is successful.
*					 RC_RM_NO_MORE_TUPLES if no tuples are available to delete.

updateRecord (RM_TableData *rel, Record *record)

* This function is used to update a record in the table.
*
* rel: Management Structure for a Record Manager to handle one relation.
* record: Management Structure for a Record to store rid and data of a tuple.
*
* returns : RC_OK if delete record is successful.
*					 RC_RM_NO_MORE_TUPLES if no tuples are available to update.

getRecord (RM_TableData *rel, RID id, Record *record)

* This function is used to get a record from the table using rid.
*
*
* rel: Management Structure for a Record Manager to handle one relation.
* rid: Record identifier.
* record: Management Structure for a Record to store rid and data of a tuple.
*
* returns : RC_OK if delete record is successful.
*					 RC_RM_NO_MORE_TUPLES if no tuples are available to update.

Scans
-----
startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)

* This function is used to start scanning the table using scan management structure.
*
*
* rel: Management Structure for a Record Manager to handle one relation.
* scan_mgmt: holds the scan management data
*
* returns : RC_OK if initializing scan is successful.

next (RM_ScanHandle *scan, Record *record)

* This function is used with the above function to perform the scan function
*
*
* rid: Record identifier.
* record: Management Structure for a Record to store rid and data of a tuple.
*
* returns : RC_OK if scan operation is successful.
*			 RC_RM_NO_MORE_TUPLES if no tuples are available to scan.


closeScan (RM_ScanHandle *scan)

* This function is used to clean all the resources used by the record manager
*
* scan_mgmt: holds the scan management data
* record: Management Structure for a Record to store rid and data of a tuple.
*
* returns : RC_OK if closing the scan operation is successful.

Dealing with schemas
---------------------
getRecordSize (Schema *schema)

* Function: getRecordSize
* ---------------------------
* This function is used to get a record size for dealing with schemas
*
* numAttr: attribute count in the schema
*
* returns : recordSize with the size of the record.


createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)

* This function is used to cretae a scehma and initialize its attributes 
*
* numAttr: number of attributes in the schema
* attrNames: names of the attributes of schema
* dataTypes: datatype of every attribute
* typeLength: size of the attributes
* keySize: size of the schema keys
* keyAttrs: attributes associated with the keys
*
* returns : the newly created schema  

freeSchema (Schema *schema)

* This function is free the created schema after the operations are performed. 
*
* returns : RC_OK if free schema is successful.


Dealing with records and attribute values
------------------------------------------
createRecord (Record **record, Schema *schema)

* This function is used to create a record for the table by allocating memory.
*
* record: Management Structure for a Record to store rid and data of a tuple.
*
* returns : RC_OK if creation of record is successful.

freeRecord (Record *record)

 * This function is used to free a record from the table and the data associated with it.
 *
 * data: data stored in the record
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if freeing the record is successful

getAttr (Record *record, Schema *schema, int attrNum, Value **value)

 * This function is get the attribute associated with the schema. 
 *
 * offset: offset value of the attribute
 * attrData: data corresponding to the attribute
 * value: attribute value derived from the offset
 *
 * returns : RC_OK if the attribute is fetched properly.
 * 			 RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE if the datatype is not correct

setAttr (Record *record, Schema *schema, int attrNum, Value *value)
	
 * This function is set the attributes inside a record.
 *
 * offset: offset value of the attribute
 * attrData: data corresponding to the attribute
 * value: attribute value derived from the offset
 *
 * returns : RC_OK if the attribute is fetched properly.
 * 			 RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE if the datatype is not correct




Test Cases

-----------------------------------------------------------

Files: test_assign3_1.c

1. The program verifies all the test cases that are mentioned in the test file i.e test_assign3_1 and ensures that there are no errors.

2. The program also verfies the test case for test_expr.c and ensure there are no errors.

3. Both the test cases run perfectly. Some memory leaks in the test cases are resolved.
