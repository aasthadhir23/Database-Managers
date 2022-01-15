
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "record_mgr.h"
#include "expr.h"


int totalPages;
//Management structure for maintaining RECORD MANGER metadata.
typedef struct RM_RecordMgmt
{
	int *freePages;
	BM_BufferPool *bm;
} 
RM_RecordMgmt;

//Management structure for maintaining TABLE metadata.
typedef struct RM_TableInfo
{
	
	int schemaSize;
	int numOfTuples;
}
RM_TableInfo;


//Management structure for maintaining RECORD SCAN MANGER metadata.
typedef struct RM_ScanMgmt
{
	Record *currentRecord;
	int currentPage;
	int currentSlot;
	Expr *condn;	
}
RM_ScanMgmt;



/*
 * Function: updatePageInfo
 * ---------------------------
 * This method is used to update the page information by calling
 * makeDirty, unpinPage and forcePage functions in buffer manager.
 *
 * schema : Management structure to maintain schema details.
 * result : Used to store the offset.
 * attrNum : Number of attributes.
 *
 * returns : RC_OK if all steps in attributeOffSet are successful.
 *
 */

void updatePageInfo(RM_TableData *rel, BM_PageHandle *page){
	markDirty(((RM_RecordMgmt *)rel->mgmtData)->bm, page);
	unpinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page);
	forcePage(((RM_RecordMgmt *)rel->mgmtData)->bm, page);
}

/*
 * Function: attrituteOffset
 * ---------------------------
 * This method calculates the offset associated with each attribute
 * by taking size of each attribute datatype.
 *
 * schema : Management structure to maintain schema details.
 * result : Used to store the offset.
 * attrNum : Number of attributes.
 *
 * returns : RC_OK if all steps in attributeOffSet are successful.
 *
 */

RC attrOffset (Schema *schema, int attrNum, int *result)
{
	int offset = 0;
	int attrPosition = 0;

	for(attrPosition = 0; attrPosition < attrNum; attrPosition++)
	{
		switch (schema->dataTypes[attrPosition])
		{
		case DT_STRING:
			offset += schema->typeLength[attrPosition];
			break;
		case DT_INT:
			offset += sizeof(int);
			break;
		case DT_FLOAT:
			offset += sizeof(float);
			break;
		case DT_BOOL:
			offset += sizeof(bool);
			break;
		}
	}

	*result = offset;
	return RC_OK;
}

/*
 * Function: initRecordManager
 * ---------------------------
 * This method initializes the record manager.
 *
 * returns : RC_OK as initialing is done and nothing is left to initialize.
 *
 */

RC initRecordManager (void *mgmtData)
{
	printf("Initializing record manager done!\n");
	return RC_OK;
}

/*
 * Function: shutdownRecordManager
 * ---------------------------
 * This method shuts down the record manager.
 *
 * returns : RC_OK as memory is made free during allocation.
 *
 */
RC shutdownRecordManager ()
{
	printf("Shutdown of record manager done!\n");
	return RC_OK;
}

/*
 * Function: createTable
 * ---------------------------
 * This function is used to Create a Table.
 * Create the underlying page file and store information about the schema, free-space, ...
 * and so on in the Table Information pages.
 *
 * name: Name of the relation/table.
 * schema: Schema of the table.
 *
 * returns : RC_FILE_NOT_FOUND if pagefile creation of opening fails.
 *					 RC_WRITE_FAILED if write operation for writing serialized data fails.
 * 					 RC_OK if all steps are executed and table is created.
 *
 */
RC createTable (char *name, Schema *schema)
{
	SM_FileHandle filehandle;
 	char *serializedData = serializeSchema(schema);
	int i;
	for(i=0;i<sizeof(serializedData); i++){
		printf("%c ",serializedData[i]);
	}

 	RM_TableInfo *tableInfo = (RM_TableInfo *)malloc(sizeof(RM_TableInfo));

 	RC createPageFlag = createPageFile(name);
 	RC openPageFlag = openPageFile(name,&filehandle);
 	if(createPageFlag!=RC_OK || openPageFlag!=RC_OK)
 	{
 		return RC_FILE_NOT_FOUND;
 	}

 	tableInfo->schemaSize = 0;
 	RC writeflag = writeBlock(0,&filehandle,serializedData);
 	if(writeflag!=RC_OK)
 	{
 		return RC_WRITE_FAILED;
 	}
	return RC_OK;
}

/*
* Function: openTable
* ---------------------------
* This function is used to Open a table which is already created with name. This should have a pageFile created.
* For any operation to be performed, the table has to be opened first.
*
* name: Name of the relation/table.
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK if all steps are executed and table is opened.
*/
RC openTable (RM_TableData *rel, char *name)
{
	RM_RecordMgmt *rm_mgmt = (RM_RecordMgmt*)malloc(sizeof(RM_RecordMgmt));
	FILE *fptr = fopen(name, "r+");	char* readHeader;
	readHeader = (char*)calloc(PAGE_SIZE,sizeof(char));
	fgets(readHeader,PAGE_SIZE,fptr);
	char* totalPage;
	totalPage = readHeader;
	totalPages = atoi(totalPage);
	rm_mgmt->bm = MAKE_POOL();

	//Make a Page Handle
	BM_PageHandle *page = MAKE_PAGE_HANDLE();
	initBufferPool(rm_mgmt->bm,name,6,RS_FIFO,NULL);
	pinPage(rm_mgmt->bm,page,0);
	rm_mgmt->freePages = (int*)malloc(sizeof(int));
	rm_mgmt->freePages[0] = totalPages;
	rel->schema = deserializeSchema(page->data);
	rel->name = name;
	rel->mgmtData = rm_mgmt;
	free(readHeader);
	free(page);
	return RC_OK;
}

/*
* Function: closeTable
* ---------------------------
* The table is closed after all the operations are completed.
* All the memory allocations are de-allocated to avoid memory leaks.
*
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK after all memory allocations are de-allocated and table is closed.
*/
RC closeTable (RM_TableData *rel)
{
	printf(" Entered close Table \n");
	RM_RecordMgmt *rmgmt = (RM_RecordMgmt*)malloc(sizeof(RM_RecordMgmt));
	rmgmt = rel->mgmtData;
	shutdownBufferPool(rmgmt->bm);
	free(rmgmt);
	free(rel->schema->attrNames);
	free(rel->schema->dataTypes);
	free(rel->schema->keyAttrs);
	free(rel->schema->typeLength);

	free(rel->schema);

	return RC_OK;
}

/*
* Function: deleteTable
* ---------------------------
* This function deletes the table using destroyPageFile function from buffermanager.
*
* name: Name of the relation/table.
*
* returns : RC_OK if destroy pagefile is successful.
*					RC_FILE_NOT_FOUND if file is pagefile is not found.
*/

RC deleteTable (char *name)
{
	printf("Entered Delete ");
	RC destroyFlag = destroyPageFile(name);
	if(destroyFlag == RC_OK)
	{
		return RC_OK;
	}
	else{
		return RC_FILE_NOT_FOUND;
	}
}

/*
* Function: getNumTuples
* ---------------------------
* This function is used to get number of tuples/rows in the table.
*
* rel: Management Structure for a Record Manager to handle one relation.
*
* returns : RC_OK if destroy getRecord is successful.
*/

int getNumTuples (RM_TableData *rel)
{
	Record *record = (Record *)malloc(sizeof(Record));
	RID rid;
	int count = 0;
	RC flagGetRecord;

	rid.page = 1;
	rid.slot = 0;

	while(rid.page > 0 && rid.page < totalPages)
	{
		flagGetRecord = getRecord (rel, rid, record);
		if(flagGetRecord != RC_OK)
		{
			printf("Get records failed!");
		}
		else{
			count += 1;
			rid.page += 1;
			rid.slot = 0;
		}
	}

	record = NULL;
	free(record);
	return count;
}

/*
 * Function: insertRecord
 * ---------------------------
 * This function is used to insert a new record into the table.
 * When a new record is inserted the record manager should assign an
 * RID to this record and update the record parameter passed to insertRecord .
 *
 * rel: Management Structure for a Record Manager to handle one relation.
 * record: Management Structure for Record which has rid and data of a tuple.
 *
 * returns : RC_OK if destroy getRecord is successful.
 */

RC insertRecord (RM_TableData *rel, Record *record)
{
	Record *r = (Record *)malloc(sizeof(Record));
	RID rid;
	rid.page = 1;
	rid.slot = 0;

	while(rid.page > 0 && rid.page < totalPages)
	{
		rid.page = rid.page + 1;
		rid.slot = 0;
	}
	r = NULL;
	free(r);
	((RM_RecordMgmt *)rel->mgmtData)->freePages[0] = rid.page;
	BM_PageHandle *page = MAKE_PAGE_HANDLE();
	/*if(strncmp(page->data, "DEL", 3) == 0)
		return RC_RM_UPDATE_NOT_POSSIBLE_ON_DELETED_RECORD;*/

	record->id.page = ((RM_RecordMgmt *)rel->mgmtData)->freePages[0];
	record->id.slot = 0;

	char * serializedRecord = serializeRecord(record, rel->schema);

	pinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page, ((RM_RecordMgmt *)rel->mgmtData)->freePages[0]);

	memset(page->data, '\0', strlen(page->data));
	sprintf(page->data, "%s", serializedRecord);
	updatePageInfo(rel,page);
	//printf("record data: %s\n", page->data);
	free(page);
	((RM_RecordMgmt *)rel->mgmtData)->freePages[0] += 1;
	totalPages++;
	return RC_OK;
}

/*
 * Function: deleteRecord
 * ---------------------------
 * This function is used to delete a record from the table.
 *
 * rel: Management Structure for a Record Manager to handle one relation.
 * id: rid to be deleted.
 *
 * returns : RC_OK if delete record is successful.
 *					 RC_RM_NO_MORE_TUPLES if no tuples are available to delete.
 */
RC deleteRecord (RM_TableData *rel, RID id)
{
	
	char deleteFlag[3] = "DEL";
	char *deletedflagstr = (char*)malloc(sizeof(char*));
	if(id.page > 0 && id.page <=  totalPages)
	{
		BM_PageHandle *page = MAKE_PAGE_HANDLE();
/*		if(strncmp(page->data, "DEL", 3) == 0)
			return RC_RM_UPDATE_NOT_POSSIBLE_ON_DELETED_RECORD;*/
		pinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page, id.page);
		strcpy(deletedflagstr, deleteFlag);
		strcat(deletedflagstr, page->data);
		page->pageNum = id.page;
		memset(page->data, '\0', strlen(page->data));
		sprintf(page->data, "%s", deletedflagstr);
		printf("deleted record: %s\n", page->data);
		updatePageInfo(rel,page);
		page = NULL;
		free(page);
		return RC_OK;
	}
	else
	{
		return RC_RM_NO_MORE_TUPLES;
	}

	return RC_OK;
}


/*
 * Function: updateRecord
 * ---------------------------
 * This function is used to update a record in the table.
 *
 * rel: Management Structure for a Record Manager to handle one relation.
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if delete record is successful.
 *					 RC_RM_NO_MORE_TUPLES if no tuples are available to update.
 */

RC updateRecord (RM_TableData *rel, Record *record)
{
	printf("record to be updated: %s\n", record->data);
	// Check boundary conditions for tuple availability
	if(record->id.page <= 0 && record->id.page >  totalPages)
	{
		return RC_RM_NO_MORE_TUPLES;
	}
	else
	{
		BM_PageHandle *page = MAKE_PAGE_HANDLE();
		int pageNum, slotNum;
		pageNum = record->id.page;
		slotNum = record->id.slot;
		char *record_str = serializeRecord(record, rel->schema);
		pinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page, record->id.page);
		memset(page->data, '\0', strlen(page->data));
		sprintf(page->data, "%s", record_str);
		free(record_str);
		updatePageInfo(rel,page);
		free(page);
		return RC_OK;
	}

	return RC_OK;
}

/*
 * Function: getRecord
 * ---------------------------
 * This function is used to get a record from the table using rid.
 *
 *
 * rel: Management Structure for a Record Manager to handle one relation.
 * rid: Record identifier.
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if delete record is successful.
 *					 RC_RM_NO_MORE_TUPLES if no tuples are available to update.
 *
 */

RC getRecord (RM_TableData *rel, RID id, Record *record)
{

	if(id.page <= 0 && id.page >  totalPages)
	{
		return RC_RM_NO_MORE_TUPLES;
	}
	else
	{
		BM_PageHandle *page = MAKE_PAGE_HANDLE();
		pinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page, id.page);
		char *record_data = (char*)malloc(sizeof(char) * strlen(page->data));
		strcpy(record_data,page->data);
		printf("%s record data: \n" ,record_data);
		
		
		record->id = id;
	  Record* deSerializedRecord = deserializeRecord(record_data,rel->schema);
		unpinPage(((RM_RecordMgmt *)rel->mgmtData)->bm, page);
		record->data = deSerializedRecord->data;
		if(strncmp(record_data, "DEL", 3) == 0)
			return RC_RM_UPDATE_NOT_POSSIBLE_ON_DELETED_RECORD;
		free(deSerializedRecord);
		free(page);

		return RC_OK;
	}

	return RC_OK;
}


/*
SCANS
*/

/*
 * Function: startScan
 * ---------------------------
 * This function is used to start scanning the table using scan management structure.
 *
 *
 * rel: Management Structure for a Record Manager to handle one relation.
 * scan_mgmt: holds the scan management data
 *
 * returns : RC_OK if initializing scan is successful.
 *					 
 */

RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{

	//Initialize the Scan Management Struct
	RM_ScanMgmt *scan_mgmt = (RM_ScanMgmt*) malloc (sizeof(RM_ScanMgmt));
	
	scan_mgmt->currentRecord = (Record*) malloc (sizeof(Record));
	
	//using Scan Handle Structure & init its attributes
	scan->rel = rel;

	scan_mgmt->currentPage = 1;
	scan_mgmt->currentSlot = 0;
	scan_mgmt->condn = cond;

	//update and store the managememt data
	scan->mgmtData = scan_mgmt;

	return RC_OK;
}

/*
 * Function: next
 * ---------------------------
 * This function is used with the above function to perform the scan function
 *
 *
 * rid: Record identifier.
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if scan operation is successful.
 *			 RC_RM_NO_MORE_TUPLES if no tuples are available to scan.
 *
 */
RC next (RM_ScanHandle *scan, Record *record)
{
	Value *result;
	RID rid;

	rid.page = ((RM_ScanMgmt *)scan->mgmtData)->currentPage;
	rid.slot = ((RM_ScanMgmt *)scan->mgmtData)->currentSlot;

	if(((RM_ScanMgmt *)scan->mgmtData)->condn == NULL)
	{
		while(rid.page > 0 && rid.page < totalPages)
		{
			getRecord (scan->rel, rid, ((RM_ScanMgmt *)scan->mgmtData)->currentRecord);

			record->data = ((RM_ScanMgmt *)scan->mgmtData)->currentRecord->data;
			record->id = ((RM_ScanMgmt *)scan->mgmtData)->currentRecord->id;
			((RM_ScanMgmt *)scan->mgmtData)->currentPage = ((RM_ScanMgmt *)scan->mgmtData)->currentPage + 1;

			rid.page = ((RM_ScanMgmt *)scan->mgmtData)->currentPage;
			rid.slot = ((RM_ScanMgmt *)scan->mgmtData)->currentSlot;

			return RC_OK;
		}
	}
	else	
	{
		while(rid.page > 0 && rid.page < totalPages)
		{
			getRecord (scan->rel, rid, ((RM_ScanMgmt *)scan->mgmtData)->currentRecord);

			evalExpr (((RM_ScanMgmt *)scan->mgmtData)->currentRecord, scan->rel->schema, ((RM_ScanMgmt *)scan->mgmtData)->condn, &result);
			
			if(result->dt == DT_BOOL && result->v.boolV)
			{
				record->data = ((RM_ScanMgmt *)scan->mgmtData)->currentRecord->data;
				record->id = ((RM_ScanMgmt *)scan->mgmtData)->currentRecord->id;
				((RM_ScanMgmt *)scan->mgmtData)->currentPage = ((RM_ScanMgmt *)scan->mgmtData)->currentPage + 1;

				return RC_OK;
			}
			else	
			{
				((RM_ScanMgmt *)scan->mgmtData)->currentPage = ((RM_ScanMgmt *)scan->mgmtData)->currentPage + 1;
				rid.page = ((RM_ScanMgmt *)scan->mgmtData)->currentPage;
				rid.slot = ((RM_ScanMgmt *)scan->mgmtData)->currentSlot;
			}
		}
	}

	((RM_ScanMgmt *)scan->mgmtData)->currentPage = 1;

	return RC_RM_NO_MORE_TUPLES;
}

/*
 * Function: closeScan
 * ---------------------------
 * This function is used to clean all the resources used by the record manager
 *
 * scan_mgmt: holds the scan management data
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if closing the scan operation is successful.
 */
 
 RC closeScan (RM_ScanHandle *scan)
{
	//Make all the allocations, NULL and free them
	
	((RM_ScanMgmt *)scan->mgmtData)->currentRecord = NULL;
	free(((RM_ScanMgmt *)scan->mgmtData)->currentRecord);

	scan->mgmtData = NULL;
	
	free(scan->mgmtData);

	scan = NULL;
	free(scan);

	return RC_OK;
}

/*
 * DEALING WITH SCHEMAS
*/

/*
 * Function: getRecordSize
 * ---------------------------
 * This function is used to get a record size for dealing with schemas
 *
 * numAttr: attribute count in the schema
 *
 * returns : recordSize with the size of the record.
 *
 */
int getRecordSize(Schema *schema)
{
	int i, recordSize;
	recordSize = 0;

	for(i = 0; i < schema->numAttr; i++)
	{
		switch(schema->dataTypes[i])
		{
		case DT_INT:
			recordSize += sizeof(int);
			break;

		case DT_FLOAT:
			recordSize += sizeof(float);
			break;

		case DT_BOOL:
			recordSize += sizeof(bool);
			break;

		default:
			recordSize += schema->typeLength[i];
			break;
		}
	}

	return recordSize;
}

/*
 * Function: createSchema
 * ---------------------------
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
 *
 */

Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	//allocate memory for Schema to be created
	Schema *newSchema = (Schema*) malloc (sizeof(Schema));

	//initialize all the attributes for the schema
	newSchema->numAttr = numAttr;
	newSchema->attrNames = attrNames;
	newSchema->dataTypes = dataTypes;
	newSchema->typeLength = typeLength;
	newSchema->keySize = keySize;
	newSchema->keyAttrs = keys;

	return newSchema;
}

/*
 * Function: freeSchema
 * ---------------------------
 * This function is free the created schema after the operations are performed. 
 *
 * returns : RC_OK if free schema is successful.
 *
 */

RC freeSchema (Schema *schema)
{
	if(schema!=NULL)
	{
		//Free the schema content
		free(schema);
		return RC_OK;
    }
}

/*
 * DEALING WITH RECORDS AND ATTRIBUTE VALUES
 */

/*
 * Function: createRecord
 * ---------------------------
 * This function is used to create a record for the table by allocating memory.
 *
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if creation of record is successful.
 *
 */
 
RC createRecord (Record **rec, Schema *schema)
{
	//Allocating memory for record
	*rec = (Record*) malloc (sizeof(Record));
	//Allocating memory for data 
	(*rec)->data = (char*) malloc (getRecordSize(schema));

	return RC_OK;
}

/*
 * Function: freeRecord
 * ---------------------------
 * This function is used to free a record from the table and the data associated with it.
 *
 *
 * data: data stored in the record
 * record: Management Structure for a Record to store rid and data of a tuple.
 *
 * returns : RC_OK if freeing the record is successful.
 *
 */
RC freeRecord (Record *record)
{
	if(record!= NULL)
	{
		//Data is freed
		record->data = NULL;
		free(record->data);

		//Complete record is freed
		record = NULL;
		free(record);
    }
	return RC_OK;
}

/*
 * Function: getAttr
 * ---------------------------
 * This function is get the attribute associated with the schema. 
 *
 * offset: offset value of the attribute
 * attrData: data corresponding to the attribute
 * value: attribute value derived from the offset
 *
 * returns : RC_OK if the attribute is fetched properly.
 * 			 RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE if the datatype is not correct
 */
 
RC getAttr (Record *record, Schema *schema, int attrNum, Value **val)
{
	int offset;
	char *attrData;

	*val = (Value*)malloc(sizeof(Value));

	//calculate the offset, to get the attribute value from
	attrOffset(schema, attrNum, &offset);
	attrData = record->data + offset;

	(*val)->dt =schema->dataTypes[attrNum];

	
		if(schema->dataTypes[attrNum]==DT_INT)
			memcpy(&((*val)->v.intV) ,attrData,sizeof(int));	//get the attribute into value
		
		else if(schema->dataTypes[attrNum]==DT_STRING)
		{
			char *buf;
			int len = schema->typeLength[attrNum];
			buf = (char *) malloc(len + 1);
			strncpy(buf, attrData, len);
			buf[len] = '\0';
			(*val)->v.stringV = buf;
		}
		
		
		else if(schema->dataTypes[attrNum]==DT_FLOAT)
			memcpy(&((*val)->v.floatV),attrData, sizeof(float));

		else if(schema->dataTypes[attrNum]==DT_BOOL)
			memcpy(&((*val)->v.boolV),attrData ,sizeof(bool));
		
		else
			return RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE;
	

	return RC_OK;
}

/*
 * Function: setAttr
 * ---------------------------
 * This function is set the attributes inside a record.
 *
 * offset: offset value of the attribute
 * attrData: data corresponding to the attribute
 * value: attribute value derived from the offset
 *
 * returns : RC_OK if the attribute is fetched properly.
 * 			 RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE if the datatype is not correct
 *
 */
 
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
	int offset;
	char *attrData;

	//offset values
	attrOffset(schema, attrNum, &offset);
	attrData = record->data + offset;

	//Attributes datatype value		
	
	if(schema->dataTypes[attrNum]==DT_INT)
	{
		memcpy(attrData,&(value->v.intV) ,sizeof(int));		
	}

	else if(schema->dataTypes[attrNum]==DT_STRING)
	{
		char *buf;
		int len = schema->typeLength[attrNum];
		buf = (char *) malloc(len);
		buf = value->v.stringV;
		//end the string with '\0'
		buf[len] = '\0';			
		memcpy(attrData,buf,len);
	}
	else if(schema->dataTypes[attrNum]==DT_FLOAT)
			memcpy(attrData,&(value->v.floatV), sizeof(float));

	else if(schema->dataTypes[attrNum]==DT_BOOL)
		memcpy(attrData,&(value->v.boolV) ,sizeof(bool));

	else
			return RC_RM_NO_DESERIALIZER_FOR_THIS_DATATYPE;
			
	
	return RC_OK;
}
