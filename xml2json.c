/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#include "htable.h"
#include "json.h"
#include "util.h"

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
#include <libxml/xpath.h>

/**
 * Hashtable
 */
enum  xml_entry_type {
        ENTRY_TYPE_NULL,
        ENTRY_TYPE_BOOL,
        ENTRY_TYPE_STRING,
        ENTRY_TYPE_NUMBER,
        ENTRY_TYPE_ARRAY,
        ENTRY_TYPE_OBJECT,
};

struct xml_htable {
        struct htable table;
};

struct xml_htable_entry {
        struct htable_entry entry;
        enum xml_entry_type type;
        xmlChar *key;
        size_t keylen;
        void *value;
};

static struct xml_htable_entry *alloc_xml_htable_entry(const xmlChar *key,
                                                       size_t keylen,
                                                       void *value,
                                                       enum xml_entry_type type)
{
        struct xml_htable_entry *e;

        e = xmalloc(sizeof(struct xml_htable_entry));
        e->key = xmalloc(sizeof(xmlChar) * keylen);
        memcpy(e->key, key, keylen);
        e->keylen = keylen;
        e->value = value;
        e->type = type;

        return e;
}

static void free_xml_htable_entry(struct xml_htable_entry **e)
{
        if (e && *e) {
                free((*e)->key);
                (*e)->key = NULL;
                (*e)->keylen = 0;
                (*e)->value = NULL;
                free(*e);
                *e = NULL;
        }
}

static int xml_htable_entry_cmpfn(const void *unused1 _unused_,
                                  const void *entry1,
                                  const void *entry2,
                                  const void *unused2 _unused_)
{
        const struct xml_htable_entry *e1 = entry1;
        const struct xml_htable_entry *e2 = entry2;

        return memcmp_raw(e1->key, e1->keylen, e2->key, e2->keylen);
}

static void xml_htable_init(struct xml_htable *ht)
{
        htable_init(&ht->table, xml_htable_entry_cmpfn, NULL, 0);
}

static void *xml_htable_get(struct xml_htable *ht, xmlChar *key,
                            size_t keylen)
{
        struct xml_htable_entry k;
        struct xml_htable_entry *e;

        if (!ht->table.size)
                xml_htable_init(ht);

        htable_entry_init(&k, bufhash(key, keylen));
        k.key = (xmlChar *)key;
        k.keylen = keylen;
        e = htable_get(&ht->table, &k, NULL);

        return e ? e : NULL;
}

static void xml_htable_put(struct xml_htable *ht,
                           const xmlChar *key,
                           size_t keylen, void *value,
                           enum xml_entry_type type)
{
        struct xml_htable_entry *e;

        if (!ht->table.size)
                xml_htable_init(ht);

        e = alloc_xml_htable_entry(key, keylen, value, type);
        htable_entry_init(e, bufhash(key, keylen));

        htable_put(&ht->table, e);
}

static void *xml_htable_remove(struct xml_htable *ht, xmlChar *key,
                               size_t keylen)
{
        struct xml_htable_entry e;

        if (!ht->table.size)
                xml_htable_init(ht);

        htable_entry_init(&e, bufhash(key, keylen));

        return htable_remove(&ht->table, &e, key);
}

static void xml_htable_free(struct xml_htable *ht)
{
        struct htable_iter iter;
        struct xml_htable_entry *e;

        htable_iter_init(&ht->table, &iter);
        while ((e = htable_iter_next(&iter))) {
                free_xml_htable_entry(&e);
        }

        htable_free(&ht->table, 0);
}

/**
 * XML parsing
 */

#define INDENT_SIZE 101

typedef struct _xml2jsonCtxt xml2jsonCtxt;
typedef xml2jsonCtxt *xml2jsonCtxtPtr;
struct _xml2jsonCtxt {
        FILE *output;
        char indent[INDENT_SIZE];
        int depth;
        int level;
        xmlDocPtr doc;
        xmlNodePtr node;
        xmlDictPtr dict;
        xmlXPathContextPtr pctxt;
        int options;
};

static void xml2jsonCtxtParseNodeList(xml2jsonCtxtPtr ctxt, xmlNodePtr node);

static void xml2jsonInitCtxt(xml2jsonCtxtPtr ctxt)
{
        int i;
        ctxt->output = stdout;
        ctxt->depth = 0;
        ctxt->level = 0;
        ctxt->doc = NULL;
        ctxt->node = NULL;
        ctxt->dict = NULL;
        ctxt->options = 0;
        for (i = 0; i < INDENT_SIZE; i++)
                ctxt->indent[i] = ' ';
        ctxt->indent[INDENT_SIZE] = 0;
}

static void xml2jsonCleanCtxt(xml2jsonCtxtPtr ctxt _unused_)
{
}

#if 0
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
#endif

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
                /* xml2jsonPrintSpaces(ctxt); */
                if ((node->ns != NULL) && (node->ns->prefix != NULL))  {
                        xml2jsonPrintString(ctxt, node->ns->prefix);
                        fprintf(ctxt->output, ":");
                }
                fprintf(ctxt->output, "EN: ");
                xml2jsonPrintString(ctxt, node->name);
                fprintf(ctxt->output, "\n");
                break;
        case XML_ATTRIBUTE_NODE:
                fprintf(ctxt->output, "AN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_TEXT_NODE:
                fprintf(ctxt->output, "TN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_CDATA_SECTION_NODE:
                fprintf(ctxt->output, "CSN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_ENTITY_REF_NODE:
                fprintf(ctxt->output, "ERN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_ENTITY_NODE:
                fprintf(ctxt->output, "ENN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_PI_NODE:
                fprintf(ctxt->output, "PIN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_COMMENT_NODE:
                fprintf(ctxt->output, "CON: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
                fprintf(ctxt->output, "DOCN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_DOCUMENT_TYPE_NODE:
                fprintf(ctxt->output, "DTN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_DOCUMENT_FRAG_NODE:
                fprintf(ctxt->output, "DFN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_NOTATION_NODE:
                fprintf(ctxt->output, "NN: ");
                fprintf(ctxt->output, "\n");
                break;
        case XML_DTD_NODE:
                fprintf(ctxt->output, "DTDN: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_ELEMENT_DECL:
                fprintf(ctxt->output, "ELDECL: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_ATTRIBUTE_DECL:
                fprintf(ctxt->output, "ADECL: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_ENTITY_DECL:
                fprintf(ctxt->output, "ENDECL: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_NAMESPACE_DECL:
                fprintf(ctxt->output, "NDECL: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_XINCLUDE_START:
                fprintf(ctxt->output, "XIS: ");
                fprintf(ctxt->output, "\n");
                return;
        case XML_XINCLUDE_END:
                fprintf(ctxt->output, "XIE: ");
                fprintf(ctxt->output, "\n");
                return;
        default:
                fprintf(stderr, "Unknown node type %d\n", node->type);
                return;
        } /* switch() */

        ctxt->depth++;
        fprintf(ctxt->output, ">>> %d\n", ctxt->depth);

        if ((node->type == XML_ELEMENT_NODE) && (node->nsDef != NULL))
                xml2jsonPrintNameSpaceList(ctxt, node->nsDef);

        if ((node->type == XML_ELEMENT_NODE) && (node->properties != NULL))
                xml2jsonPrintAttrList(ctxt, node->properties);

        if (node->type != XML_ENTITY_REF_NODE) {
                if ((node->type != XML_ELEMENT_NODE) &&
                    (node->content != NULL)) {
                        /* xml2jsonPrintSpaces(ctxt); */
                        fprintf(ctxt->output, "CONTENT: ");
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
        fprintf(ctxt->output, ">>> %d\n", ctxt->depth);
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
                fprintf(ctxt->output, ">> %d\n", ctxt->depth);
                xml2jsonCtxtParseNodeList(ctxt, node->children);
                ctxt->depth--;
                fprintf(ctxt->output, "<< %d\n", ctxt->depth);
        }
}

static void xml2jsonCtxtParseNodeList(xml2jsonCtxtPtr ctxt, xmlNodePtr node)
{
        while (node != NULL) {
                xml2jsonCtxtParseNode(ctxt, node);
                node = node->next;
        }
}

#if 0
static void xml2jsonCtxtParse(xml2jsonCtxtPtr ctxt, xmlDocPtr doc)
{
        if (doc == NULL)
                return;

        ctxt->doc = doc;

        ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
        if (ctxt->pctxt == NULL) {
                xmlFree(ctxt);
                return;
        }

        /* XXX: JSON ROOT HERE */
        if ((doc->type == XML_DOCUMENT_NODE) && (doc->children != NULL)) {
                ctxt->depth++;
                fprintf(ctxt->output, "> %d\n", ctxt->depth);
                xml2jsonCtxtParseNodeList(ctxt, doc->children);
                ctxt->depth--;
                fprintf(ctxt->output, "> %d\n", ctxt->depth);
        }

        xmlXPathFreeContext(ctxt->pctxt);
}
#endif

#if 0
static void to_json(xmlDocPtr doc)
{
        xml2jsonCtxt ctxt;

        xml2jsonInitCtxt(&ctxt);

        xml2jsonCtxtParse(&ctxt, doc);

        xml2jsonCleanCtxt(&ctxt);
}
#endif

static void *parse_xmlnode_children(xml2jsonCtxtPtr ctxt,
                                    xmlNodePtr node,
                                    enum xml_entry_type *type)
{
        xmlNodePtr tmp;
        struct xml_htable ht;
        struct htable_iter iter;
        struct xml_htable_entry *e;
        JsonObject *jobj = json_new();

        if (node == NULL)
                return NULL;

        xml_htable_init(&ht);
        ctxt->depth++;
        for (tmp = node; tmp; tmp = tmp->next) {
                if (tmp->type == XML_ELEMENT_NODE) {
                        void *val;
                        val = parse_xmlnode_children(ctxt, tmp->children, type);
                        xml_htable_put(&ht, tmp->name, xmlStrlen(tmp->name),
                                       val, *type);
                }
                if (tmp->type == XML_TEXT_NODE) {
                        xmlChar *s = xmlNodeGetContent(tmp);
                        int len = xmlStrlen(s), t, spaces;
                        spaces = len;

                        /* Check if the string is just empty spaces
                         * in which case we just ignore it.
                         */
                        for (t = 0; t < len; t++) {
                                if ((s[t] == 0x20) ||
                                    ((0x9 <= s[t]) && (s[t] <= 0xa)) ||
                                    (s[t] == 0xd) ||
                                    (s[t] == '\r') ||
                                    (s[t] == '\n') ||
                                    (s[t] == 0x0a)) {
                                        spaces--;
                                        continue;
                                }
                        }

                        if (spaces == 0) {
                                xmlFree(s);
                                continue;
                        }
                        /* XXX: Find the correct type here */
                        *type = ENTRY_TYPE_STRING;
                        return s;
                }
        }


        htable_iter_init_ordered(&ht.table, &iter);
        while ((e = htable_iter_next_ordered(&iter))) {
                if (e->entry.count > 1) { /* Array */
                        JsonObject *array;
                        struct xml_htable_entry *temp;
                        array = json_array_obj();
                        temp = e;
                        json_prepend_to_array(array,
                                              json_string_obj((char *)e->value));
                        while ((temp = htable_get_next(&ht.table, temp))) {
                                json_prepend_to_array(array,
                                                      json_string_obj((char *)temp->value));
                        }
                        json_append_member(jobj, (const char *)e->key, array);
                } else {        /* normal(?) object */
                        if (e->type != ENTRY_TYPE_STRING) {
                                json_append_member(jobj, (const char *)e->key, e->value);
                        } else {
                                JsonObject *strobj;
                                strobj = json_string_obj(e->value);
                                json_append_member(jobj, (const char *)e->key, strobj);
                        }
                }
        }

        xml_htable_free(&ht);
        *type = ENTRY_TYPE_OBJECT;
        return jobj;
        ctxt->depth--;
}

static void xml2json_ctx_parse(xml2jsonCtxtPtr ctxt, xmlDocPtr doc)
{
        enum xml_entry_type type;
        void *data;
        if (doc == NULL)
                return;

        ctxt->doc = doc;

        ctxt->pctxt = xmlXPathNewContext(ctxt->doc);
        if (ctxt->pctxt == NULL) {
                xmlFree(ctxt);
                return;
        }

        if ((doc->type == XML_DOCUMENT_NODE) && (doc->children != NULL)) {
                data = parse_xmlnode_children(ctxt, doc->children, &type);
                printf("--- %s ---\n", json_encode((JsonObject *)data));
                json_free(data);
        }

        xmlXPathFreeContext(ctxt->pctxt);
}

static void parse_xml_tree(xmlDocPtr doc)
{
        xml2jsonCtxt ctxt;

        xml2jsonInitCtxt(&ctxt);

        xml2json_ctx_parse(&ctxt, doc);

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

        /* to_json(doc); */
        parse_xml_tree(doc);

        xmlFreeDoc(doc);

        {
                JsonObject *array;
                JsonObject *children[5 + 1];
                JsonObject *strobj;
                jsondoc = json_new();

                array = json_array_obj();

                children[1] = json_num_obj(1);
                children[2] = json_num_obj(2);
                children[3] = json_num_obj(3);
                json_append_to_array(array, children[1]);
                json_append_to_array(array, children[3]);
                json_append_to_array(array, children[2]);

                strobj = json_string_obj("sigh");
                json_append_member(jsondoc, "elements", array);
                json_append_member(jsondoc, "another", strobj);

                printf(">> %s\n", json_encode(jsondoc));
                json_free(jsondoc);
        }

        exit(EXIT_SUCCESS);
}

