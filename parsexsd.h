
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
extern void print_array_elements(void);
xmlChar* getElementName(xmlNodePtr node); 
xmlChar* getSchemaName(xmlNodePtr node);
xmlChar* getComplexTypeName(xmlNodePtr node) ;
xmlChar* getType(xmlNodePtr node) ;
extern int walkXsdSchema(xmlNodePtr root);
extern void xsdschemafree(void);

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

