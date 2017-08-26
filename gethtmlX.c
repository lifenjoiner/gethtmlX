
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include "gumbo.h"

char* document_str;
GumboNode* document;

#ifdef DEBUG
#define DEBUG printf
#else
#define DEBUG
#endif

typedef struct{
    void** items;
    size_t length;
} Array;

Array str_to_array(const char* str, const char* delims) {
    Array Str;
    char *str_t, *p;
    void **pp;
    //
    Str.length = 0;
    Str.items = calloc(1, sizeof(char*));
    //
    str_t = strdup(str);
    p = strtok(str_t, delims);
    while (p != NULL) {
        pp = realloc(Str.items, sizeof(char*) * (Str.length+1));
        assert(pp != NULL);
        Str.items = pp;
        Str.items[Str.length] = p;
        Str.length++;
        p = strtok(NULL, delims);
    }
    return Str;
}

int array_in_array(Array *arr_v, Array *arr_s) {
    size_t i, j;
    int flag_t, flag_i;
    flag_t = 0;
    for (i = 0; i < arr_s->length; i++) {
        flag_i = 0;
        for (j = 0; j < arr_v->length; j++) {
            if (strcmp(arr_s->items[i], arr_v->items[j]) == 0) {
                flag_i = 1;
                break;
            }
        }
        if (flag_i == 1) {
            flag_t = 1;
        }
        else {
            flag_t = 0;
            break;
        }
    }
    return flag_t;
}

static void read_file(FILE* fp, char** output, size_t* length) {
#define BUFF_SIZE 1024
    char buff[BUFF_SIZE + 1];
    *length = 0; // just in case
    *output = calloc(BUFF_SIZE + 1, 1);
    while (*output = realloc(*output, *length + BUFF_SIZE + 1), assert(*output != NULL), fgets(*output + *length, BUFF_SIZE, fp) != NULL) {
        *length = strlen(*output);
    }
}

char* getAttribute(const GumboNode* node, const char* attri) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute* GA = gumbo_get_attribute(&node->v.element.attributes, attri);
        if (GA != NULL) {
            return (char*)GA->value;
        }
    }
    return NULL;
}

char* strxcat(char *dst, const char *src) {
    size_t n;
    char *pp;
    //
    n = strlen(src);
    if (n > 0) {
        pp = realloc(dst, strlen(dst) + n + 1);
        assert(pp != NULL);
        dst = pp;
        dst = strncat(dst, src, n);
    }
    return dst;
}

char* getContent(const GumboNode* node) {
    char *ct;
    char *ct_i;
    size_t length, i;
    //
    ct = calloc(1, 1);
    const GumboVector* children;
    //
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children = &node->v.document.children;
    case GUMBO_NODE_ELEMENT:
        children = &node->v.element.children;
        length = children->length;
        for (i = 0; i < length; i++) {
            ct_i = getContent( children->data[i] );
            //PrintStr(ct_i);
            ct = strxcat(ct, ct_i);
            free(ct_i);
        }
        break;
    case GUMBO_NODE_TEXT:
        ct_i = (char*)node->v.text.text;
        //PrintStr(ct_i);
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

/*  special case:
    1. original class name contains space char
    2. filter class name contains space char. IE11: Y, Chrome: N.
    3. getElementsByClassNames(), to locate exactly
    4. use regex
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
                classname_s = str_to_array(classname, " ");
                classname_v = str_to_array(CLASS->value, " \x09\x0A\x0C\x0D");
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
        //printf("%d, %d\n", n, node->v.element.original_tag.data);
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
        //printf("%d\n", node->v.text.original_text.length);
        n = node->v.text.original_text.length;
        if (n > 0) {
            str = calloc(n + 1, 1);
            memcpy(str, node->v.text.original_text.data, n);
        }
        else { return; }
        break;
    }
    //printf("Node Type: %d\n", node->type);
    PrintStr(str);
}

void PrintGumboNodes(const NodeCollection *nodes) {
    int i;
    if (nodes == NULL) {return;}
    for (i = 0; i < nodes->length; i++) {
        PrintGumboNode(nodes->items[i]);
    }
}

void each_getAttribute(const NodeCollection *nodes, const char* attri) {
    int i;
    if (nodes == NULL) {return;}
    for (i = 0; i < nodes->length; i++) {
        PrintStr(getAttribute(nodes->items[i], attri));
    }
}

/*
void PrintElementChildren(const GumboVector *children) {
    int i;
    for (i = 0; i < children->length; i++) {
        PrintGumboNode(children->data[i]);
    }
}
*/

/*
typedef void (*FUNC)();

typedef struct _FUNCOP{
    char *name;
    FUNC opr;
    FUNC outer;
} FUNCOP;

FUNCOP func_list[] = {
{"getElementById", (FUNC)&getElementById, (FUNC)&PrintGumboNode},
{"getElementsByClassName", (FUNC)&getElementsByClassName, (FUNC)&PrintGumboNodes},
{"getAttribute", (FUNC)&getAttribute, (FUNC)&PrintAttrValue},
{"getContent", (FUNC)&getContent, (FUNC)&PrintContent}
//{"getNodeCollectionItem", (FUNC)&getNodeCollectionItem, (FUNC)&PrintGumboNodes}
};

FUNCOP getOP(char *opr) {
    size_t i, n;
    n = sizeof(func_list)/sizeof(FUNCOP);
    for (i = 0; i < n; i++) {
        if (strcmp(opr, func_list[i].name) == 0) {
            return func_list[i];
        }
    }
    return (FUNCOP)NULL;
}

void* store_ret() {
    
}
*/

void help(const char *app) {
    printf("Usage: %s <operations> [html-file]\n", app);
    printf("    operation examples:\n");
    printf("    getElementById(main).getElementsByClassName(list)[0].getAttribute(href)\n");
    printf("    getElementById(main).getElementsByClassName(list)[0].getContent\n");
    printf("    getElementById(main).getElementsByClassName(list).length\n");
    printf("    getElementById(main).getElementsByTagName(a).length\n");
    printf("    getElementById(main).children.length\n");
    printf("    getElementsByTagName(a).each_getAttribute(href)\n");
    printf("    document\n");
    printf("    [document.]children\n");
    printf("    document < test.htm\n");
    printf("    type test.htm | document\n");
    exit(EXIT_FAILURE);
}

//
int main(int argc, const char** argv) {
    const char* filename;
    FILE* fp;
    char* input;
    size_t input_length = 0;
    GumboOutput* output;
    Array operations, opr, param;
    size_t i, n;
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
    read_file(fp, &input, &input_length);
    if (fp != stdin) { fclose(fp); }
    //
    if (input_length == 0) {return 0;}
    //
    document_str = input;
    //
    output = gumbo_parse_with_options(&kGumboDefaultOptions, input, input_length);
    //
    operations = str_to_array(argv[1], ".[]");
    document = output->document;
    NODE = document;
    node_collection_init(&NODES);
    n = operations.length;
    //printf("%d\n", n);
    i = n;
    for (i = 0; i < n; i++) {
        char *pp;
        int ii, nn;
        //printf("%d, %s\n", i, operations.items[i]);
        opr = str_to_array(operations.items[i], "()");
        nn = opr.length;
        pp = opr.items[0];
        //printf("\t%d\n", nn);
        switch (opr.length) {
        case 2:
            if (strcmp(pp, "getElementById") == 0 && NODE == document) {
                NODE = getElementById(NODE, opr.items[1]);
                if (i == n -1) { PrintGumboNode(NODE); }
            }
            else if (strcmp(pp, "getElementsByClassName") == 0 && NODE) {
                NODES = getElementsByClassName(NODE, opr.items[1]);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else if (strcmp(pp, "getElementsByTagName") == 0 && NODE) {
                NODES = getElementsByTagName(NODE, opr.items[1]);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else if (strcmp(pp, "getAttribute") == 0 && NODE) {
                PrintStr(getAttribute(NODE, opr.items[1]));
                i = n;
            }
            else if (strcmp(pp, "each_getAttribute") == 0 && NODES.length) {
                each_getAttribute(&NODES, opr.items[1]);
            }
            else {
                printf("Err: %s\n", operations.items[i]);
                i = n;
            }
            break;
        case 1:
            if (isdigit(*pp)) {
                ii = atoi(pp);
                if (NODES.length > 0) {
                    NODE = getNodeCollectionItem(&NODES, ii);
                    NODES.length = 0;
                    free(NODES.items);
                    if (i == n -1) { PrintGumboNode(NODE); }
                }
                else {
                    printf("Err: %s\n", operations.items[i]);
                    i = n;
                }
            }
            else if (strcmp(pp, "getContent") == 0 && NODE) {
                PrintStr(getContent(NODE));
                i = n;
            }
            else if (strcmp(pp, "length") == 0 && NODES.length) {
                printf("%d\n", NODES.length);
                i = n;
            }
            else if (strcmp(pp, "document") == 0 && NODE == document) {
                if (i == 0 && i == n - 1) {
                    PrintStr(document_str);
                    //PrintGumboNode(NODE->v.element.children.data[0]);
                    i = n;
                }
            }
            else if (strcmp(pp, "children") == 0 && NODE) {
                NODES = getChildren(NODE);
                NODE = NULL;
                if (i == n -1) { PrintGumboNodes(&NODES); }
            }
            else {
                printf("Err: %s\n", operations.items[i]);
                i = n;
            }
            break;
        default:
            printf("Err: %s\n", operations.items[i]);
            i = n;
            break;
        }
    }
    //
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    free(input);
}