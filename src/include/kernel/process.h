#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/task.h>

#define MAX_EXEC_ARGS 8
#define EXEC_ARG_MAXLEN 64

struct pt_regs;
struct fork_sandbox_config;

task_t *spawn_user_process(const char *path);
void free_task_address_space(task_t *t);
void resolve_path(task_t *t, const char *path, char *buf, int buflen);
int sys_fork(struct pt_regs *regs, const struct fork_sandbox_config *sb);
int sys_exec(struct pt_regs *regs, const char *path, char argv[][EXEC_ARG_MAXLEN]);
int sys_wait(int pid);

#endif
