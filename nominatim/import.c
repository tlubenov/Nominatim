/*
*/
#include <stdlib.h>
#include <string.h>

#include <libpq-fe.h>

#include <libxml/xmlstring.h>
#include <libxml/xmlreader.h>
#include <libxml/hash.h>

#include "nominatim.h"
#include "import.h"
#include "input.h"

typedef enum { FILETYPE_NONE, FILETYPE_STRUCTUREDV0P1 } filetypes_t;
typedef enum { FILEMODE_NONE, FILEMODE_ADD, FILEMODE_UPDATE, FILEMODE_DELETE } filemodes_t;

#define MAX_FEATUREADDRESS 500
#define MAX_FEATURENAMES 1000

struct feature_address {
	int			place_id;
	int			rankAddress;
	xmlChar *	type;
	xmlChar *	id;
	xmlChar *	key;
	xmlChar *	value;
	xmlChar *	distance;
};

struct feature_name {
	xmlChar *	type;
	xmlChar *	value;
};

struct feature {
	int			placeID;
	xmlChar *	type;
	xmlChar *	id;
	xmlChar *	key;
	xmlChar *	value;
	xmlChar *	rankAddress;
	xmlChar *	rankSearch;
	xmlChar *	countryCode;
	xmlChar * 	adminLevel;
	xmlChar *	houseNumber;
	xmlChar * 	geometry;
} feature;

int 					fileType = FILETYPE_NONE;
int 					fileMode = FILEMODE_ADD;
PGconn *				conn;
struct feature_address 	featureAddress[MAX_FEATUREADDRESS];
struct feature_name 	featureName[MAX_FEATURENAMES];
struct feature 			feature;
int 					featureAddressLines = 0;
int 					featureNameLines = 0;
int 					featureCount = 0;
xmlHashTablePtr 		partionTableTagsHash;



void StartElement(xmlTextReaderPtr reader, const xmlChar *name)
{
    char * value;
    float version;
    int isAddressLine;

    if (fileType == FILETYPE_NONE)
    {
    	// Potential to handle other file types in the future / versions
        if (xmlStrEqual(name, BAD_CAST "osmStructured"))
        {
        	value = (char*)xmlTextReaderGetAttribute(reader, BAD_CAST "version");
        	version = strtof(value, NULL);
        	xmlFree(value);

        	if (version == (float)0.1)
        	{
        		fileType = FILETYPE_STRUCTUREDV0P1;
        		fileMode = FILEMODE_ADD;
        	}
        	else
        	{
                fprintf( stderr, "Unknown osmStructured version %f\n", version );
                exit_nicely();
        	}
        }
        else
        {
            fprintf( stderr, "Unknown XML document type: %s\n", name );
            exit_nicely();
        }
        return;
    }

    if (xmlStrEqual(name, BAD_CAST "add"))
    {
    	fileMode = FILEMODE_ADD;
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "update"))
    {
    	fileMode = FILEMODE_UPDATE;
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "delete"))
    {
    	fileMode = FILEMODE_DELETE;
    	return;
    }
    if (fileMode == FILEMODE_NONE)
    {
        fprintf( stderr, "Unknown import mode in: %s\n", name );
        exit_nicely();
    }

    if (xmlStrEqual(name, BAD_CAST "feature"))
    {
    	feature.type = xmlTextReaderGetAttribute(reader, BAD_CAST "type");
    	feature.id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
    	feature.key = xmlTextReaderGetAttribute(reader, BAD_CAST "key");
    	feature.value = xmlTextReaderGetAttribute(reader, BAD_CAST "value");
    	feature.rankAddress = xmlTextReaderGetAttribute(reader, BAD_CAST "rank");
    	feature.rankSearch = xmlTextReaderGetAttribute(reader, BAD_CAST "importance");

		feature.countryCode = NULL;
		feature.adminLevel = NULL;
		feature.houseNumber = NULL;
		feature.geometry = NULL;
		featureAddressLines = 0;
		featureNameLines = 0;

    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "names")) return;
    if (xmlStrEqual(name, BAD_CAST "name"))
    {
    	featureName[featureNameLines].type = xmlTextReaderGetAttribute(reader, BAD_CAST "type");
    	featureName[featureNameLines].value = xmlTextReaderReadString(reader);
    	featureNameLines++;
    	if (featureNameLines >= MAX_FEATURENAMES)
    	{
            fprintf( stderr, "Too many name elements\n");
            exit_nicely();
    	}
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "osmGeometry"))
    {
    	feature.geometry = xmlTextReaderReadString(reader);
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "adminLevel"))
    {
    	feature.adminLevel = xmlTextReaderReadString(reader);
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "countryCode"))
    {
    	feature.countryCode = xmlTextReaderReadString(reader);
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "houseNumber"))
    {
    	feature.houseNumber = xmlTextReaderReadString(reader);
    	return;
    }
    if (xmlStrEqual(name, BAD_CAST "address"))
    {
    	featureAddressLines = 0;
    	return;
    }
    isAddressLine = 0;
    if (xmlStrEqual(name, BAD_CAST "continent"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "sea"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "country"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "state"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "county"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "city"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "town"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "village"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "unknown"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "suburb"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "postcode"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "neighborhood"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "street"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "access"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "building"))
    {
        isAddressLine = 1;
    }
    else if (xmlStrEqual(name, BAD_CAST "other"))
    {
        isAddressLine = 1;
    }
    if (isAddressLine)
    {
    	value = (char*)xmlTextReaderGetAttribute(reader, BAD_CAST "rank");
    	if (!value)
    	{
            fprintf( stderr, "Address element missing rank\n");
            exit_nicely();
    	}
    	featureAddress[featureAddressLines].rankAddress =  atoi(value);
    	xmlFree(value);

    	featureAddress[featureAddressLines].type = xmlTextReaderGetAttribute(reader, BAD_CAST "type");
    	featureAddress[featureAddressLines].id = xmlTextReaderGetAttribute(reader, BAD_CAST "id");
    	featureAddress[featureAddressLines].key = xmlTextReaderGetAttribute(reader, BAD_CAST "key");
    	featureAddress[featureAddressLines].value = xmlTextReaderGetAttribute(reader, BAD_CAST "value");
    	featureAddress[featureAddressLines].distance = xmlTextReaderGetAttribute(reader, BAD_CAST "distance");

    	featureAddressLines++;
    	if (featureAddressLines >= MAX_FEATUREADDRESS)
    	{
            fprintf( stderr, "Too many address elements\n");
            exit_nicely();
    	}

    	return;
    }
    fprintf(stderr, "%s: Unknown element name: %s\n", __FUNCTION__, name);
}

void EndElement(xmlTextReaderPtr reader, const xmlChar *name)
{
	PGresult * 		res;
	PGresult * 		resPlaceID;
    const char *	paramValues[11];
    char *			place_id;
    char *			partionQueryName;
	int i;

    if (xmlStrEqual(name, BAD_CAST "feature"))
	{
    	featureCount++;
    	if (featureCount % 1000 == 0) printf("feature %i(k)\n", featureCount/1000);

        if (fileMode == FILEMODE_ADD)
        {
        	resPlaceID = PQexecPrepared(conn, "get_new_place_id", 0, NULL, NULL, NULL, 0);
        	if (PQresultStatus(resPlaceID) != PGRES_TUPLES_OK)
			{
				fprintf(stderr, "get_place_id: INSERT failed: %s", PQerrorMessage(conn));
				PQclear(resPlaceID);
				exit(EXIT_FAILURE);
			}
        }
        else
        {
            paramValues[0] = (const char *)feature.type;
            paramValues[1] = (const char *)feature.id;
            paramValues[2] = (const char *)feature.key;
            paramValues[3] = (const char *)feature.value;
            resPlaceID = PQexecPrepared(conn, "get_new_place_id", 4, paramValues, NULL, NULL, 0);
            if (PQresultStatus(resPlaceID) != PGRES_TUPLES_OK)
            {
                fprintf(stderr, "index_placex: INSERT failed: %s", PQerrorMessage(conn));
                PQclear(resPlaceID);
                exit(EXIT_FAILURE);
            }
        }
		place_id = PQgetvalue(resPlaceID, 0, 0);

        if (fileMode == FILEMODE_UPDATE || fileMode == FILEMODE_DELETE)
        {
            paramValues[0] = (const char *)place_id;
            res = PQexecPrepared(conn, "placex_delete", 1, paramValues, NULL, NULL, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "placex_delete: DELETE failed: %s", PQerrorMessage(conn));
                PQclear(res);
                exit(EXIT_FAILURE);
            }
            PQclear(res);

            res = PQexecPrepared(conn, "search_name_delete", 1, paramValues, NULL, NULL, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "search_name_delete: DELETE failed: %s", PQerrorMessage(conn));
                PQclear(res);
                exit(EXIT_FAILURE);
            }
            PQclear(res);

            res = PQexecPrepared(conn, "place_addressline_delete", 1, paramValues, NULL, NULL, 0);
            if (PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                fprintf(stderr, "place_addressline_delete: DELETE failed: %s", PQerrorMessage(conn));
                PQclear(res);
                exit(EXIT_FAILURE);
            }
            PQclear(res);
        }

        if (fileMode == FILEMODE_UPDATE || fileMode == FILEMODE_ADD)
        {
			// Insert into placex
			paramValues[0] = (const char *)place_id;
			paramValues[1] = (const char *)feature.type;
			paramValues[2] = (const char *)feature.id;
			paramValues[3] = (const char *)feature.key;
			paramValues[4] = (const char *)feature.value;
//			paramValues[5] = (const char *)feature.name;
			paramValues[6] = (const char *)feature.adminLevel;
			paramValues[7] = (const char *)feature.houseNumber;
			paramValues[8] = (const char *)feature.rankAddress;
			paramValues[9] = (const char *)feature.rankSearch;
			paramValues[10] = (const char *)feature.geometry;
			res = PQexecPrepared(conn, "placex_insert", 11, paramValues, NULL, NULL, 0);
			if (PQresultStatus(res) != PGRES_COMMAND_OK)
			{
				fprintf(stderr, "index_placex: INSERT failed: %s", PQerrorMessage(conn));
				PQclear(res);
				exit(EXIT_FAILURE);
			}
			PQclear(res);

			for(i = 0; i < featureAddressLines; i++)
			{
				// insert into place_address
				paramValues[0] = (const char *)place_id;
				paramValues[1] = (const char *)featureAddress[i].distance;
				paramValues[2] = (const char *)featureAddress[i].type;
				paramValues[3] = (const char *)featureAddress[i].id;
				paramValues[4] = (const char *)featureAddress[i].key;
				paramValues[5] = (const char *)featureAddress[i].value;
				res = PQexecPrepared(conn, "place_addressline_insert", 6, paramValues, NULL, NULL, 0);
				if (PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					fprintf(stderr, "place_addressline_insert: INSERT failed: %s", PQerrorMessage(conn));
					PQclear(res);
					exit(EXIT_FAILURE);
				}
				PQclear(res);

				xmlFree(featureAddress[i].type);
				xmlFree(featureAddress[i].id);
				xmlFree(featureAddress[i].key);
				xmlFree(featureAddress[i].value);
				xmlFree(featureAddress[i].distance);
			}

			if (featureNameLines)
			{
				paramValues[0] = (const char *)place_id;
				res = PQexecPrepared(conn, "search_name_insert", 1, paramValues, NULL, NULL, 0);
				if (PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					fprintf(stderr, "search_name_insert: INSERT failed: %s", PQerrorMessage(conn));
					PQclear(res);
					exit(EXIT_FAILURE);
				}
				PQclear(res);
			}

			partionQueryName = xmlHashLookup2(partionTableTagsHash, feature.key, feature.value);
			if (partionQueryName)
			{
				// insert into partition table
				paramValues[0] = (const char *)place_id;
				paramValues[1] = (const char *)feature.geometry;
				res = PQexecPrepared(conn, partionQueryName, 2, paramValues, NULL, NULL, 0);
				if (PQresultStatus(res) != PGRES_COMMAND_OK)
				{
					fprintf(stderr, "%s: INSERT failed: %s", partionQueryName, PQerrorMessage(conn));
					PQclear(res);
					exit(EXIT_FAILURE);
				}
				PQclear(res);

			}

        }
        else
        {
			for(i = 0; i < featureAddressLines; i++)
			{
				xmlFree(featureAddress[i].type);
				xmlFree(featureAddress[i].id);
				xmlFree(featureAddress[i].key);
				xmlFree(featureAddress[i].value);
				xmlFree(featureAddress[i].distance);
			}
        }

		xmlFree(feature.type);
		xmlFree(feature.id);
		xmlFree(feature.key);
		xmlFree(feature.value);
		xmlFree(feature.rankAddress);
		xmlFree(feature.rankSearch);
//		if (feature.name) xmlFree(feature.name);
		if (feature.countryCode) xmlFree(feature.countryCode);
		if (feature.adminLevel) xmlFree(feature.adminLevel);
		if (feature.houseNumber) xmlFree(feature.houseNumber);
		if (feature.geometry) xmlFree(feature.geometry);

		PQclear(resPlaceID);
	}
}

static void processNode(xmlTextReaderPtr reader)
{
	xmlChar *name;
    name = xmlTextReaderName(reader);
    if (name == NULL)
    {
        name = xmlStrdup(BAD_CAST "--");
    }

    switch(xmlTextReaderNodeType(reader))
    {
        case XML_READER_TYPE_ELEMENT:
            StartElement(reader, name);
            if (xmlTextReaderIsEmptyElement(reader))
                EndElement(reader, name); /* No end_element for self closing tags! */
            break;
        case XML_READER_TYPE_END_ELEMENT:
            EndElement(reader, name);
            break;
        case XML_READER_TYPE_TEXT:
        case XML_READER_TYPE_CDATA:
        case XML_READER_TYPE_SIGNIFICANT_WHITESPACE:
            /* Ignore */
            break;
        default:
            fprintf(stderr, "Unknown node type %d\n", xmlTextReaderNodeType(reader));
            break;
    }

    xmlFree(name);
}

int nominatim_import(const char *conninfo, const char *partionTagsFilename, const char *filename)
{
    xmlTextReaderPtr	reader;
    int 				ret = 0;
	PGresult * 			res;
	FILE *				partionTagsFile;
	char * 				partionQueryName;
    char 				partionQuerySQL[1024];

	conn = PQconnectdb(conninfo);
    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        exit(EXIT_FAILURE);
    }

	partionTableTagsHash = xmlHashCreate(200);

	partionTagsFile = fopen(partionTagsFilename, "rt");
	if (!partionTagsFile)
    {
        fprintf(stderr, "Unable to read partition tags file: %s\n", partionTagsFilename);
        exit(EXIT_FAILURE);
    }

	char buffer[1024], osmkey[256], osmvalue[256];
	int fields;
	while(fgets(buffer, sizeof(buffer), partionTagsFile) != NULL)
	{
		fields = sscanf( buffer, "%23s %63s", osmkey, osmvalue );

		if( fields <= 0 ) continue;

	    if( fields != 2  )
	    {
	      fprintf( stderr, "Error partition file\n");
	      exit_nicely();
	    }
	    partionQueryName = malloc(strlen("partition_insert_")+strlen(osmkey)+strlen(osmvalue)+2);
		strcpy(partionQueryName, "partition_insert_");
		strcat(partionQueryName, osmkey);
		strcat(partionQueryName, "_");
		strcat(partionQueryName, osmvalue);

		strcpy(partionQuerySQL, "insert into place_classtype_");
		strcat(partionQuerySQL, osmkey);
		strcat(partionQuerySQL, "_");
		strcat(partionQuerySQL, osmvalue);
		strcat(partionQuerySQL, " (place_id, centroid) values ($1, ST_Centroid(st_setsrid($2, 4326)))");

	    res = PQprepare(conn, partionQueryName, partionQuerySQL, 2, NULL);
	    if (PQresultStatus(res) != PGRES_COMMAND_OK)
		{
	        fprintf(stderr, "Failed to prepare %s: %s\n", partionQueryName, PQerrorMessage(conn));
	    	exit(EXIT_FAILURE);
		}

		xmlHashAddEntry2(partionTableTagsHash, BAD_CAST osmkey, BAD_CAST osmvalue, BAD_CAST partionQueryName);
	}

    res = PQprepare(conn, "get_new_place_id",
        	"select nextval('seq_place')",
    	0, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare get_new_place_id: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "get_place_id",
        	"select place_id from placex where osm_type = $1 and osm_id = $2 and class = $3 and type = $4",
    	4, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare get_place_id: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "placex_insert",
        	"insert into placex (place_id,osm_type,osm_id,class,type,name,admin_level,housenumber,rank_address,rank_search,geometry) "
        	"values ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, st_setsrid($11, 4326))",
    	11, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare placex_insert: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "search_name_insert",
        	"insert into search_name (place_id, search_rank, address_rank, country_code, name_vector, nameaddress_vector, centroid) "
        	"select place_id, rank_address, rank_search, country_code, make_keywords(name), "
        	"(select uniq(sort(array_agg(name_vector))) from place_addressline join search_name on "
        	"(address_place_id = search_name.place_id) where place_addressline.place_id = $1 ), st_centroid(geometry) from placex "
        	"where place_id = $1",
    	1, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare placex_insert: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "place_addressline_insert",
        	"insert into place_addressline (place_id, address_place_id, fromarea, isaddress, distance, cached_rank_address) "
        	"select $1, place_id, false, true, $2, rank_address from placex where osm_type = $3 and osm_id = $4 and class = $5 and type = $6",
    	6, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare place_addressline_insert: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "placex_delete",
        	"delete from placex where place_id = $1",
    	1, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare placex_delete: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "search_name_delete",
        	"delete from search_name where place_id = $1",
    	1, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare search_name_delete: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    res = PQprepare(conn, "place_addressline_delete",
        	"delete from place_addressline where place_id = $1",
    	1, NULL);
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
	{
        fprintf(stderr, "Failed to prepare place_addressline_delete: %s\n", PQerrorMessage(conn));
    	exit(EXIT_FAILURE);
	}

    featureCount = 0;

    reader = inputUTF8(filename);

    if (reader == NULL)
    {
        fprintf(stderr, "Unable to open %s\n", filename);
        return 1;
    }

    ret = xmlTextReaderRead(reader);
    while (ret == 1)
    {
    	processNode(reader);
    	ret = xmlTextReaderRead(reader);
    }
	if (ret != 0) {
		fprintf(stderr, "%s : failed to parse\n", filename);
		return ret;
	}

	xmlFreeTextReader(reader);
	xmlHashFree(partionTableTagsHash, NULL);

    return 0;
}
