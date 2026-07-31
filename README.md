# PokuOS-Kernel

个人兴趣项目：从零编写一个迷你操作系统内核，用于学习底层原理。

## 功能特性

- GRUB（Multiboot1 标准）引导
- 分页内存管理（4KB 页表，递归映射）
- 内核堆分配器
- 抢占式调度（PIT 时钟中断 + 优先级轮转）
- 虚拟文件系统（VFS）
- 初始内存文件系统（initrd，ramfs）
- ELF 用户程序加载与用户态/内核态切换（TSS + iret）
- VGA 文本模式（80×25）+ PS/2 键盘
- 串口（COM1）镜像输出，便于调试
- `int 0x80` 系统调用
- 用户态 Shell

## 环境要求

- Linux 宿主机（开发环境：Arch Linux + base-devel）
- 工具链：`gcc`、`nasm`、`ld`（需支持 `-m elf_i386`）、`make`
- 运行：`qemu-system-i386`
- 制作 ISO：`grub-mkrescue`（可选）

## 快速开始

### 构建

```bash
cd src
make            # 编译内核，生成 kernel.bin
make initrd.img # 编译用户程序 shell.elf 并打包，生成 initrd.img
```

两条都执行后即可运行；`make iso` 会同时构建内核和 initrd，并生成可引导的 ISO 镜像。

### 运行（QEMU）

方式一：直接引导内核（推荐，开发调试方便）

```bash
qemu-system-i386 -m 128M -kernel kernel.bin -initrd initrd.img
```

加 `-serial stdio` 可以把内核日志和 Shell 输出镜像到终端：

```bash
qemu-system-i386 -m 128M -kernel kernel.bin -initrd initrd.img -serial stdio
```

方式二：GRUB ISO

```bash
make iso
qemu-system-i386 -cdrom pokuos.iso
```

## 使用说明

启动后内核会从 initrd 中加载 `shell.elf`，自动切换到用户态 Shell，提示符为 `> `。

| 命令 | 功能 |
|------|------|
| `help` | 显示命令列表 |
| `clear`（或 `clean`） | 清屏 |
| `reboot` | 重启系统 |
| `uname` | 显示内核名称 `PokuOS` |
| 其他 | 提示 `Unknown command` |

输入通过 PS/2 键盘（`Enter` 确认，`Backspace` 删除），输出显示在 VGA 屏幕上并同时镜像到串口。

## 系统调用

通过 `int 0x80` 发起，系统调用号放在 `eax`：

| 号 | 名称 | 说明 |
|----|------|------|
| 0 | `sys_write` | 输出字符串 |
| 1 | `sys_read` | 读取数据 |
| 2 | `sys_exit` | 退出进程 |
| 3 | `sys_getchar` | 读取一个字符 |
| 4 | `sys_putchar` | 输出一个字符 |
| 5 | `sys_clear` | 清屏 |
| 6 | `sys_reboot` | 重启 |

## 项目结构

```text
PokuOS-Kernel/
├── src/
│   ├── boot/        # 引导汇编（Multiboot 头、入口）
│   ├── driver/      # 设备驱动：键盘、串口、VGA
│   ├── include/     # 头文件（按模块分类）
│   ├── init/        # 初始化进程
│   ├── kernel/      # 内核核心：分页、堆、调度、VFS、initrd、ELF、系统调用
│   ├── tools/       # 主机工具（initrd 打包器）
│   ├── user/        # 用户态程序（Shell）
│   ├── makefile     # 构建脚本
│   └── linker.ld    # 内核链接脚本
├── CHANGELOG.md     # 更新日志
├── LICENSE          # 许可证
└── README.md        # 项目说明
```

## 状态与规划

- 开发者正在积极维护
- 当前目标：做一个至少能稳定运行的迷你内核
- 远期目标：尝试在上面运行一些工具

## 许可证

GNU Lesser General Public License v2.1

###### 以及....新的README好看吗
