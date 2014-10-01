#ifndef PIPES_PIPES_H
#define PIPES_PIPES_H
#pragma once

#include <sys/types.h>

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These special values don't use -1 so passing the return values
 * of failed open() calls and similar are detected. */
#define PIPES_LEAVE      -2
#define PIPES_PIPE       -3
#define PIPES_NULL       -4
#define PIPES_ERR_TO_OUT -5

#define PIPES_PASS     {-1, PIPES_PIPE,  PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_IN(IN)   {-1, (IN),        PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_OUT(OUT) {-1, PIPES_PIPE,  (OUT),       PIPES_LEAVE}
#define PIPES_ERR(ERR) {-1, PIPES_PIPE,  PIPES_LEAVE, (ERR)}
#define PIPES_FIRST    {-1, PIPES_LEAVE, PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_LAST     {-1, PIPES_PIPE,  PIPES_LEAVE, PIPES_LEAVE}

#define PIPES_GET_LAST(CHAIN) ((CHAIN)[(sizeof(CHAIN) / sizeof(struct pipes_chain))-2].pipes)
#define PIPES_GET_IN(CHAIN)   ((CHAIN)[0].pipes.infd)
#define PIPES_GET_OUT(CHAIN)  (PIPES_GET_LAST(CHAIN).outfd)
#define PIPES_GET_ERR(CHAIN)  (PIPES_GET_LAST(CHAIN).errfd)

struct pipes {
	pid_t pid;
	int infd;
	int outfd;
	int errfd;
};

struct pipes_chain {
	struct pipes pipes;
	char const* const* argv;
	char const* const* envp;
};

PIPES_EXPORT int pipes_open(char const *const argv[], char const *const envp[], struct pipes* pipes);
PIPES_EXPORT int pipes_close(struct pipes* pipes);

PIPES_EXPORT int pipes_open_chain( struct pipes_chain chain[]);
PIPES_EXPORT int pipes_close_chain(struct pipes_chain chain[]);
PIPES_EXPORT int pipes_kill_chain( struct pipes_chain chain[], int sig);

#ifdef __cplusplus
}
#endif

#endif
