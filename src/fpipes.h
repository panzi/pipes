#ifndef PIPES_FPIPES_H
#define PIPES_FPIPES_H
#pragma once

#include <sys/types.h>
#include <stdio.h>

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These special values don't use NULL so passing the return values
 * of failed fopen() calls and similar are detected. */
#define FPIPES_LEAVE      ((FILE*)1)
#define FPIPES_PIPE       ((FILE*)2)
#define FPIPES_NULL       ((FILE*)3)
#define FPIPES_TO_STDOUT  ((FILE*)4)
#define FPIPES_TO_STDERR  ((FILE*)5)
#define FPIPES_TEMP       ((FILE*)6)

#define FPIPES_PASS     {-1, FPIPES_PIPE,  FPIPES_PIPE,  FPIPES_LEAVE}
#define FPIPES_IN(IN)   {-1, (IN),         FPIPES_PIPE,  FPIPES_LEAVE}
#define FPIPES_OUT(OUT) {-1, FPIPES_PIPE,  (OUT),        FPIPES_LEAVE}
#define FPIPES_ERR(ERR) {-1, FPIPES_PIPE,  FPIPES_PIPE,  (ERR)}
#define FPIPES_FIRST    {-1, FPIPES_LEAVE, FPIPES_PIPE,  FPIPES_LEAVE}
#define FPIPES_LAST     {-1, FPIPES_PIPE,  FPIPES_LEAVE, FPIPES_LEAVE}

#define FPIPES_GET_LAST(CHAIN) ((CHAIN)[(sizeof(CHAIN) / sizeof(struct fpipes_chain))-2].pipes)
#define FPIPES_GET_IN(CHAIN)   ((CHAIN)[0].pipes.in)
#define FPIPES_GET_OUT(CHAIN)  (FPIPES_GET_LAST(CHAIN).out)
#define FPIPES_GET_ERR(CHAIN)  (FPIPES_GET_LAST(CHAIN).err)

struct fpipes {
	pid_t pid;
	FILE *in;
	FILE *out;
	FILE *err;
};

struct fpipes_chain {
	struct fpipes pipes;
	char const* const* argv;
	char const* const* envp;
};

PIPES_EXPORT int fpipes_open(char const *const argv[], char const *const envp[], struct fpipes* pipes);
PIPES_EXPORT int fpipes_close(struct fpipes* pipes);

PIPES_EXPORT int fpipes_open_chain( struct fpipes_chain chain[]);
PIPES_EXPORT int fpipes_close_chain(struct fpipes_chain chain[]);
PIPES_EXPORT int fpipes_kill_chain( struct fpipes_chain chain[], int sig);

PIPES_EXPORT FILE* fpipes_take_in( struct fpipes_chain chain[]);
PIPES_EXPORT FILE* fpipes_take_out(struct fpipes_chain chain[]);
PIPES_EXPORT FILE* fpipes_take_err(struct fpipes_chain chain[]);

#ifdef __cplusplus
}
#endif

#endif
