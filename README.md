# PokuOS-Kernel 

这个项目实际上是个个人兴趣项目，顺便学习底层。

### 当前状态

#### 开发者正在积极维护该项目

### 目前实现了什么？
- 引导暂时采用GRUB（Multiboot标准）
- 分页内存管理（4KB页表，递归映射）
- 内核堆分配器
- 多任务调度由协作式任务切换向抢占式调度过渡（框架，PIT时钟中断 + 优先级轮转）
- 虚拟文件系统（VFS）
- 初始内存文件系统
- 简陋的Shell
- 简单的系统调用（`int 0x80` 提供 `sys_write` 和 `sys_exit`）

### 未来的道路


尝试做到一个至少能跑的迷你内核，至少能稳定跑起来。
在此之后尝试BSD工具能不能在上面跑

### 环境要求

- 32位虚拟机
- 宿主机：base-devel qemu-full （支持 `-m elf_i386`）
- 完整的工具链（`gcc` `nasm``ld` `qemu-system-i386` ）

### 如何构建？？？

```bash
cd src
./build.sh

```
## 体验

启动后你将看到：
```bash

=== PokuOS Init System ===
Type 'help' for commands.
>
```
输入help可以查看目前支持命令

## 项目结构

```text
PokuOS-Kernel/
├── src/
│   ├── boot/               # 引导代码（汇编）
│   ├── driver/             # 设备驱动（C 源文件）
│   ├── include/            # 头文件（按模块分类）
│   │   ├── driver/
│   │   ├── init/
│   │   └── kernel/
│   ├── init/               # 初始化进程（Shell）
│   ├── kernel/             # 内核核心（所有核心模块）
│   └── tools/              # 主机工具（生成 initrd）
├── CHANGELOG.md            # 更新日志
├── LICENSE                 # 许可证
├── README.md               # 项目说明
├── build.sh                # 编译脚本
└── linker.ld               # 链接脚本

```

### 许可证

GNU Lesser General Public License v2.1

