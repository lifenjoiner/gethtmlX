
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "array.h"
#include "stringx.h"
#include "file.h"

#include "gumbo.h"

char* document_str;
GumboNode* document;

char* getAttribute(const GumboNode* node, const char* attri) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute* GA = gumbo_get_attribute(&node->v.element.attributes, attri);
        if (GA != NULL) {
            return (char*)GA->value;
        }
    }
    return NULL;
}

char* getContent(const GumboNode* node) {
    char *ct, *ct_i;
    size_t length, i;
    const GumboVector* children;
    //
    ct = calloc(1, 1);
    //
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children = &node->v.document.children;
    case GUMBO_NODE_ELEMENT:
        children = &node->v.element.children;
        length = children->length;
        for (i = 0; i < length; i++) {
            ct_i = getContent( children->data[i] );
            //
            DBG("%s\n", ct_i);
            //
            ct = strxcat(ct, ct_i);
            free(ct_i);
        }
        break;
    case GUMBO_NODE_TEXT:
        ct_i = (char*)node->v.text.text;
        //
        DBG("%s\n", ct_i);
        //
        ct = strxcat(ct, ct_i);
        break;
    default:
        break;
    }
    //
    return ct;
}

GumboNode* getElementById(const GumboNode* node, const char* id) {
    GumboVector children;
    GumboNode* targetNode = NULL;
    GumboAttribute* ID;
    size_t length, i;
    //
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children = node->v.document.children;
        break;
    case GUMBO_NODE_ELEMENT:
        children = node->v.element.children;
        break;
    default:
        children.length = 0;
        break;
    }
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT ) {
            if ( (ID = gumbo_get_attribute(&child->v.element.attributes, "id"))
                && strcmp(ID->value, id) == 0 ) {
                targetNode = child;
                break;
            }
            if ( targetNode = getElementById(child, id) ) break;
        }
    }
    return targetNode;
}

typedef Array NodeCollection;

/* MUST be initialled, or there will be random value! */
static void node_collection_init(NodeCollection* nc) {
    nc->items = NULL;
    nc->length = 0;
}

static void node_collection_free(NodeCollection* nc) {
    free(nc->items);
    nc->length = 0;
}

/*  special case:
    1. original class name contains space char
    2. filter class name contains space char. IE11: Y, Chrome: N.
    3. getElementsByClassNames(), to locate exactly
*/
NodeCollection getElementsByClassName(const GumboNode* node, const char* classname) {
    GumboVector children;
    GumboAttribute* CLASS;
    NodeCollection node_collection;
    NodeCollection sub_node_collection;
    node_collection_init(&node_collection);
    node_collection_init(&sub_node_collection);
    int pointer_length = sizeof(void *);
    size_t length, i;
    //
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children = node->v.document.children;
        break;
    case GUMBO_NODE_ELEMENT:
        children = node->v.element.children;
        break;
    default:
        children.length = 0;
        break;
    }
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT ) {
            if ((CLASS = gumbo_get_attribute(&child->v.element.attributes, "class"))
            && *CLASS->value ) { /* speed up */
                Array classname_s, classname_v;
                classname_s = str_split(classname, " ");
                classname_v = str_split(CLASS->value, " \x09\x0A\x0C\x0D");
                if ( array_in_array(&classname_v, &classname_s) != 0 ) {
                    void **pp = realloc(node_collection.items, (node_collection.length + 1) * pointer_length);
                    assert(pp != NULL);
                    node_collection.items = pp;
                    node_collection.items[node_collection.length] = child;
                    node_collection.length++;
                }
            }
            if ( sub_node_collection = getElementsByClassName(child, classname), sub_node_collection.length > 0 ) {
                void **pp = realloc(node_collection.items, (node_collection.length + sub_node_collection.length) * pointer_length);
                assert(pp != NULL);
                node_collection.items = pp;
                memcpy(node_collection.items + node_collection.length, sub_node_collection.items, sub_node_collection.length * pointer_length);
                node_collection.length += sub_node_collection.length;
                free(sub_node_collection.items);
            }
        }
    }
    return node_collection;
}

NodeCollection getElementsByTagName(const GumboNode* node, const char* tagname) {
    GumboVector children;
    NodeCollection node_collection;
    NodeCollection sub_node_collection;
    node_collection_init(&node_collection);
    node_collection_init(&sub_node_collection);
    int pointer_length = sizeof(void *);
    size_t length, i;
    //
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children = node->v.document.children;
        break;
    case GUMBO_NODE_ELEMENT:
        children = node->v.element.children;
        break;
    default:
        children.length = 0;
        break;
    }
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT ) {
            if (child->v.element.original_tag.length > 0) {
                char *p, *p2;
                int n;
                p = (char*)child->v.element.original_tag.data;
                p2 = strpbrk(p, " \x09\x0A\x0C\x0D/>");
                n = p2 - p;
                if (n > 1) {
                    p2 = calloc(n, 1);
                    strncpy(p2, p + 1, n - 1);
                    if (stricmp(p2, tagname) == 0) {
                        void **pp = realloc(node_collection.items, (node_collection.length + 1) * pointer_length);
                        assert(pp != NULL);
                        node_collection.items = pp;
                        node_collection.items[node_collection.length] = child;
                        node_collection.length++;
                    }
                    free(p2);
                }
            }
            if ( sub_node_collection = getElementsByTagName(child, tagname), sub_node_collection.length > 0 ) {
                void **pp = realloc(node_collection.items, (node_collection.length + sub_node_collection.length) * pointer_length);
                assert(pp != NULL);
                node_collection.items = pp;
                memcpy(node_collection.items + node_collection.length, sub_node_collection.items, sub_node_collection.length * pointer_length);
                node_collection.length += sub_node_collection.length;
                free(sub_node_collection.items);
            }
        }
    }
    return node_collection;
}

NodeCollection getChildren(const GumboNode* node) {
    NodeCollection children;
    node_collection_init(&children);
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children.items = node->v.document.children.data;
        children.length = node->v.document.children.length;
        break;
    case GUMBO_NODE_ELEMENT:
        children.items = node->v.element.children.data;
        children.length = node->v.element.children.length;
        break;
    default:
        break;
    }
    return children;
}

GumboNode* getNodeCollectionItem(NodeCollection *NC, size_t i) {
    return NC->items[i];
}

//
void PrintStr(const char *s) {
    if (s == NULL) {return;}
    printf("%s\n", s);
}

void PrintGumboNode(const GumboNode* node) {
    char* str;
    size_t n;
    str = "";
    if (node == NULL) { return; }
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        return;
        break;
    case GUMBO_NODE_ELEMENT:
        n = node->v.element.end_pos.offset
            - node->v.element.start_pos.offset
            + node->v.element.original_end_tag.length;
        //
        DBG("%d, %d\n", n, node->v.element.original_tag.data);
        //
        if (n > 0) {
            str = calloc(n + 1, 1);
            //memcpy(str, node->v.element.original_tag.data, n);
            /* documentElement */
            memcpy(str, document_str + node->v.element.start_pos.offset, n);
        }
        else { return; }
        break;
    case GUMBO_NODE_TEXT:
        str = (char*)node->v.text.text;
        break;
    default:
        DBG("%d\n", node->v.text.original_text.length);
        //
        n = node->v.text.original_text.length;
        if (n > 0) {
            str = calloc(n + 1, 1);
            memcpy(str, node->v.text.original_text.data, n);
        }
        else { return; }
        break;
    }
    //
    DBG("Node Type: %d\n", node->type);
    //
    PrintStr(str);
}

void PrintGumboNodes(const NodeCollection *nodes) {
    int i;
    if (nodes == NULL) {return;}
    for (i = 0; i < nodes->length; i++) {
        PrintGumboNode(nodes->items[i]);
    }
}

//
void help(const char *app) {
    printf("Get information from html. v0.2.0 by YX Hao\n");
    printf("Usage: %s <operations> [html-file]\n", app);
    printf("operation examples:\n");
    printf("    getElementById(main).getElementsByClassName(list)[0].getAttribute(href)\n");
    printf("    getElementById(main).getElementsByClassName(list)[0].getContent\n");
    printf("    getElementById(main).getElementsByClassName(\"a b\").length\n");
    printf("    getElementById(main).getElementsByTagName(a)\n");
    printf("    getElementById(main).children.length\n");
    printf("    getElementsByTagName(a).each(getAttribute(href))\n");
    printf("    document\n");
    printf("    [document.]children\n");
    printf("examples:\n");
    printf("    type test.htm | %s document\n", app);
    printf("    %s document < test.htm\n", app);
    printf("    type ss.htm |%s getElementById(free).getElementsByClassName(col-sm-4).each(getElementsByTagName(h4)) |%s getElementsByTagName(h4).each(getContent) >st.txt\n", app, app);
    exit(EXIT_FAILURE);
}

void parse_options_on_ws(NodeCollection NODES, GumboNode* NODE, const char* ops) {
    Array operations, opr;
    char *param;
    size_t i, n;
    //
    operations = str_split(ops, ".[]");
    n = operations.length;
    //
    DBG("%d\n", n);
    //
    i = n;
    for (i = 0; i < n; i++) {
        char *pp;
        int ii, nn;
        //
        DBG("%d, %s\n", i, operations.items[i]);
        //
        opr = str_split(operations.items[i], "()");
        nn = opr.length;
        pp = opr.items[0];
        //
        DBG("\t%d\n", nn);
        //
        switch (nn) {
        case 3:
            goto EACH;
        case 2:
            param = opr.items[1];
            if (strcmp(pp, "getElementById") == 0 && NODE == document) {
                NODE = getElementById(NODE, param);
                if (i == n -1) { PrintGumboNode(NODE); }
            }
            else if (strcmp(pp, "getElementsByClassName") == 0 && NODE) {
                NODES = getElementsByClassName(NODE, param);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else if (strcmp(pp, "getElementsByTagName") == 0 && NODE) {
                NODES = getElementsByTagName(NODE, param);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else if (strcmp(pp, "getAttribute") == 0 && NODE) {
                PrintStr(getAttribute(NODE, param));
                NODE = NULL;
            }
            else
EACH:
            if (strcmp(pp, "each") == 0 && NODES.length) {
                size_t j, l;
                NodeCollection NODES2;
                node_collection_init(&NODES2);
                l = NODES.length;
                for (j = 0; j < l; j++) {
                    parse_options_on_ws(NODES2, NODES.items[j], operations.items[i] + 5);
                }
                node_collection_free(&NODES);
            }
            else { goto UNKNOWN; }
            break;
        case 1:
            if (isdigit(*pp)) {
                ii = atoi(pp);
                if (NODES.length > 0) {
                    NODE = getNodeCollectionItem(&NODES, ii);
                    node_collection_free(&NODES);
                    if (i == n -1) { PrintGumboNode(NODE); }
                }
                else { goto UNKNOWN; }
            }
            else if (strcmp(pp, "getContent") == 0 && NODE) {
                PrintStr(getContent(NODE));
                node_collection_free(&NODES);
            }
            else if (strcmp(pp, "length") == 0 && NODES.length) {
                printf("%d\n", NODES.length);
                node_collection_free(&NODES);
            }
            else if (strcmp(pp, "document") == 0 && NODE == document) {
                if (i == 0 && i == n - 1) {
                    PrintStr(document_str); //MUST
                    node_collection_free(&NODES);
                }
            }
            else if (strcmp(pp, "children") == 0 && NODE) {
                NODES = getChildren(NODE);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else { goto UNKNOWN; }
            break;
        default:
UNKNOWN:
            printf("Err: %s\n", operations.items[i]);
            i = n;
            break;
        }
    }
}

//
int main(int argc, const char** argv) {
    const char* filename;
    FILE* fp;
    char* input;
    size_t input_length;
    GumboOutput* output;
    NodeCollection NODES;
    GumboNode* NODE;
    //
    if (argc == 2) { fp = stdin; }
    else if (argc == 3) {
        //
        filename = argv[2];
        fp = fopen(filename, "r");
        if (!fp) {
            printf("File not found: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }
    else { help(argv[0]); }
    //
    input_length = read_file(fp, &input);
    if (fp != stdin) { fclose(fp); }
    //
    if (input_length == 0) {return 0;}
    //
    output = gumbo_parse_with_options(&kGumboDefaultOptions, input, input_length);
    //
    document_str = input;
    document = output->document;
    NODE = document;
    node_collection_init(&NODES);
    //
    parse_options_on_ws(NODES, NODE, argv[1]);
    //
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    free(input);
}