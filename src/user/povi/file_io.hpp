#ifndef POVI_FILE_IO_HPP
#define POVI_FILE_IO_HPP

#include "buffer.hpp"

/* 通过 sys_open/sys_read/sys_write/sys_close 加载与保存缓冲区。 */
bool load_file(const char *path, Buffer &buf);
bool save_file(const char *path, Buffer &buf);

#endif
