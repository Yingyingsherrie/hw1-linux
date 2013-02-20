/* -*- compile-command: "gcc a.c -o a.out" -*- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "utlist.h"

typedef struct __entry {
    int pid;
    int ppid;
    char *cmd;
    int nchld, cnt;
    struct __entry **children, *parent;
    struct __entry *prev;
    struct __entry *next;
} entry;

#define BUFLEN 256

int pidcmp(entry *a, entry *b)
{
    return a->pid - b->pid;
}

void f(entry *e, int n)
{
    int i;
    for (i = 0; i < n; i++) putchar(' ');
    printf("%d:%d:%s\n", e->pid, e->ppid, e->cmd);
    for (i = 0; i < e->nchld; i++)
  f(e->children[i], n + 4);
}

int main()
{
    FILE *fp = popen("ps -ef", "r");
    char *buf = (char*)malloc(BUFLEN), *ptr;
    size_t n = BUFLEN, len, i;
    entry *head = NULL, *e, *e2, etmp, *roots[16];
    int first = 1, nroots = 0;

    while (getline(&buf, &n, fp) > 0) {
	if (first) { first = 0; continue; }
	len = strlen(buf);

	if (!len) continue; 	/* empty line */

	if (buf[len-1] == '\n')
	    buf[--len] = '\0';

	char *saveptr;
	int flag = 1;
	int fld = 0;

	// printf("%s\n", buf);
	int pid, ppid;
	while ((ptr = strtok
		((flag ? buf : NULL), 
		 (fld == 7 ? "" : " \t"))) != NULL) {
	    flag = 0;
	    switch (fld++) {
		case 1:
		    pid = atoi(ptr);
		    break;
		case 2:
		    ppid = atoi(ptr);
		    break;
		case 7:
		    e = (entry *)malloc(sizeof(entry));
		    assert (pid >= 0 && ppid >= 0);
		    e->pid = pid;
		    e->ppid = ppid;
		    e->cmd = strdup(ptr);
		    e->nchld = e->cnt = 0;
		    e->children = NULL;
		    e->parent = NULL;
		    DL_APPEND(head, e);
		    e = NULL;
		    pid = -1;
		    ppid = -1;
		    break;
	    }
	}
    }
    pclose(fp);

    DL_FOREACH(head, e) {
	if (e->ppid) {
	    etmp.pid = e->ppid;
	    DL_SEARCH(head, e2, &etmp, pidcmp);
	    assert(e2->pid == e->ppid);
	    e->parent = e2;
	    e2->nchld++;
	} else {
	    assert(nroots < 16);
	    roots[nroots++] = e;
	}
    }
    assert(nroots > 0);

    DL_FOREACH(head, e) {
	e->children = (entry **)malloc(sizeof(entry *) * e->nchld);
    }

    DL_FOREACH(head, e) {
	if ((e2 = e->parent))
	    e2->children[e2->cnt++] = e;
    }

    DL_FOREACH(head, e) {
	assert(e->nchld == e->cnt);
    }

    for (i = 0; i < nroots; i++)
	f(roots[i], 0);

    return 0;
}
