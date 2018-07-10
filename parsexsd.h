
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
void print_array_elements(void);
xmlChar* getElementName(xmlNodePtr node); 
xmlChar* getSchemaName(xmlNodePtr node);
xmlChar* getComplexTypeName(xmlNodePtr node) ;

/* Struct to hold elements which are defined as arrays from XSD 
 * 
 * TODO: For now it's in a struct - the primary reason for this is not to walk the XSD for each element
 * with in XML, find a better way of internal representation 
 */


struct xmlArrayDef {
		xmlChar* complexName;
		xmlChar* elemName;
		long minOccurs;
		int maxOccurs;
		struct xmlArrayDef *next ;
};

typedef struct xmlArrayDef *xmlArrayDefPtr ;

/* TODO: Replace global variables with local variables */

xmlArrayDefPtr rootSchemaDetails = NULL; 
void walkXsdSchema  (xmlNodePtr root);
char *nodeInfo ;
char *tempInfo; 
static char complexName[100];
static char elementName[100];
static char minO[100];
static char maxO[100];
static xmlChar* nullstring ; 

/* Walk the XSD tree and build a list of all the elements that are defined as arrayrs in xsd 
 *
 * complexName - xsd Complex type name
 * elemName - xsd Element name
 * minO - minimum occurence
 * maxO - maximum occurence
 */


static int buildArrayTree(xmlChar* acomplexName, xmlChar* elemName, xmlChar* aminO, xmlChar* amaxO) {

		xmlArrayDefPtr t = malloc(sizeof(struct xmlArrayDef)) ;
		t->elemName = malloc(sizeof(char)*100);
		t->complexName = malloc(sizeof(char)*100);
		t->minOccurs = -1 ;
		t->maxOccurs = -1 ;
		t->next = NULL ;
		xmlArrayDefPtr r = NULL; 

		if((strlen((char*)acomplexName) == 0) || (strlen((char*)elemName) == 0) 
						|| (strlen((char*)aminO) == 0) || (strlen((char*)amaxO) == 0)) {
				printf("BuildArrayTree : Cannot send empty values");
				return 0;
		}
		strcpy((char*)t->complexName, (char*)acomplexName);
		strcpy((char*)t->elemName, (char*)elemName);
		t->minOccurs = strtol((char*)aminO,NULL,10);
		if (errno == EINVAL) {
				printf("Conversion error occured for minO");
				return 0;
		}

		if(xmlStrEqual(amaxO, (const xmlChar*)"unbounded")) 
				t->maxOccurs = -1 ;
		else {
				t->maxOccurs = strtol((char*)amaxO,NULL,10);
				if (errno == EINVAL) {
						printf("Conversion error occured for maxO");
						return 0;
				}
		}

		if(rootSchemaDetails == NULL) {
				rootSchemaDetails = t;
				return 1;
		} else {
				for(r=rootSchemaDetails ; r->next ; r=r->next);
				r->next=t ;
				return 1;
		}
}

/* Given a node, get the complex type assosiated with it by traversing to parent or previous */

xmlChar* getComplexTypeName(xmlNodePtr node) {

		xmlNodePtr tmp2;

		for(tmp2=node ; tmp2 ; tmp2=tmp2->parent) {
				if(xmlStrEqual((const xmlChar*)"element",tmp2->name) && 
								(tmp2->properties != NULL) && (tmp2->properties->children != NULL))
						return (xmlChar*)tmp2->properties->children->content;
		}

		for(tmp2=node ; tmp2 ; tmp2=tmp2->prev) {
				if(xmlStrEqual((const xmlChar*)"element",tmp2->name) && 
								(tmp2->properties != NULL) && (tmp2->properties->children != NULL))
						return (xmlChar*)tmp2->properties->children->content;
		}

		return nullstring ;
}

/* Given a node, get the schema name */

xmlChar* getSchemaName(xmlNodePtr node) {

		if(node->properties->children != NULL)
				return (xmlChar*)node->properties->children->content ;

		return nullstring;
}

/* Given a node, get the element name assosiated with it by traversing to child or next */

xmlChar* getElementName(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"name",anode->name)) && anode->next) 
				anode=anode->next ;

		if(anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;

		return nullstring;
}

/* Given a node, get min occurs property */

xmlChar* getMinOccurs(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"minOccurs",anode->name)) && anode->next) 
				anode=anode->next ;

		if(xmlStrEqual((const xmlChar*)"minOccurs",anode->name) && anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;

		return nullstring;

}

/* Given a node, get max occurs property */

xmlChar* getMaxOccurs(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"maxOccurs",anode->name)) && anode->next) 
				anode=anode->next ;

		if(xmlStrEqual((const xmlChar*)"maxOccurs",anode->name) && anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;
		else
				return nullstring;

		return nullstring;
}
void

/* Walk the xsd schema and build a list of required name and properties */

walkXsdSchema(xmlNodePtr root)
{
    xmlNodePtr	    node;
    memset( complexName, '\0', sizeof(char)*100 );
    memset( elementName, '\0', sizeof(char)*100 );
    memset( minO, '\0', sizeof(char)*100 );
    memset( maxO, '\0', sizeof(char)*100 );

    nullstring = malloc(sizeof(xmlChar)*10);
    memset( nullstring, '\0', sizeof(char)*10 );
	strcpy(nullstring, (xmlChar*)"\0");

    for (node = root; node; node = node->next) {
			if (xmlStrEqual(node->name, (const xmlChar *) "complexType")) {
					strcpy(complexName, (char*)getComplexTypeName(node));
			}
			if (xmlStrEqual(node->name, (const xmlChar *) "schema")) {
			}
			if (xmlStrEqual(node->name, (const xmlChar *) "element") && node->properties != NULL)  {
					strcpy(elementName, (char*)getElementName(node)) ;
					strcpy(minO, (char*)getMinOccurs(node));
					strcpy(maxO, (char*)getMaxOccurs(node));
			}

			if((strlen(maxO) > 0) && (xmlStrEqual(node->name, (const xmlChar *) "element"))) {
					if(strlen((char*)complexName) == 0)
							strcpy(complexName, "root");
					if((strlen((char*)minO) == 0) && (strlen((char*)maxO)> 0)) 
							strcpy(minO, "0");
					if((strlen((char*)maxO) == 0) && (strlen((char*)minO)> 0)) 
							strcpy(maxO, "unbounded");
					if(!(buildArrayTree((xmlChar*)complexName, (xmlChar*)elementName, (xmlChar*)minO, (xmlChar*)maxO))) 
							exit(0) ; 
			}
			walkXsdSchema(node->children);
	}
}

/* print the build list -- debuging */

void print_array_elements() {

		xmlArrayDefPtr t ;

		for(t=rootSchemaDetails ; t ; t=t->next) 
				printf("%s -> %s [ %lu , %d ]\n", t->complexName, t->elemName, t->minOccurs, t->maxOccurs);
}

