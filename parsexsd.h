
/* parsexsd.h : Walk the xsd tree as provided by libxml
 *
 *
 * Copyright (c) 2018 Ram Gopalkrisha ramkumarg1@gmail.com */

#ifndef XML2JSON_XSD_H
#define XML2JSON_XSD_H
#endif

#define LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemastypes.h>
#include <libxml/schemasInternals.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif


/*prototypes */

xmlChar* getMinOccurs(xmlNodePtr node) ;
xmlChar* getMaxOccurs(xmlNodePtr node) ;
static void print_array_elements(void);
xmlChar* getElementName(xmlNodePtr node); 
xmlChar* getSchemaName(xmlNodePtr node);
xmlChar* getComplexTypeName(xmlNodePtr node) ;
xmlChar* getType(xmlNodePtr node) ;

int buildArrayTree(xmlChar* complexName, xmlChar* elemName, xmlChar* minO, xmlChar* maxO, xmlChar* type);

/* Array type - based on the min/max combinations following are the outcomes */

enum arraytype {
		SINGLE_ELEMENT,
	   	OPTIONAL_OR_ARRAY, 
		MANDATORY_AND_ARRAY,
};

/* Struct to hold elements which are defined as arrays from XSD 
 * 
 * TODO: For now it's in a struct - the primary reason for this is not to walk the XSD for each element
 * with in XML, find a better way of internal representation 
 */

struct xmlArrayDef {
		xmlChar* complexName;
		xmlChar* elemName;
		xmlChar* type;
		long minOccurs;
		int maxOccurs;
		enum arraytype isArray;
		struct xmlArrayDef *next ;
};

typedef struct xmlArrayDef *xmlArrayDefPtr ;

/* TODO: Replace global variables with local variables */

static xmlArrayDefPtr rootSchemaDetails = NULL; 
static void walkXsdSchema  (xmlNodePtr root);
char *nodeInfo ;
char *tempInfo; 
static char complexName[100];
static char elementName[100];
static char minO[100];
static char maxO[100];
static char type[100];


/* Walk the xsd schema and build a list of required name and properties */

static inline void walkXsdSchema(xmlNodePtr root)
{
    xmlNodePtr	    node;
    memset( complexName, '\0', sizeof(char)*100 );
    memset( elementName, '\0', sizeof(char)*100 );
    memset( minO, '\0', sizeof(char)*100 );
    memset( maxO, '\0', sizeof(char)*100 );
    memset( type, '\0', sizeof(char)*100 );

		for (node = root; node; node = node->next) {
			if (xmlStrEqual(node->name, (xmlChar *) "complexType")) {
					strcpy(complexName, (char*)getComplexTypeName(node));
			}
			if (xmlStrEqual(node->name, (xmlChar *) "schema")) {
			}
			if (xmlStrEqual(node->name, (xmlChar *) "element") && node->properties != NULL)  {
					strcpy(elementName, (char*)getElementName(node)) ;
					strcpy(minO, (char*)getMinOccurs(node));
					strcpy(maxO, (char*)getMaxOccurs(node));
					strcpy(type, (char*)getType(node));
			}

		if((xmlStrEqual(node->name, (xmlChar *) "element"))) {
			if(!(buildArrayTree((xmlChar*)complexName, (xmlChar*)elementName, (xmlChar*)minO, (xmlChar*)maxO, (xmlChar*)type))) 
					exit(0) ; 
		}
			walkXsdSchema(node->children);
	}

}

/* print the build list -- debuging */

static void print_array_elements() {

		xmlArrayDefPtr t ;

		for(t=rootSchemaDetails ; t ; t=t->next) 
				printf("%s -> %s [ %lu , %d ] %s, %d\n", t->complexName, t->elemName, t->minOccurs, t->maxOccurs, t->type, t->isArray);

}

