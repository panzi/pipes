#ifndef PIPES_H
#define PIPES_H
#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pipes {
	pid_t pid;
	int infd;
	int outfd;
	int errfd;
};

#define PIPES_LEAVE      -1
#define PIPES_PIPE       -2
#define PIPES_NULL       -3
#define PIPES_ERR_TO_OUT -4

#define PIPES_IN(IN)   {-1, (IN),        PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_PASS     {-1, PIPES_PIPE,  PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_OUT(OUT) {-1, PIPES_PIPE,  (OUT),       PIPES_LEAVE}
#define PIPES_HEAD     {-1, PIPES_LEAVE, PIPES_PIPE,  PIPES_LEAVE}
#define PIPES_TAIL     {-1, PIPES_PIPE,  PIPES_LEAVE, PIPES_LEAVE}

int pipes_open(char const *const argv[], char const *const envp[], struct pipes* pipes);
int pipes_close(struct pipes* pipes);

struct pipes_chain {
	struct pipes pipes;
	char const* const* argv;
	char const* const* envp;
};

int pipes_open_chain(struct pipes_chain chain[]);
int pipes_close_chain(struct pipes_chain chain[]);
int pipes_kill_chain(struct pipes_chain chain[], int sig);

#ifdef __cplusplus
}
#endif

#endif
