
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

#ifdef __cplusplus
extern "C" {
#endif


/* Structure to hold node name, isArray flag derived from min and max occurs from XSD 
 * There is no prev pointer as the search for the nodeName always start from root node 

TODO: Incorporate the type as well and pass it on to JsonObject  
TODO: Add macros to derive the isArray from min/max occurs 


struct xsdNodeInfo {
		xmlChar* nodeName ;
		int nodeType ;
		int minOccurs ;
		int maxOccurs ;
		int isArray ;
		struct xsdNodeInfo *next ;
};

typedef struct xsdNodeInfo* xsdNodeInfo ;


Allocate memory for struct and return pointer, if fails return -1 : caller has to free it 
TODO: Convert the mem and str functions from the xml2json library 

int xsdNodeInfo_init(xsdNodeInfo* n) {

		n = malloc(sizeof(struct xsdNodeInfo)) ;
		n->nodeName = malloc(sizeof(char)*50) ;
		n->minOccurs = 0 ;
		n->maxOccurs = 0 ;
		n->next = NULL ;
		return n;
}
 Populate the xsdNodeInfo struct from XSD, if not found return NULL */

int getNodeInfo(xmlNodePtr xsdroot, xmlNodePtr node);

int getNodeInfo(xmlNodePtr xsdroot, xmlNodePtr node)
{
	xmlNodePtr n = NULL ;

	if(xsdroot == NULL || node == NULL)
			return 0;

    for (n = xsdroot; n; n = n->next) {
            if (((n->properties != NULL)) && xmlStrEqual(n->name, node->name))	{
					return 1 ;
            }
            getNodeInfo(xsdroot, node->children);
    }
	return 0;
}
