#ifndef PROCESS_H
#define PROCESS_H

#include <kernel/task.h>

struct pt_regs;
struct fork_sandbox_config;

task_t *spawn_user_process(const char *path);
void free_task_address_space(task_t *t);
int sys_fork(struct pt_regs *regs, const struct fork_sandbox_config *sb);
int sys_exec(struct pt_regs *regs, const char *path);
int sys_wait(int pid);

#endif
