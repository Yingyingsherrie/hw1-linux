/* -*- compile-command: "gcc b.c -o b" -*- */

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

#ifdef __minix
#define PIDCOL  2
#define PPIDCOL	3
#else
#define PIDCOL	1
#define PPIDCOL	2
#endif

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
    char *buf = (char*)malloc(BUFLEN), *ptr, *cmd = NULL;
    size_t n = BUFLEN, len, i;
    entry *head = NULL, *e, *e2, etmp, **roots;
    int first = 1, nroots = 0;
    int cmdoff = -1;

    while (getline(&buf, &n, fp) > 0) {
	if (first) {
    	    first = 0;
	    ptr = strstr(buf, "CMD");
	    assert(ptr);
	    cmdoff = ptr - buf;
    	    continue;
	}
	len = strlen(buf);

	if (!len) continue;	/* empty line */

	if (buf[len-1] == '\n')
	    buf[--len] = '\0';

	assert(len >= cmdoff);
	cmd = strdup(buf+cmdoff);

	char *saveptr;
	int flag = 1;
	int fld = 0;

	// printf("%s\n", buf);
	int pid, ppid, cont=1;
	while (cont && (ptr = strtok
		((flag ? buf : NULL),  " \t")) != NULL) {
	    flag = 0;
	    switch (fld++) {
		case PIDCOL:
		    pid = atoi(ptr);
		    break;
		case PPIDCOL:
		    ppid = atoi(ptr);

		    e = (entry *)malloc(sizeof(entry));
		    assert (pid >= 0 && ppid >= 0);
		    e->pid = pid;
		    e->ppid = ppid;
		    e->cmd = cmd;
		    e->nchld = e->cnt = 0;
		    e->children = NULL;
		    e->parent = NULL;
		    DL_APPEND(head, e);
		    e = NULL;
		    pid = -1;
		    ppid = -1;

		    cont = 0;
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
	    nroots++;
	    /*
	    assert(nroots < 16);
	    roots[nroots++] = e;
	    */
	}
    }
    assert(nroots > 0);
    roots = (entry **)malloc(sizeof(entry *) * nroots);

    DL_FOREACH(head, e) {
	e->children = (entry **)malloc(sizeof(entry *) * e->nchld);
    }

    i = 0;
    DL_FOREACH(head, e) {
	if ((e2 = e->parent) != NULL)
	    e2->children[e2->cnt++] = e;
	else
	    roots[i++] = e;
    }
    assert(i == nroots);

    DL_FOREACH(head, e) {
	assert(e->nchld == e->cnt);
    }

    for (i = 0; i < nroots; i++)
	f(roots[i], 0);

    return 0;
}
