#include <fs/mount.h>
#include <fs/vfs.h>
#include <kernel/heap.h>
#include <kernel/kstring.h>
#include <kernel/serial.h>
#include <stddef.h>

static struct mount_node *mount_root = NULL;

int vfs_mount(const char *path, struct super_block *sb)
{
        struct mount_node *node;

        if (!path || !sb)
                return -1;

        node = kmalloc(sizeof(struct mount_node));
        if (!node)
                return -1;

        node->path = kmalloc(strlen(path) + 1);
        if (!node->path)
        {
                kfree(node);
                return -1;
        }
        strcpy(node->path, path);
        node->sb = sb;
        memset(node->children, 0, sizeof(node->children));
        node->next = mount_root;
        mount_root = node;

        serial_write("[mount] mounted ");
        serial_write(path);
        serial_write("\n");

        return 0;
}

struct super_block *vfs_find_mount(const char *path, const char **rest)
{
        struct mount_node *node;
        struct mount_node *best = NULL;
        size_t best_len = 0;

        node = mount_root;
        while (node)
        {
                const char *mp = node->path;
                size_t len = strlen(mp);

                if (strncmp(mp, path, len) == 0 &&
                    (path[len] == '\0' || path[len] == '/'))
                {
                        if (len > best_len)
                        {
                                best_len = len;
                                best = node;
                        }
                }
                node = node->next;
        }

        if (rest)
                *rest = path + best_len;

        return best ? best->sb : NULL;
}
