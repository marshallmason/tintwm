#include <unistd.h>

#include "../tintwm.h"

#define EXECUTE(args, ...) { \
	const char *cmd[] = { args, __VA_ARGS__ }; \
	spawn(cmd); \
	break; \
}

#define SHELL(cmd) EXECUTE("/bin/sh", "-c", cmd, NULL)

static void
spawn(const char **cmd)
{
	if (fork() != 0) return;
	setsid();
	execvp(((char **)cmd)[0], (char **)cmd);
	DIE("execvp failed\n");
}
