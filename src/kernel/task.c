#include <kernel/task.h>
#include <driver/vga.h>
#include <stddef.h>

#define MAX_TASKS 16
#define STACK_SIZE 4096

static task_t tasks[MAX_TASKS];
static int task_count = 0;
static int pid_counter = 1;

static task_t idle_task;
task_t *current_task = NULL;

extern void switch_to(task_t *prev, task_t *next);

/* 任务意外返回的处理 */
static void task_exit_handler(void)
{
	vga_write("[TASK] Task returned unexpectedly. Halting.\n");
	for (;;)
		__asm__ volatile ("hlt");
}

/* 创建新任务（带优先级） */
task_t *create_task(task_func_t func, int priority)
{
	task_t *t;
	uint32_t *stack_top;

	if (task_count >= MAX_TASKS)
		return NULL;

	t = &tasks[task_count++];
	t->pid = pid_counter++;
	t->priority = (priority > 0) ? priority : TASK_PRIO_NORMAL;
	t->timeslice = TASK_TIMESLICE;
	t->state = TASK_READY;
	t->ebx = 0;
	t->esi = 0;
	t->edi = 0;

	/* 伪造内核栈 */
	stack_top = (uint32_t *)(t->stack + STACK_SIZE);
	*(--stack_top) = (uint32_t)task_exit_handler;
	*(--stack_top) = (uint32_t)func;
	t->esp = (uint32_t)stack_top;
	t->ebp = t->esp;

	/* 插入调度链表（末尾） */
	if (current_task) {
		t->next = current_task->next;
		current_task->next = t;
	} else {
		t->next = t;
	}

	return t;
}

/* 调度器初始化 */
void task_init(void)
{
	idle_task.pid = 0;
	idle_task.esp = 0;
	idle_task.ebp = 0;
	idle_task.ebx = 0;
	idle_task.esi = 0;
	idle_task.edi = 0;
	idle_task.priority = TASK_PRIO_IDLE;
	idle_task.timeslice = TASK_TIMESLICE;
	idle_task.state = TASK_READY;
	idle_task.next = &idle_task;

	current_task = &idle_task;

	vga_write("[TASK] Scheduler initialized.\n");
}

/* 协作式主动让权 */
void yield(void)
{
	task_t *prev, *next;

	if (!current_task)
		return;

	prev = current_task;
	next = current_task->next;

	if (next == current_task)
		return;

	current_task = next;
	switch_to(prev, next);
}

/* 获取当前任务 */
task_t *get_current_task(void)
{
	return current_task;
}
