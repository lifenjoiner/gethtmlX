
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#include "debug.h"
#include "vector.h"
#include "stringx.h"
#include "file.h"

#include "gumbo.h"

char* document_str;
GumboNode* document;

void get_node_children(const GumboNode* node, GumboVector *children) {
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        *children = node->v.document.children;
        break;
    case GUMBO_NODE_ELEMENT:
    // case GUMBO_NODE_TEMPLATE: has no child!
        *children = node->v.element.children;
        break;
    default:
        children->length = 0;
        break;
    }
}

/**/
char* getAttribute(const GumboNode* node, const char* attri) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        GumboAttribute* GA = gumbo_get_attribute(&node->v.element.attributes, attri);
        if (GA != NULL) {
            return (char*)GA->value;
        }
    }
    return NULL;
}

char* textContent(const GumboNode* node) {
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
    // case GUMBO_NODE_TEMPLATE: has no child!
        children = &node->v.element.children;
        length = children->length;
        for (i = 0; i < length; i++) {
            ct_i = textContent( children->data[i] );
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
    get_node_children(node, &children);
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT || child->type == GUMBO_NODE_TEMPLATE ) {
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

/*  special case:
    1. original class name contains space char
    2. filter class name contains space char. IE11: Y, Chrome: N.
    3. getElementsByClassNames(), to locate exactly
*/
Vector getElementsByClassName(const GumboNode* node, const char* classname) {
    GumboVector children;
    GumboAttribute* CLASS;
    Vector node_collection;
    Vector sub_node_collection;
    Vector_init(&node_collection);
    Vector_init(&sub_node_collection);
    int pointer_length = sizeof(void *);
    size_t length, i;
    //
    get_node_children(node, &children);
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT || child->type == GUMBO_NODE_TEMPLATE ) {
            if ((CLASS = gumbo_get_attribute(&child->v.element.attributes, "class"))
            && *CLASS->value ) { /* speed up */
                Vector classname_s, classname_v;
                classname_s = str_split(classname, " ");
                classname_v = str_split(CLASS->value, " \x09\x0A\x0C\x0D");
                if ( Vector_in_Vector(&classname_v, &classname_s) != 0 ) {
                    void **pp = realloc(node_collection.items, (node_collection.length + 1) * pointer_length);
                    assert(pp != NULL);
                    node_collection.items = pp;
                    node_collection.items[node_collection.length] = child;
                    node_collection.length++;
                }
                Vector_free_ex(&classname_v);
                Vector_free_ex(&classname_s);
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

Vector getElementsByTagName(const GumboNode* node, const char* tagname) {
    GumboVector children;
    Vector node_collection;
    Vector sub_node_collection;
    Vector_init(&node_collection);
    Vector_init(&sub_node_collection);
    int pointer_length = sizeof(void *);
    size_t length, i;
    //
    get_node_children(node, &children);
    length = children.length;
    for (i = 0; i < length; i++) {
        GumboNode* child = children.data[i];
        if ( child->type == GUMBO_NODE_ELEMENT || child->type == GUMBO_NODE_TEMPLATE ) {
            char *p, *tag;
            int n = 0;
            /* html returns more
            if (child->v.element.tag != GUMBO_TAG_UNKNOWN) {
                p = (char*)gumbo_normalized_tagname(child->v.element.tag);
                n = strlen(p);
            }
            else
            */
            if (child->v.element.original_tag.length > 0) {
                p = (char*)child->v.element.original_tag.data + 1;
                tag = strpbrk(p, " \x09\x0A\x0C\x0D/>");
                n = tag - p;
            }
            //else //GumboError occutred!
            if (n > 0) {
                tag = calloc(n + 1, 1);
                strncpy(tag, p, n);
                if (stricmp(tag, tagname) == 0) {
                    void **pp = realloc(node_collection.items, (node_collection.length + 1) * pointer_length);
                    assert(pp != NULL);
                    node_collection.items = pp;
                    node_collection.items[node_collection.length] = child;
                    node_collection.length++;
                }
                free(tag);
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

Vector getChildren(const GumboNode* node) {
    Vector children;
    Vector_init(&children);
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        children.items = node->v.document.children.data;
        children.length = node->v.document.children.length;
        break;
    case GUMBO_NODE_ELEMENT:
    // case GUMBO_NODE_TEMPLATE: has no child!
        children.items = node->v.element.children.data;
        children.length = node->v.element.children.length;
        break;
    default:
        break;
    }
    return children;
}

GumboNode* getVectorItem(Vector *NC, size_t i) {
    return NC->items[i];
}

/**/
void PrintStr(const char *s) {
    if (s == NULL) {return;}
    printf("%s\n", s);
}

void PrintGumboNode(const GumboNode* node) {
    char* str = "", *p;
    size_t n = 0;
    if (node == NULL) { return; }
    switch (node->type) {
    case GUMBO_NODE_DOCUMENT:
        return;
        break;
    case GUMBO_NODE_ELEMENT:
    case GUMBO_NODE_TEMPLATE:
        DBG("original_tag.length: %d\n", node->v.element.original_tag.length);
        if (node->v.element.end_pos.offset > node->v.element.start_pos.offset) { //???
            n = node->v.element.end_pos.offset
                - node->v.element.start_pos.offset
                + node->v.element.original_end_tag.length;
        }
        else {
            DBG("end_pos.offset: %d\n", node->v.element.end_pos.offset);
            n = node->v.element.original_tag.length;
        }
        p = document_str + node->v.element.start_pos.offset;
        break;
    case GUMBO_NODE_TEXT:
        p = (char *)node->v.text.text;
        n = strlen(p);
        break;
    default:
        DBG("%d\n", node->v.text.original_text.length);
        //
        n = node->v.text.original_text.length;
        p = (char *)node->v.text.original_text.data;
        break;
    }
    //
    DBG("Node Type: %d\n", node->type);
    //
    if (n > 0) {
        str = calloc(n + 1, 1);
        memcpy(str, p, n);
        PrintStr(str);
        free(str);
    }
}

void PrintGumboNodes(const Vector *nodes) {
    int i, n;
    if (nodes == NULL) {return;}
    n = nodes->length;
    for (i = 0; i < n; i++) {
        PrintGumboNode(nodes->items[i]);
    }
}

/* https://dom.spec.whatwg.org/#interface-document */
void help(const char *app) {
    fprintf(stderr, "Get information from html. v0.2.3 by YX Hao\n");
    fprintf(stderr, "Usage: %s <operations> [html-file]\n", app);
    fprintf(stderr, "Encoding caution: UTF8 desired! Or strange things could happen.\n");
    fprintf(stderr, "operation examples:\n");
    fprintf(stderr, "    getElementById(main).getElementsByClassName(list)[0].getAttribute(href)\n");
    fprintf(stderr, "    getElementById(main).getElementsByClassName(list)[0].textContent\n");
    fprintf(stderr, "    getElementById(main).getElementsByClassName(\"a b\").length\n");
    fprintf(stderr, "    getElementById(main).getElementsByTagName(a)\n");
    fprintf(stderr, "    getElementById(main).children.length\n");
    fprintf(stderr, "    getElementsByTagName(a).each(getAttribute(href))\n");
    fprintf(stderr, "    document\n");
    fprintf(stderr, "    [document.]children\n");
    fprintf(stderr, "examples:\n");
    fprintf(stderr, "    type test.htm | %s getElementsByTagName(a)\n", app);
    fprintf(stderr, "    %s getElementsByTagName(a) < test.htm\n", app);
    fprintf(stderr, "    type ss.htm | %s getElementsByClassName(col-sm-4) | %s getElementsByTagName(h4).each(textContent)\n", app, app);
    fprintf(stderr, "Tips: You may use this together with iconv.\n");
    exit(EXIT_FAILURE);
}

void parse_operations_on_ws(GumboNode* NODE, const char* ops) {
    Vector NODES;
    Vector operations, opr;
    char *param;
    size_t i, n;
    //
    operations = str_split(ops, ".[]");
    n = operations.length;
    //
    DBG("%d\n", n);
    //
    Vector_init(&NODES);
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
            }
            else if (strcmp(pp, "getElementsByClassName") == 0 && NODE) {
                NODES = getElementsByClassName(NODE, param);
                NODE = NULL;
            }
            else if (strcmp(pp, "getElementsByTagName") == 0 && NODE) {
                NODES = getElementsByTagName(NODE, param);
                NODE = NULL;
            }
            else if (strcmp(pp, "getAttribute") == 0 && NODE) {
                PrintStr(getAttribute(NODE, param));
                NODE = NULL;
            }
            else
EACH:
            if (strcmp(pp, "each") == 0 && NODES.length) {
                size_t j, l;
                l = NODES.length;
                for (j = 0; j < l; j++) {
                    parse_operations_on_ws(NODES.items[j], operations.items[i] + 5);
                }
                Vector_free(&NODES);
            }
            else { goto UNKNOWN; }
            break;
        case 1:
            if (isdigit(*pp)) {
                ii = atoi(pp);
                if (NODES.length > 0) {
                    NODE = getVectorItem(&NODES, ii);
                    Vector_free(&NODES);
                }
                else { goto UNKNOWN; }
            }
            else if (strcmp(pp, "textContent") == 0 && NODE) {
                PrintStr(textContent(NODE));
                NODE = NULL;
            }
            else if (strcmp(pp, "length") == 0 && NODES.length) {
                printf("%d\n", NODES.length);
                Vector_free(&NODES);
            }
            else if (strcmp(pp, "document") == 0 && i == 0) {
                // valid, but don't output.
            }
            else if (strcmp(pp, "children") == 0 && NODE) {
                NODES = getChildren(NODE);
                NODE = NULL;
            }
            else { goto UNKNOWN; }
            break;
        default:
UNKNOWN:
            Vector_free(&NODES);
            NODE = NULL;
            fprintf(stderr, "Err: %s\n", operations.items[i]);
            i = n;
            break;
        }
        Vector_free_ex(&opr);
    }
    PrintGumboNode(NODE);
    PrintGumboNodes(&NODES);
    Vector_free_ex(&operations);
}

/**/
int main(int argc, const char** argv) {
    const char* filename;
    FILE* fp;
    char *input;
    size_t input_length, HTML_length;
    GumboOutput* output;
    GumboNode* NODE;
    //
    if (argc == 2) { fp = stdin; }
    else if (argc == 3) {
        //
        filename = argv[2];
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "File not found: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }
    else {
        help(argv[0]);
        return 87; //ERROR_INVALID_PARAMETER
    }
    //
    input_length = read_file(fp, &input);
    if (fp != stdin) { fclose(fp); }
    //
    document_str = input;
    HTML_length = input_length;
    if (strncmp(input, "\xEF\xBB\xBF", 3) == 0) {
        DBG("BOM\n");
        document_str += 3;
        HTML_length -= 3;
    }
    //
    if (HTML_length == 0) {return 0;}
    //
    output = gumbo_parse(document_str);
    //
    DBG("Errors: %d\n", output->errors.length);
    //
    document = output->document;
    NODE = document;
    //
    parse_operations_on_ws(NODE, argv[1]);
    //
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    free(input);
    //
    return 0;
}