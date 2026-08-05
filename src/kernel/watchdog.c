#include <kernel/kstring.h>
#include <kernel/process.h>
#include <kernel/serial.h>
#include <kernel/task.h>
#include <kernel/tier.h>
#include <kernel/watchdog.h>

#define WD_MAX 8
#define WD_MAX_RESTARTS 5
#define WD_UNSTABLE_LIMIT 3
#define WD_BACKOFF_BASE 5
#define WD_UNSTABLE_TICKS 10

struct wd_entry
{
        char path[64];
        tier_t tier;
        task_t *task;
        int restarts;
        int unstable;
        int delay_ticks;
        int spawn_due;
        int active;
        uint32_t spawn_tick;
};

static struct wd_entry wd_table[WD_MAX];
static int wd_count;
static uint32_t wd_tick;

void watchdog_init(void)
{
        wd_count = 0;
        wd_tick = 0;
}

int watchdog_register(const char *path, tier_t tier, task_t *initial_task)
{
        struct wd_entry *e;

        if (!path || wd_count >= WD_MAX)
                return -1;
        e = &wd_table[wd_count++];
        strcpy(e->path, path);
        e->tier = tier;
        e->task = initial_task;
        e->restarts = 0;
        e->unstable = 0;
        e->delay_ticks = 0;
        e->spawn_due = 0;
        e->active = 1;
        e->spawn_tick = wd_tick;
        if (initial_task)
                initial_task->wd_managed = 1;
        return 0;
}

void watchdog_tick(void)
{
        int i;

        wd_tick++;
        for (i = 0; i < wd_count; i++)
        {
                struct wd_entry *e = &wd_table[i];
                task_t *t = e->task;

                if (!e->active)
                        continue;
                if (t && t->state != TASK_EXITED)
                        continue;

                if (e->delay_ticks > 0)
                {
                        e->delay_ticks--;
                        if (e->delay_ticks == 0)
                                e->spawn_due = 1;
                        continue;
                }

                if (t)
                {
                        uint32_t life = wd_tick - e->spawn_tick;

                        if (life < WD_UNSTABLE_TICKS)
                                e->unstable++;
                        else
                                e->unstable = 0;

                        if (e->unstable >= WD_UNSTABLE_LIMIT)
                        {
                                serial_write("[WD] give up (unstable): ");
                                serial_write(e->path);
                                serial_write("\n");
                                if (t)
                                        free_task_slot(t);
                                e->task = NULL;
                                e->active = 0;
                                continue;
                        }
                        e->restarts++;
                        if (e->restarts > WD_MAX_RESTARTS)
                        {
                                serial_write("[WD] give up (max restarts): ");
                                serial_write(e->path);
                                serial_write("\n");
                                if (t)
                                        free_task_slot(t);
                                e->task = NULL;
                                e->active = 0;
                                continue;
                        }
                        serial_write("[WD] restart ");
                        serial_write(e->path);
                        serial_write(" restarts=");
                        serial_write_hex(e->restarts);
                        serial_write(" delay=");
                        serial_write_hex(WD_BACKOFF_BASE << (e->restarts - 1));
                        serial_write("\n");
                        e->delay_ticks = WD_BACKOFF_BASE << (e->restarts - 1);
                        if (t)
                                free_task_slot(t);
                        e->task = NULL;
                        continue;
                }

                e->spawn_due = 1;
        }
}

void watchdog_check(void)
{
        int i;

        for (i = 0; i < wd_count; i++)
        {
                struct wd_entry *e = &wd_table[i];

                if (!e->active || !e->spawn_due)
                        continue;
                e->spawn_due = 0;
                e->task = spawn_user_process(e->path);
                if (e->task)
                {
                        e->task->wd_managed = 1;
                        e->spawn_tick = wd_tick;
                        serial_write("[WD] spawned ");
                        serial_write(e->path);
                        serial_write(" pid=");
                        serial_write_hex(e->task->pid);
                        serial_write("\n");
                }
                else
                {
                        e->spawn_due = 1;
                }
        }
}
