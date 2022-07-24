/*
 * mdb.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mylist.h"
#include "mdb.h"

int loadmdb(FILE *fp, struct List *dest) 
{
    /*
     * read all records into memory
     */

    struct MdbRec r;
    struct Node *node = NULL;
    int count = 0;

    while (fread(&r, sizeof(r), 1, fp) == 1) {

        // allocate memory for a new record and copy into it the one
        // that was just read from the database.
        struct MdbRec *rec = (struct MdbRec *)malloc(sizeof(r));
        if (!rec)
            return -1;
        memcpy(rec, &r, sizeof(r));

        // add the record to the linked list.
        node = addAfter(dest, node, rec);
        if (node == NULL) 
            return -1;

        count++;
    }

    // see if fread() produced error
    if (ferror(fp)) 
        return -1;

    return count;
}

void freemdb(struct List *list) 
{
    // free all the records
    traverseList(list, &free);
    removeAllNodes(list);
}
