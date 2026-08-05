# PokuOS-Kernel

实验项目:小型操作系统内核

当前版本: V26.2.2

## 功能特性

- GRUB（Multiboot1）引导，QEMU 可直接运行，物理内存 64MB
- 分页内存管理 + 写时复制（COW）+ 虚拟内存层
- 内核堆分配器（边界标签、前后合并、可动态扩展）
- 抢占式调度（PIT 时钟中断 + 5 级 MLFQ 队列）
- 虚拟文件系统（VFS）+ RAMFS（initrd）+ TARFS + 页缓存 + memfs（可写内存文件系统）
- ELF 用户程序加载与用户态/内核态切换（TSS + iret）
- 多语言支持：C / C++ / Rust
- 外部程序：povi 文本编辑器、uname
- 安全模型：四级 Tier 权限 + Capability 能力模型 + 沙盒 fork
- 看门狗（崩溃自动重启）、异常处理框架（寄存器转储、栈回溯）
- 内核字符串库以 Rust 实现（kstring）
- `int 0x80` 系统调用（23 个）、用户态 Shell（命令历史、外部程序执行）、ACPI 电源管理

## 环境要求

- Linux 宿主机（开发环境：Arch Linux + base-devel）
- 工具链：`gcc`、`nasm`、`ld`（需支持 `-m elf_i386`）、`make`
- Rust：`rustup`（nightly）+ `cargo`（需 nightly 特性 build-std），目标 `i686-unknown-none`
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
make run    
```

或者直接用 GRUB ISO：

```bash
make iso
qemu-system-i386 -cdrom pokuos.iso
```

## 使用说明

启动后内核会从 initrd 中加载 `shell.elf`，自动切换到用户态 Shell，提示符为 `> `。

Shell 支持执行外部程序（`*.elf`），输入无后缀命令时自动补 `.elf` 重试。程序查找路径：`/mnt`（TARFS 只读，含打包的用户程序）与 `/home`（memfs 可写）。内置用户程序：`hello`、`args`（传参）、`fork_test`（fork/wait）、`burner`（调度压力）、`sandbox_test`（沙盒）、`cppdemo`（C++ 演示）、`povi`（文本编辑器）、`uname`（Rust）。

| 命令 | 功能 |
|------|------|
| `help` | 显示命令列表 |
| `clear` | 清屏 |
| `reboot` | 重启系统 |
| `poweroff` | 关机（ACPI） |
| `uname` | 显示内核名称 `PokuOS` |
| `tier` | 显示当前任务 tier |
| `wait` | 等待子进程退出 |
| `ls` | 列出 `/mnt` 与 `/home` 目录 |
| `povi` | 启动文本编辑器 |
| 其他 | 尝试作为外部程序执行 |

输入通过 PS/2 键盘（`Enter` 确认，`Backspace` 删除，`↑`/`↓` 浏览命令历史），输出显示在 VGA 屏幕上。启动时会先显示 `motd` 登录横幅。

## 系统调用

通过 `int 0x80` 发起，系统调用号放在 `eax`：

| 号 | 名称 | 说明 |
|----|------|------|
| 0 | `sys_write` | 输出字符串 |
| 1 | `sys_open` | 打开文件 |
| 2 | `sys_exit` | 退出进程 |
| 3 | `sys_getchar` | 读取一个字符 |
| 4 | `sys_putchar` | 输出一个字符 |
| 5 | `sys_clear` | 清屏 |
| 6 | `sys_reboot` | 重启 |
| 7 | `sys_poweroff` | 关机（ACPI） |
| 8 | `sys_tier_query` | 查询当前 tier |
| 9 | `sys_tier_request` | 请求切换 tier |
| 10 | `sys_fork` | 创建子进程（COW） |
| 11 | `sys_exec` | 加载并执行 ELF |
| 12 | `sys_wait` | 等待子进程退出 |
| 13 | `sys_fork_with_sandbox` | 沙盒 fork |
| 14 | `sys_mlfq_query` | 查询 MLFQ 队列信息 |
| 15 | `sys_yield` | 主动让出 CPU |
| 16 | `sys_read` | 读取数据 |
| 17 | `sys_close` | 关闭文件 |
| 18 | `sys_kill` | 终止进程 |
| 19 | `sys_set_tier` | 设置进程 tier |
| 20 | `sys_uname` | 读取内核名称 |
| 21 | `sys_ls` | 列出目录 |
| 22 | `sys_sandbox_query` | 查询沙盒状态 |

## 项目结构

```text
PokuOS-Kernel/
├── src/
│   ├── boot/        # 引导汇编（Multiboot 头、入口）
│   ├── driver/      # 设备驱动：键盘、VGA、ATA
│   ├── fs/          # 文件系统：VFS、RAMFS、TARFS、memfs、页缓存
│   ├── include/     # 头文件（按模块分类）
│   ├── init/        # 初始化进程
│   ├── kernel/      # 内核核心：分页、堆、调度、ELF、系统调用、异常
│   ├── rust/        # 内核 kstring 字符串库（Rust 实现）
│   ├── tools/       # 主机工具（格式检查）
│   ├── user/        # 用户态程序（Shell、povi、测试程序、C++/Rust 支持）
│   │   ├── cpp_lib/ # C++ 用户程序运行时
│   │   ├── povi/    # 文本编辑器（C++）
│   │   └── rust/    # Rust 用户程序（uname）
│   ├── vm/          # VM 层（vm_object / vm_map / COW）
│   ├── makefile     # 构建脚本
│   └── linker.ld    # 内核链接脚本
├── CHANGELOG.md     # 更新日志
├── LICENSE          # 许可证
└── README.md        # 项目说明
```

## 状态与规划

- 当前版本: V26.2.2
- 开发者正在积极维护
- 已完成: 虚拟内存层、异常处理框架、看门狗、Capability 权限模型、沙盒 fork、TARFS 文件系统、memfs、多语言用户程序（C/C++/Rust）、povi 文本编辑器
- 当前目标: 稳定运行迷你内核，完善用户态程序生态
- 远期目标: 尝试在上面运行更多工具

## 许可证

GNU Lesser General Public License v2.1
