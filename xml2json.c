/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */
#define _unused_          __attribute__((unused))

#include "json.h"

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/chvalid.h>

#define INDENT_SIZE 101

typedef struct _xml2jsonCtxt xml2jsonCtxt;
typedef xml2jsonCtxt *xml2jsonCtxtPtr;
struct _xml2jsonCtxt {
        FILE *output;
        char indent[INDENT_SIZE];
        int depth;
        xmlDocPtr doc;
        xmlNodePtr node;
        xmlDictPtr dict;
        int options;
};

static void xml2jsonCtxtParseNodeList(xml2jsonCtxtPtr ctxt, xmlNodePtr node);

static void xml2jsonInitCtxt(xml2jsonCtxtPtr ctxt)
{
        ctxt->output = stdout;
        ctxt->depth = 0;
        ctxt->doc = NULL;
        ctxt->node = NULL;
        ctxt->dict = NULL;
        ctxt->options = 0;
        memset(ctxt->indent, ' ', INDENT_SIZE - 1);
        ctxt->indent[INDENT_SIZE] = 0;
}

static void xml2jsonCleanCtxt(xml2jsonCtxtPtr ctxt _unused_)
{
}

static void xml2jsonPrintSpaces(xml2jsonCtxtPtr ctxt)
{
        if ((ctxt->output != NULL) && (ctxt->depth > 0)) {
                if (ctxt->depth < 50)
                        fprintf(ctxt->output, "%s",
                                &ctxt->indent[INDENT_SIZE - 2 *ctxt->depth]);
                else
                        fprintf(ctxt->output, "%s", ctxt->indent);
        }
}

static void xml2jsonPrintString(xml2jsonCtxtPtr ctxt, const xmlChar * str)
{
        int i;

        if (str == NULL) {
                fprintf(ctxt->output, "()");
                return;
        }

        for (i = 0; i < xmlStrlen(str); i++) {
                if (str[i] == 0)
                        return;
                else if (xmlIsBlank_ch(str[i]))
                        fputc(' ', ctxt->output);
                else if (str[i] >= 0x80)
                        fprintf(ctxt->output, "#%X", str[i]);
                else
                        fputc(str[i], ctxt->output);
        }
}

static void xml2jsonPrintNameSpaceList(xml2jsonCtxtPtr ctxt, xmlNsPtr ns)
{
}


static void xml2jsonPrintAttrList(xml2jsonCtxtPtr ctxt, xmlAttrPtr attr)
{
}

static void xml2jsonPrintEntity(xml2jsonCtxtPtr ctxt, xmlEntityPtr ent)
{
}

static void xml2jsonCtxtParseOneNode(xml2jsonCtxtPtr ctxt, xmlNodePtr node)
{
        if (node == NULL)
                return;

        ctxt->node = node;

        switch(node->type) {
        case XML_ELEMENT_NODE:
                xml2jsonPrintSpaces(ctxt);
                if ((node->ns != NULL) && (node->ns->prefix != NULL))  {
                        fprintf(ctxt->output, ":");
                }
                xml2jsonPrintString(ctxt, node->name);
                fprintf(ctxt->output, "\n");
                break;
        case XML_ATTRIBUTE_NODE:
                break;
        case XML_TEXT_NODE:
                break;
        case XML_CDATA_SECTION_NODE:
                break;
        case XML_ENTITY_REF_NODE:
                break;
        case XML_ENTITY_NODE:
                break;
        case XML_PI_NODE:
                break;
        case XML_COMMENT_NODE:
                break;
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
                break;
        case XML_DOCUMENT_TYPE_NODE:
                break;
        case XML_DOCUMENT_FRAG_NODE:
                break;
        case XML_NOTATION_NODE:
                break;
        case XML_DTD_NODE:
                return;
        case XML_ELEMENT_DECL:
                return;
        case XML_ATTRIBUTE_DECL:
                return;
        case XML_ENTITY_DECL:
                return;
        case XML_NAMESPACE_DECL:
                return;
        case XML_XINCLUDE_START:
                return;
        case XML_XINCLUDE_END:
                return;
        default:
                fprintf(stderr, "Unknown node type %d\n", node->type);
                return;
        } /* switch() */

        ctxt->depth++;

        if ((node->type == XML_ELEMENT_NODE) && (node->nsDef != NULL))
                xml2jsonPrintNameSpaceList(ctxt, node->nsDef);

        if ((node->type == XML_ELEMENT_NODE) && (node->properties != NULL))
                xml2jsonPrintAttrList(ctxt, node->properties);

        if (node->type != XML_ENTITY_REF_NODE) {
                if ((node->type != XML_ELEMENT_NODE) &&
                    (node->content != NULL)) {
                        xml2jsonPrintSpaces(ctxt);
                        xml2jsonPrintString(ctxt, node->content);
                        fprintf(ctxt->output, "\n");
                }
        } else {
                xmlEntityPtr ent;

                ent = xmlGetDocEntity(node->doc, node->name);
                if (ent != NULL)
                        xml2jsonPrintEntity(ctxt, ent);
        }

        ctxt->depth--;

}

static void xml2jsonCtxtParseNode(xml2jsonCtxtPtr ctxt, xmlNodePtr node)
{
        if (node == NULL)
                return;

        xml2jsonCtxtParseOneNode(ctxt, node);

        if ((node->type != XML_NAMESPACE_DECL) &&
            (node->children != NULL) &&
            (node->type != XML_ENTITY_REF_NODE)) {
                ctxt->depth++;
                xml2jsonCtxtParseNodeList(ctxt, node);
                ctxt->depth--;
        }
}

static void xml2jsonCtxtParseNodeList(xml2jsonCtxtPtr ctxt, xmlNodePtr node)
{
        while (node != NULL) {
                xml2jsonCtxtParseNode(ctxt, node);
                node = node->next;
        }
}

static void xml2jsonCtxtParse(xml2jsonCtxtPtr ctxt, xmlDocPtr doc)
{
        if (doc == NULL)
                return;

        if ((doc->type == XML_DOCUMENT_NODE) && (doc->children != NULL)) {
                ctxt->depth++;
                xml2jsonCtxtParseNodeList(ctxt, doc->children);
                ctxt->depth--;
        }
}

static void to_json(xmlDocPtr doc)
{
        xml2jsonCtxt ctxt;

        xml2jsonInitCtxt(&ctxt);

        xml2jsonCtxtParse(&ctxt, doc);

        xml2jsonCleanCtxt(&ctxt);
}

int main(int argc, char **argv)
{
        int fd;
        struct stat sbinfo;
        xmlDocPtr doc = NULL;
        const char *base;
        int options = XML_PARSE_COMPACT | XML_PARSE_BIG_LINES;
        JsonObject *jsondoc;

        if (argc != 2) {
                fprintf(stderr, "%s <xml-file>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        /* mmap the file() */
        if (stat(argv[1], &sbinfo) < 0) {
                perror("stat: ");
                exit(EXIT_FAILURE);
        }

        if ((fd = open(argv[1], O_RDONLY)) < 0) {
                perror("open: ");
                exit(EXIT_FAILURE);
        }

        base = mmap(NULL, sbinfo.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (base == (void *) MAP_FAILED){
                close(fd);
                perror("mmap: ");
                exit(EXIT_FAILURE);
        }

        /* Read into an xmlDocPtr */
        doc = xmlReadMemory((char *) base, sbinfo.st_size, argv[1],
                            NULL, options);

        munmap((char *)base, sbinfo.st_size);
        close(fd);

        to_json(doc);

        xmlFreeDoc(doc);

        jsondoc = json_new();

        json_free(jsondoc);

        exit(EXIT_SUCCESS);
}

