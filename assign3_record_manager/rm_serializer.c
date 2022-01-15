#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dberror.h"
#include "tables.h"
#include "record_mgr.h"

// dynamic string
typedef struct VarString {
  char *buf;
  int size;
  int bufsize;
} VarString;

#define MAKE_VARSTRING(var)				\
  do {							\
  var = (VarString *) malloc(sizeof(VarString));	\
  var->size = 0;					\
  var->bufsize = 100;					\
  var->buf = malloc(100);				\
  } while (0)

#define FREE_VARSTRING(var)			\
  do {						\
  free(var->buf);				\
  free(var);					\
  } while (0)

#define GET_STRING(result, var)			\
  do {						\
    result = malloc((var->size) + 1);		\
    memcpy(result, var->buf, var->size);	\
    result[var->size] = '\0';			\
  } while (0)

#define RETURN_STRING(var)			\
  do {						\
    char *resultStr;				\
    GET_STRING(resultStr, var);			\
    FREE_VARSTRING(var);			\
    return resultStr;				\
  } while (0)

#define ENSURE_SIZE(var,newsize)				\
  do {								\
    if (var->bufsize < newsize)					\
    {								\
      int newbufsize = var->bufsize;				\
      while((newbufsize *= 2) < newsize);			\
      var->buf = realloc(var->buf, newbufsize);			\
    }								\
  } while (0)

#define APPEND_STRING(var,string)					\
  do {									\
    ENSURE_SIZE(var, var->size + strlen(string));			\
    memcpy(var->buf + var->size, string, strlen(string));		\
    var->size += strlen(string);					\
  } while(0)

#define APPEND(var, ...)			\
  do {						\
    char *tmp = malloc(10000);			\
    sprintf(tmp, __VA_ARGS__);			\
    APPEND_STRING(var,tmp);			\
    free(tmp);					\
  } while(0)

// prototypes
static RC attrOffset (Schema *schema, int attrNum, int *result);

// Seeializing the schema
char *serializeTableInfo(RM_TableData *rel)
{
  VarString *result;
  MAKE_VARSTRING(result);
  
  APPEND(result, "TABLE <%s> with <%i> tuples:\n", rel->name, getNumTuples(rel));
  APPEND_STRING(result, serializeSchema(rel->schema));
  
  RETURN_STRING(result);  
}

char *serializeTableContent(RM_TableData *rel)
{
  int i;
  VarString *result;
  RM_ScanHandle *sc = (RM_ScanHandle *) malloc(sizeof(RM_ScanHandle));
  Record *r = (Record *) malloc(sizeof(Record));
  MAKE_VARSTRING(result);

  for(i = 0; i < rel->schema->numAttr; i++)
    APPEND(result, "%s%s", (i != 0) ? ", " : "", rel->schema->attrNames[i]);

  startScan(rel, sc, NULL);
  
  while(next(sc, r) != RC_RM_NO_MORE_TUPLES) 
    {
    APPEND_STRING(result,serializeRecord(r, rel->schema));
    APPEND_STRING(result,"\n");
    }
  closeScan(sc);

  RETURN_STRING(result);
}


char *serializeSchema(Schema *schema)
{
  int i;
  VarString *result;
  MAKE_VARSTRING(result);

  APPEND(result, "Schema with <%i> attributes (", schema->numAttr);

  for(i = 0; i < schema->numAttr; i++)
    {
      APPEND(result,"%s%s: ", (i != 0) ? ", ": "", schema->attrNames[i]);
      switch (schema->dataTypes[i])
	{
	case DT_INT:
	  APPEND_STRING(result, "INT");
	  break;
	case DT_FLOAT:
	  APPEND_STRING(result, "FLOAT");
	  break;
	case DT_STRING:
	  APPEND(result,"STRING[%i]", schema->typeLength[i]);
	  break;
	case DT_BOOL:
	  APPEND_STRING(result,"BOOL");
	  break;
	}
    }
  APPEND_STRING(result,")");

  APPEND_STRING(result," with keys: (");

  for(i = 0; i < schema->keySize; i++)
    APPEND(result, "%s%s", ((i != 0) ? ", ": ""), schema->attrNames[schema->keyAttrs[i]]); 

  APPEND_STRING(result,")\n");

  RETURN_STRING(result);
}

char *serializeRecord(Record *record, Schema *schema)
{
  VarString *result;
  MAKE_VARSTRING(result);

  int i;
  
  APPEND(result, "[%i-%i] (", record->id.page, record->id.slot);

  for(i = 0; i < schema->numAttr; i++)
    {
      APPEND_STRING(result,serializeAttr (record, schema, i));
      APPEND(result, "%s", (i == (schema->numAttr-1)) ? "" : ",");
    }
  
  APPEND_STRING(result, ")");

  RETURN_STRING(result);
}

char *serializeAttr(Record *record, Schema *schema, int attrNum)
{
  int offset;
  char *attrData;
  VarString *result;
  MAKE_VARSTRING(result);
  
  attrOffset(schema, attrNum, &offset);
  attrData = record->data + offset;

  switch(schema->dataTypes[attrNum])
    {
    case DT_INT:
      {
	int val = 0;
	memcpy(&val,attrData, sizeof(int));
	APPEND(result, "%s:%i", schema->attrNames[attrNum], val);
      }
      break;
    case DT_STRING:
      {
	char *buf;
	int len = schema->typeLength[attrNum];
	buf = (char *) malloc(len + 1);
	strncpy(buf, attrData, len);
	buf[len] = '\0';
	
	APPEND(result, "%s:%s", schema->attrNames[attrNum], buf);
	free(buf);
      }
      break;
    case DT_FLOAT:
      {
	float val;
	memcpy(&val,attrData, sizeof(float));
	APPEND(result, "%s:%f", schema->attrNames[attrNum], val);
      }
      break;
    case DT_BOOL:
      {
	bool val;
	memcpy(&val,attrData, sizeof(bool));
	APPEND(result, "%s:%s", schema->attrNames[attrNum], val ? "TRUE" : "FALSE");
      }
      break;
    default:
      return "NO SERIALIZER FOR DATATYPE";
    }

  RETURN_STRING(result);
}

char *serializeValue(Value *val)
{
  VarString *result;
  MAKE_VARSTRING(result);
  
  switch(val->dt)
    {
    case DT_INT:
      APPEND(result,"%i",val->v.intV);
      break;
    case DT_FLOAT:
      APPEND(result,"%f", val->v.floatV);
      break;
    case DT_STRING:
      APPEND(result,"%s", val->v.stringV);
      break;
    case DT_BOOL:
      APPEND_STRING(result, ((val->v.boolV) ? "true" : "false"));
      break;
    }

  RETURN_STRING(result);
}

Value *stringToValue(char *val)
{
  Value *result = (Value *) malloc(sizeof(Value));
  
  switch(val[0])
    {
    case 'i':
      result->dt = DT_INT;
      result->v.intV = atoi(val + 1);
      break;
    case 'f':
      result->dt = DT_FLOAT;
      result->v.floatV = atof(val + 1);
      break;
    case 's':
      result->dt = DT_STRING;
      result->v.stringV = malloc(strlen(val));
      strcpy(result->v.stringV, val + 1);
      break;
    case 'b':
      result->dt = DT_BOOL;
      result->v.boolV = (val[1] == 't') ? TRUE : FALSE;
      break;
    default:
      result->dt = DT_INT;
      result->v.intV = -1;
      break;
    }
  
  return result;
}


RC attrOffset (Schema *schema, int attrNum, int *result)
{
  int offset = 0;
  int attrPos = 0;
  
  for(attrPos = 0; attrPos < attrNum; attrPos++)
    switch (schema->dataTypes[attrPos])
      {
      case DT_STRING:
	offset += schema->typeLength[attrPos];
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
  
  *result = offset;
  return RC_OK;
}



/*
 * Function: deserializeSchema
 * ---------------------------
 * This function is used to deserilaize the serialized data. 
 *
 * serializedSchemaData : The serialized schema that has to deserialized to schema
 *
 * returns : deserialized schema once successful
 *
 */

Schema *deserializeSchema(char *serializedSchemaData)
{
	VarString *result;
	
	MAKE_VARSTRING(result);
	
	int i,j,schemaNumAttr,lastAttr;

	Schema *schema;
	schema = (Schema*)malloc(sizeof(Schema));

	char *splitStart, *splitEnd, *splitString;
	 
	splitStart = (char*)malloc(sizeof(char));
	splitEnd  = (char*)malloc(sizeof(char));
	splitString = (char*)malloc(sizeof(char));

	splitStart = strtok(serializedSchemaData,"<");
	splitEnd = strtok(NULL,">");

	schemaNumAttr = strtol(splitEnd, &splitStart,10);

	schema->numAttr = schemaNumAttr;

	schema->attrNames = (char **) malloc (sizeof(char*) * schemaNumAttr);
	schema->dataTypes = (DataType*) malloc (sizeof (DataType) * schemaNumAttr);
	schema->typeLength = (int*) malloc (sizeof(int) * schemaNumAttr);

	splitEnd = strtok(NULL,"(");

  	lastAttr =  schemaNumAttr-1;

  	//enter dataTypes and their datalengths
  	for(i=0;i<schemaNumAttr;i++)
  	{
  		splitEnd = strtok(NULL,": ");

  		schema->attrNames[i] = (char*)malloc(sizeof(char*));
  		
  		strcpy(schema->attrNames[i],splitEnd);

  		if(i != lastAttr)
  		{
  			splitEnd = strtok(NULL,", ");
  		}
  		
  		else
  		{
  			splitEnd = strtok(NULL,") ");
  		}

  		
		if(strcmp(splitEnd,"FLOAT")==0)
  		{
  			schema->dataTypes[i] = DT_FLOAT;
  			schema->typeLength[i] = 0;
  		}
  		else if(strcmp(splitEnd,"INT")==0)
  		{
  			schema->dataTypes[i] = DT_INT;
  			schema->typeLength[i] = 0;
  		}
  		else if(strcmp(splitEnd,"BOOL")==0)
  		{
  			schema->dataTypes[i] = DT_BOOL;
  			schema->typeLength[i] = 0;
  		}
  		else
  		{
  			strcpy(splitString, splitEnd);
  			
  			char *str;
			str = (char*) malloc (sizeof(char));
  			
			sprintf(str,"%d",i);
  			strcat(splitString,str);

  			str = NULL;
  			free(str);
  		}
  	}

  	//to check ifkeys are present
  	if((splitEnd = strtok(NULL,"("))!=NULL)
  	{
  		
  		char *splitKey; 
		char *keyAttr[schemaNumAttr];
  		int numOfKeys = 0;
  		
		splitEnd = strtok(NULL,")");
		splitKey = (char*)malloc(sizeof(char));
  		splitKey = strtok(splitEnd,", ");

  		//Find out the number of Keys & store the attrValues for those Keys
  		while(splitKey!=NULL)
  		{
  			keyAttr[numOfKeys] = (char*)malloc(sizeof(char*));
  			strcpy(keyAttr[numOfKeys],splitKey);
  			numOfKeys++;
  			splitKey = strtok(NULL,", ");
  		}

  		splitKey = NULL;
  		free(splitKey);

  		//MARK all the key attrs as their INDEX values
  		schema->keyAttrs = (int*)malloc(sizeof(int)*numOfKeys);
  		schema->keySize = numOfKeys;

  		//for every Key, find the attributes and mark it's index
  		for(i=0;i<numOfKeys;i++)
  		{
  			for(j=0;j<schemaNumAttr;j++)
  			{
  				if(strcmp(keyAttr[i],schema->attrNames[j])==0)
  				{
  					schema->keyAttrs[i] = j;
  					break;
  				}
  			}
  		}
  	}

  	//for STRING[SIZE] allocate all the attributes

  	if(strlen(splitString)!=0)
  	{
  		splitString = strtok(splitString,"[");
  		if(strcmp(splitString,"STRING")==0)
  		{
  			int val, index;
  			splitString = strtok(NULL,"]");
  			val = atoi(splitString);
  			splitString = strtok(NULL,"=");
  			index = atoi(splitString);
  			schema->dataTypes[index] = DT_STRING;

  			schema->typeLength[index] = val;
  		}
  	}
	
	
  	splitString = NULL;
  	free(splitString);
  	splitStart = NULL;
  	free(splitStart);
  	splitEnd = NULL;
  	free(splitEnd);
  	
  	return schema;
}


/*
 * Function: deserializeSchema
 * ---------------------------
 * This function is used to deserilaize the token strings. 
 *
 * deserialize_record_str : The token string to be deserialized into the record strings
 * schema: schema with the strings
 * 
 * returns : record after deserialization is successful.
 *
 */
Record* deserializeRecord(char *deserialize_record_str, Schema *schema)
{
	int i, lastAttr;
	int integer;
	float fl;
	bool boolean;
	
	lastAttr = schema->numAttr-1;

	Value *value;
	Record *record;
	record = (Record*)malloc(sizeof(Record*));
	record->data = (char*) malloc (sizeof(char*));

	char *splitStart, *splitEnd;

	splitStart = strtok(deserialize_record_str,"(");

	for(i=0;i< schema->numAttr;i++)
	{
		splitEnd = strtok(NULL,":");

		if(i == lastAttr)
		{
			splitEnd = strtok(NULL,")");
		}
		else
		{
			splitEnd = strtok(NULL,",");
		}

		if(schema->dataTypes[i] == DT_INT)
		{
			integer = strtol(splitEnd, &splitStart, 10);
			MAKE_VALUE(value,DT_INT,integer);
			setAttr(record,schema,i,value);
			free(value);
		}
		else if(schema->dataTypes[i] == DT_FLOAT)
		{	fl = strtof(splitEnd, NULL);
			MAKE_VALUE(value,DT_FLOAT,fl);
			setAttr(record,schema,i,value);
			free(value);
		}
		else if(schema->dataTypes[i] == DT_BOOL)
		{
			boolean = (splitEnd[0] == 't') ? TRUE: FALSE;
			MAKE_VALUE(value,DT_BOOL,boolean);
			setAttr(record,schema,i,value);
			free(value);
		}
		else if(schema->dataTypes[i] == DT_STRING)
		{
			MAKE_STRING_VALUE(value,splitEnd);
			setAttr (record,schema,i,value);
			freeVal(value);
		}
		else 
			printf("Ivalid datatype");
	}

return record;
}
