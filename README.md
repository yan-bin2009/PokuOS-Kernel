#####PokuOS - 暂时的名称

首先这是个学习项目，并非正式项目

###当前状态
```text
- [x] 分页（恒等映射前 4MB）
- [x] GDT / IDT 中断管理
- [x] PS/2 键盘驱动（支持 Shift、退格、回车）
- [x] VGA 文本模式显示（清屏、滚动）
- [x] 系统调用（`int 0x80`，支持 `sys_write` / `sys_exit`）
- [x] 用户态切换框架（`iret` 切换至 Ring 3）
- [x] 交互式 Shell（`help` / `clear` / `reboot`）
- [ ] 真正的进程管理（`fork` / `exec` 待实现）
- [ ] 内存管理（`kmalloc` 待实现）
```
### 环境要求

- x86_64 Linux（推荐 Arch 此项目开发者的系统用的这个）
- base-devel （支持 `-m elf_i386`）
- `qemu-system-i386`（可选，用于测试）

### 编译

```bash
cd src
./build.sh

```
###体验

启动后你将看到：
```bash

=== PokuOS Init System ===
Type 'help' for commands.
>
```
输入help可以查看目前支持命令

##项目结构

```text
src/
├── boot/          # 引导程序（Multiboot + GDT/IDT）
├── kernel/        # 内核核心（中断、分页、系统调用、任务）
├── driver/        # 驱动（键盘、VGA）
├── init/          # 初始化（命令行 Shell）
├── include/       # 公共头文件
├── linker.ld      # 链接脚本
└── build.sh       # 编译脚本

```
###系统调用

当前支持的系统调用：
```text
调用号	名称	参数
-----------------------------
1	sys_exit	无
-----------------------------
4	sys_write	fd, buf, len
-----------------------------
```
###相关代码
```nasm
mov eax, 4          ; sys_write
mov ebx, 1          ; stdout
mov ecx, msg
mov edx, 22
int 0x80
```
###许可证

GNU Lesser General Public License v2.1

```text

原版说明文档如下：

这是一个比较简陋的玩具内核，目前测试，能在32 位 x86 机器上操作，能够在QEMU中引导，响应键盘中断（由于缺少shell,导致你输入的东西是无法返回相应的指令）并在屏幕上回显文字
1.创建目的
我实际上是为了自己能够理解一些操作系统底层原理，包括学习nasm，C语言等等。虽然大多由AI协助。
2.她目前能干什么？
很抱歉，什么也干不了。只能吞下你在键盘上打的字
3.如何启动
 通过 GRUB 或 QEMU 的 `-kernel` 选项加载,执行"qemu-system-i386 -kernel kernel.bin -m 32"
4.目前特征
 保护模式下的段管理和中断处理，支持键盘输入，包括backpace,换行等等。
 5.环境要求
 x86 架构 Linux（推荐 Arch 因为我就在这个系统上写的）
`nasm`、`gcc`、`ld`（支持 `-m elf_i386`）
6.那我怎么构建？
很简单，我早在src下给你弄好了：

``bash
cd ~/pokuos/src
chmod +x build.sh
./build.sh
``

7.协议？
 GNU LESSER GENERAL PUBLIC LICENSE，另见于LICENSE文件
8.补充
我现在尝试把shell内置里面，或者作为外置软件，内核只需要保持干净就对了。
以及里面错误的名称：keybord等等，我是懒得改的，我把这些屎一样的代码扔进ai时，他强烈建议我改正。可惜的是我偏偏不改。
所以不要在意
                          --yan-bin2009   2026/7/25

```
