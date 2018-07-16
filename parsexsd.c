
/* parsexsd.c : Walk the xsd tree as provided by libxml
 *
 *
 * Copyright (c) 2018 Ram Gopalkrisha ramkumarg1@gmail.com */

#ifndef XML2JSON_XSD_C
#define XML2JSON_XSD_C
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>

#include "parsexsd.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Walk the XSD tree and build a list of all the elements that are defined as arrayrs in xsd 
 *
 * complexName - xsd Complex type name
 * elemName - xsd Element name
 * minO - minimum occurence
 * maxO - maximum occurence
 */

int buildArrayTree(xmlChar* complexNamein, xmlChar* elemName, xmlChar* minOin, xmlChar* maxOin, xmlChar* typein) {

		xmlArrayDefPtr t = malloc(sizeof(struct xmlArrayDef)) ;
		t->elemName = malloc(sizeof(char)*100);
		t->complexName = malloc(sizeof(char)*100);
		t->type = malloc(sizeof(char)*100);
		t->minOccurs = -1 ;
		t->maxOccurs = 0 ;
		t->isArray = 0 ;
		t->next = NULL ;
		xmlArrayDefPtr r = NULL; 

		strcpy((char*)t->complexName, (char*)complexNamein);
		strcpy((char*)t->elemName, (char*)elemName);
		strcpy((char*)t->type, (char*)typein);

		if(strlen((char*)complexName) == 0) strcpy((char*)complexName, "root");

		if(strlen((char*)minOin) == 0) strcpy((char*)minOin, "0");
		t->minOccurs = strtol((char*)minOin,NULL,10);
		if (errno == EINVAL) {
				printf("Conversion error occured for minOin");
				return 0;
		}

		if(strlen((char*)maxOin) == 0) strcpy((char*)maxOin, "0");

		/* -99 is an arbitary negative number, as XSD comes with "unbound", the size of array is platform dependent
		 * also, this number has little or no impact in conversion" */
		if(xmlStrEqual(maxOin, (xmlChar*)"unbounded"))  strcpy((char*)maxOin, "-99"); 

		t->maxOccurs = strtol((char*)maxOin,NULL,10);
		if (errno == EINVAL) {
				printf("Conversion error occured for minOin");
				return 0;
		}

		/* minOccurs = 0 && maxOccurs > 1 - optional or an array 
		   minOccurs = +{d,>1}  - mandatory and an array 
		   minOccurs > 1 && maxOccurs > 1 - mandatory and array */

		if(t->minOccurs == 0 && t->maxOccurs > 1)
				t->isArray = OPTIONAL_OR_ARRAY ;

		if((t->minOccurs > 1) || ( t->minOccurs > 1 && t->maxOccurs > 1))
				t->isArray = MANDATORY_AND_ARRAY ;

		if(rootSchemaDetails == NULL) {
				rootSchemaDetails = t;
				return 1;
		} else {
				for(r=rootSchemaDetails ; r->next ; r=r->next);
				r->next=t ;
				return 1;
		}
}

xmlChar* getType(xmlNodePtr node) {

		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((xmlChar*)"type",anode->name)) && anode->next) 
				anode=anode->next ;

		if(xmlStrEqual((xmlChar*)"type",anode->name) && anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;

		return (xmlChar*)"\0";
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

		return (xmlChar*)"\0" ;
}

/* Given a node, get the schema name */

xmlChar* getSchemaName(xmlNodePtr node) {

		if(node->properties->children != NULL)
				return (xmlChar*)node->properties->children->content ;

		return (xmlChar*)"\0" ;
}

/* Given a node, get the element name assosiated with it by traversing to child or next */

xmlChar* getElementName(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"name",anode->name)) && anode->next) 
				anode=anode->next ;

		if(anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;

		return (xmlChar*)"\0" ;
}

/* Given a node, get min occurs property */

xmlChar* getMinOccurs(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"minOccurs",anode->name)) && anode->next) 
				anode=anode->next ;

		if(xmlStrEqual((const xmlChar*)"minOccurs",anode->name) && anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;

		return (xmlChar*)"\0" ;

}

/* Given a node, get max occurs property */

xmlChar* getMaxOccurs(xmlNodePtr node) {
		xmlAttrPtr anode = node->properties ;

		while(!(xmlStrEqual((const xmlChar*)"maxOccurs",anode->name)) && anode->next) 
				anode=anode->next ;

		if(xmlStrEqual((const xmlChar*)"maxOccurs",anode->name) && anode != NULL && anode->children != NULL) 
				return (xmlChar*) anode->children->content ;
		else
				return (xmlChar*)"\0" ;

		return (xmlChar*)"\0" ;
}
