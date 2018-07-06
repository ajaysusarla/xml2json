/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#define LIBXML_SCHEMAS_ENABLED
#include "cstring.h"
#include "htable.h"
#include "json.h"
#include "util.h"
#include "parsexsd.h"

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <getopt.h>

#include <libxml/parser.h>
#include <libxml/chvalid.h>
#include <libxml/xpath.h>
#include <libxml/xmlschemastypes.h>
#include <libxml/schemasInternals.h>

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
        char *key;
        size_t keylen;
        void *value;
};

static struct xml_htable_entry *alloc_xml_htable_entry(char *key,
                                                       size_t keylen,
                                                       void *value,
                                                       enum xml_entry_type type)
{
        struct xml_htable_entry *e;

        e = xmalloc(sizeof(struct xml_htable_entry));
        e->key = xcalloc(1, keylen);
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
                if ((*e)->type == ENTRY_TYPE_STRING) {
                        free((*e)->value);
                }

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

static void *xml_htable_get(struct xml_htable *ht, char *key,
                            size_t keylen)
{
        struct xml_htable_entry k;
        struct xml_htable_entry *e;

        if (!ht->table.size)
                xml_htable_init(ht);

        htable_entry_init(&k, bufhash(key, keylen));
        k.key = key;
        k.keylen = keylen;
        e = htable_get(&ht->table, &k, NULL);

        return e ? e : NULL;
}

static void xml_htable_put(struct xml_htable *ht,
                           char *key,
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

static void *xml_htable_remove(struct xml_htable *ht, char *key,
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
static void *parse_xmlnode(xmlNodePtr node, enum xml_entry_type *type,
                           xmlNodePtr xsdroot);

static void *parse_xml_element_attributes(xmlAttrPtr attr, void *attrobj,
                                          enum xml_entry_type *type,
                                          xmlNodePtr xsdroot)
{
        if (attr == NULL) {
                *type = ENTRY_TYPE_NULL;
                return NULL;
        }

        if (attrobj == NULL)
                attrobj = json_new();

        while (attr != NULL) {
                cstring str;
                void *val;

                /* Append '@' to the attribute name */
                cstring_init(&str, 0);
                cstring_addch(&str, '@');
                cstring_addstr(&str, (char *)attr->name);

                val = parse_xmlnode(attr->children, type, xsdroot);

                if (*type == ENTRY_TYPE_STRING) {
                        JsonObject *strobj;
                        strobj = json_string_obj(val);
                        free(val);
                        json_prepend_member(attrobj, str.buf, strobj);
                } else {
                        printf("attributes: non string type entry!\n");
                }

                attr = attr->next;

                cstring_release(&str);
        }

        *type = ENTRY_TYPE_OBJECT;
        return attrobj;
}

static int parse_xml_element_node(xmlNodePtr node, struct xml_htable *ht,
                                    enum xml_entry_type *type, xmlNodePtr xsdroot)
{
        void *val = NULL;
        int has_attr = 0;
        void *attrval = NULL;
        JsonObject *attrobj = NULL;

        if (node == NULL) {
                *type = ENTRY_TYPE_NULL;
                return -1;
        }

        val = parse_xmlnode(node->children, type, xsdroot);
        switch (*type) {
        case ENTRY_TYPE_NULL:
                xfree(val);
                val = NULL;
                break;
        case ENTRY_TYPE_STRING:
                if (node->properties) {
                        attrobj = json_new();
                        json_prepend_member(attrobj, "#text",
                                            json_string_obj(val));
                        free(val);
                        val = attrobj;
                }
                break;
        case ENTRY_TYPE_OBJECT:
        case ENTRY_TYPE_ARRAY:
        case ENTRY_TYPE_BOOL:
        case ENTRY_TYPE_NUMBER:
        default:
                break;
        }

        if (node->properties != NULL) {
                /* We need to parse XML attributes */

                attrval = parse_xml_element_attributes(node->properties, val,
                                                       type, xsdroot);
                if (val == NULL)
                        val = attrval;
                has_attr = 1;
        }

        xml_htable_put(ht, (char *)node->name, xmlStrlen(node->name),
                       val, *type);

        return has_attr;
}

static char *parse_xml_text_node(xmlNodePtr node, enum xml_entry_type *type,
                                 size_t *slen, xmlNodePtr xsdroot)
{
        xmlChar *content;
        cstring str;
        size_t len = 0, i = 0;

        if (node->content == NULL)
                return NULL;

        cstring_init(&str, 0);

        content = xmlNodeGetContent(node);
        len = xmlStrlen(content);

        for (i = 0; i < len; i++) {
                if ((content[i] == 0x20) ||
                    ((0x9 <= content[i]) && (content[i] <= 0xa)) ||
                    (content[i] == 0xd) ||
                    (content[i] == '\r') ||
                    (content[i] == '\n') ||
                    (content[i] == 0x0a)) {
                        continue;
                }
                cstring_addch(&str, content[i]);
        }

        if (!content) *type = ENTRY_TYPE_NULL;

        *type = getNodeInfo(node, xsdroot) ;

        xmlFree(content);

        if (str.len == 0) {
                *type = ENTRY_TYPE_NULL;
                cstring_release(&str);
        } else {
                *type = ENTRY_TYPE_STRING;
        }

        return cstring_detach(&str, slen);
}

static JsonObject *xml_htable_to_json_obj(struct xml_htable *ht)
{
        JsonObject *jobj;
        struct htable_iter iter;
        struct xml_htable_entry *e = NULL;

        jobj = json_new();

        htable_iter_init_ordered(&ht->table, &iter);

        while ((e = htable_iter_next_ordered(&iter))) {
                if (e->entry.count > 1) { /* Array */
                        JsonObject *array;
                        struct xml_htable_entry *temp = NULL;
                        temp = e;

                        array = json_array_obj();

                        if (e->type == ENTRY_TYPE_NULL)
                                json_prepend_to_array(array, json_null_obj());
                        else if (e->type == ENTRY_TYPE_STRING)
                                json_prepend_to_array(array,
                                                      json_string_obj(e->value));
                        else
                                json_prepend_to_array(array, e->value);


                        /* Parse the other entries for the same key */
                        while ((temp = htable_get_next(&ht->table, temp))) {
                                if (temp->type == ENTRY_TYPE_NULL)
                                        json_prepend_to_array(array,
                                                              json_null_obj());
                                else if (temp->type == ENTRY_TYPE_STRING)
                                        json_prepend_to_array(array,
                                                              json_string_obj(temp->value));
                                else
                                        json_prepend_to_array(array, temp->value);
                        }

                        json_append_member(jobj, (char *)e->key, array);

                } else {                  /* Normal(?) non-array object */
                        if (e->type == ENTRY_TYPE_NULL)
                                json_append_member(jobj, e->key,
                                                   json_null_obj());
                        else if (e->type == ENTRY_TYPE_STRING)
                                json_append_member(jobj, e->key,
                                                   json_string_obj(e->value));
                        else
                                json_append_member(jobj, e->key, e->value);
                }
        }

        return jobj;
}

static void *parse_xmlnode(xmlNodePtr node, enum xml_entry_type *type,
                           xmlNodePtr xsdroot)
{
        struct xml_htable ht;
        xmlNodePtr n;
        JsonObject *jobj;

        if (node == NULL) {
                *type = ENTRY_TYPE_NULL;
                return NULL;
        }

        /* Initialise a ordered hash table */
        xml_htable_init(&ht);

        for (n = node; n; n = n->next) {
                void *val;
                size_t slen = 0;

                switch(n->type) {
                case XML_ELEMENT_NODE:
                        parse_xml_element_node(n, &ht, type, xsdroot);
                        break;
                case XML_TEXT_NODE:
                        val = parse_xml_text_node(n, type, &slen, xsdroot);
                        if (slen == 0) continue;
                        xml_htable_free(&ht);
                        return val;
                default:
                        break;
                }
        }

        /* If we've got here, we are returning a json object */
        *type = ENTRY_TYPE_OBJECT;
        jobj = xml_htable_to_json_obj(&ht);

        /* Free the ordered hash table */
        xml_htable_free(&ht);

        return jobj;
}

static void parse_xml_tree(xmlDocPtr doc, xmlNodePtr xsdroot)
{

        enum xml_entry_type type;

        if (doc == NULL)
                return;

        if ((doc->type == XML_DOCUMENT_NODE) && (doc->children != NULL)) {
                void *data;
                char *json_str;

                data = parse_xmlnode(doc->children, &type, xsdroot);

                /* Encode our json object into a string */
                json_str = json_encode((JsonObject *)data);
                printf("%s\n", json_str);
                xfree(json_str);

                json_free(data);
        }
}

static void usage_and_die(void)
{
        fprintf(stderr, "xml2json - A program to convert an XML file to JSON!\n");
        fprintf(stderr, "USAGE: xml2json <xmlfile> -x=<xsdfile>\n");
        fprintf(stderr, " xsd|x  : use the xsd file to validate!\n");
        fprintf(stderr, "          (This is optional)\n");
        fprintf(stderr, " help|h : print this help and exit!\n");
        fprintf(stderr, "\n");

        exit(1);
}

int main(int argc, char **argv)
{
        int fd;
        struct stat sbinfo;
        xmlDocPtr doc = NULL;
        char *base;
        int xml_options = XML_PARSE_COMPACT;

        /* XSD related variables */
        int ret ;
        xmlSchemaPtr    schema = NULL;
        xmlSchemaParserCtxtPtr ctxt;
        xmlSchemaValidCtxtPtr vctxt;
        xmlParserCtxtPtr pctxt = NULL;

        static struct option long_options[] = {
                {"xsd", required_argument, NULL, 'x'},
                {"help", no_argument, NULL, 'h'},
                {NULL, 0, NULL, 0}
        };
        int option;
        int option_index;
        char *xsdfile = NULL;
        char *xmlfile = NULL;

#ifdef LINUX
        xml_options |= XML_PARSE_BIG_LINES;
#endif

        while ((option = getopt_long(argc, argv, "hx:",
                                    long_options,
                                    &option_index)) != -1) {
                switch (option) {
                case 'x':
                        xsdfile = optarg;
                        break;
                case 'h':
                case '?':
                default:
                        usage_and_die();
                        break;
                }
        }

        if (argc - optind != 1) {
                usage_and_die();
        }

        xmlfile = argv[optind++];

        /* mmap the file() */
        if (stat(xmlfile, &sbinfo) < 0) {
                perror("stat: ");
                exit(EXIT_FAILURE);
        }

        if ((fd = open(xmlfile, O_RDONLY)) < 0) {
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
        doc = xmlReadMemory((char *) base, sbinfo.st_size, xmlfile,
                            NULL, xml_options);

        munmap((char *)base, sbinfo.st_size);
        close(fd);

        /* Read the xsd and validate the xml before building XSD and XML/JSON
           structures */
        if (xsdfile != NULL) {
                ctxt = xmlSchemaNewParserCtxt(xsdfile);
                xmlSchemaSetParserErrors(ctxt, (xmlSchemaValidityErrorFunc)fprintf,
                                         (xmlSchemaValidityWarningFunc)fprintf,
                                         stderr);
                schema = xmlSchemaParse(ctxt);

                if(schema == NULL) {
                        exit(EXIT_FAILURE);
                }

                vctxt = xmlSchemaNewValidCtxt(schema);
                pctxt = xmlSchemaValidCtxtGetParserCtxt(vctxt) ;

                xmlSchemaSetValidErrors(vctxt, (xmlSchemaValidityErrorFunc)fprintf,
                                        (xmlSchemaValidityWarningFunc) fprintf,
                                        stderr);
                ret = xmlSchemaValidateDoc(vctxt, doc);
                if (ret == 0) {
                        printf("%s validates\n", xmlfile);
                } else if (ret > 0) {
                        printf("%s fails to validate\n", xmlfile);
                } else {
                        printf("%s validation generated an internal error\n", xmlfile);
                }
        }
        parse_xml_tree(doc, xsdfile ? schema->doc->children : NULL);

        xmlFreeDoc(doc);

        exit(EXIT_SUCCESS);
}
