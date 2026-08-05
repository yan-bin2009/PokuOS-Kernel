#include "file_io.hpp"
#include "string.hpp"
#include "syscall.hpp"

bool load_file(const char *path, Buffer &buf)
{
        char tmp[512];
        String cur;
        int fd;
        int got;
        int i;
        bool any = false;

        fd = sys_open(path, O_RDONLY);
        if (fd < 0)
                return false;

        buf.clear_lines();

        for (;;)
        {
                got = sys_read(fd, tmp, sizeof(tmp));
                if (got <= 0)
                        break;
                any = true;
                for (i = 0; i < got; i++)
                {
                        if (tmp[i] == '\n')
                        {
                                buf.add_line(cur.c_str(), cur.length());
                                cur.clear();
                        }
                        else
                        {
                                cur.append(tmp[i]);
                        }
                }
        }

        if (!any || cur.length() > 0)
                buf.add_line(cur.c_str(), cur.length());

        sys_close(fd);
        return true;
}

bool save_file(const char *path, Buffer &buf)
{
        int fd;
        int i;

        fd = sys_open(path, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd < 0)
                return false;

        for (i = 0; i < buf.num_lines(); i++)
        {
                const String &l = buf.line(i);

                if (sys_write_fd(fd, l.c_str(), (unsigned)l.length()) < 0)
                {
                        sys_close(fd);
                        return false;
                }
                if (sys_write_fd(fd, "\n", 1) < 0)
                {
                        sys_close(fd);
                        return false;
                }
        }

        sys_close(fd);
        return true;
}
