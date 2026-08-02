# PokuOS-Kernel

个人兴趣项目：从零编写一个迷你操作系统内核，用于学习底层原理。

## 功能特性

- GRUB（Multiboot1 标准）引导
- 分页内存管理（4KB 页表、递归映射、用户/内核隔离）
- 写时复制（COW）
- 内核堆分配器（边界标签、前后合并）
- 抢占式调度（PIT 时钟中断 + 优先级轮转）
- 虚拟文件系统（VFS）
- 初始内存文件系统（initrd，ramfs）
- ELF 用户程序加载与用户态/内核态切换（TSS + iret）
- VGA 文本模式（80×25）+ PS/2 键盘
- 异常处理框架（完整寄存器转储、错误码解码、栈回溯）
- `int 0x80` 系统调用（含用户指针合法性校验）
- 用户态 Shell（命令历史、motd 登录横幅）
- ACPI 电源管理（`poweroff` 关机）

## 环境要求

- Linux 宿主机（开发环境：Arch Linux + base-devel）
- 工具链：`gcc`、`nasm`、`ld`（需支持 `-m elf_i386`）、`make`
- 运行：`qemu-system-i386`
- 制作 ISO：`grub-mkrescue`（可选）

## 快速开始

### 构建

```bash
cd src
make        # 编译内核、用户程序并打包 initrd，生成 kernel.bin 和 initrd.img
make iso    # 额外生成可引导的 ISO 镜像（pokuos.iso）
```

### 运行（QEMU）

```bash
make run    # 用 QEMU 图形窗口启动（需先 make iso）
```

或者直接用 GRUB ISO：

```bash
make iso
qemu-system-i386 -cdrom pokuos.iso
```

## 使用说明

启动后内核会从 initrd 中加载 `shell.elf`，自动切换到用户态 Shell，提示符为 `> `。

| 命令 | 功能 |
|------|------|
| `help` | 显示命令列表 |
| `clear` | 清屏 |
| `reboot` | 重启系统 |
| `poweroff` | 关机（ACPI） |
| `uname` | 显示内核名称 `PokuOS` |
| 其他 | 提示 `Unknown command` |

输入通过 PS/2 键盘（`Enter` 确认，`Backspace` 删除，`↑`/`↓` 浏览命令历史），输出显示在 VGA 屏幕上。启动时会先显示 `motd` 登录横幅。

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
| 7 | `sys_poweroff` | 关机（ACPI） |

## 项目结构

```text
PokuOS-Kernel/
├── src/
│   ├── boot/        # 引导汇编（Multiboot 头、入口）
│   ├── driver/      # 设备驱动：键盘、VGA
│   ├── include/     # 头文件（按模块分类）
│   ├── init/        # 初始化进程
│   ├── kernel/      # 内核核心：分页、堆、调度、VFS、initrd、ELF、系统调用、异常
│   ├── tools/       # 主机工具（initrd 打包器）
│   ├── user/        # 用户态程序（Shell）
│   ├── vm/          # VM 层（vm_object / vm_map / COW）
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
