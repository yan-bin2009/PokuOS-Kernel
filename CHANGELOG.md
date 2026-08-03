 # 更新日志

## 2026-07-25（小规模更新）

- 项目结构整理，头文件移入 `include/` 目录
- 所有源码缩进统一为 8 格制表符
- 实现系统调用（int 0x80），支持 sys_write 和 sys_exit
- 添加用户态切换框架（switch_to_user）
- 完善 init 命令行（help/clear/reboot）
- 修复多处编译错误（重复 include、类型未定义等）
-
## 2026-07-26-00:55（小规模更新）

- 修复了已知漏洞
- 光标错位问题解决
- 优化了代码可维护性，可扩展性
- 任务调度器完善：
- create_task 真正实现，支持最多 16 个任务
- 使用静态数组分配任务，无需动态内存
- switch_to 保存/恢复完整上下文（ebp/ebx/esi/edi）
- 系统调用改进
- sys_write 返回写入字节数
- 显式保存/恢复调用者寄存器（ebx/ecx/edx）
- 添加用户态指针校验，防止非法访问
- 用宏替代硬编码向量号（IRQ1_VECTOR、SYSCALL_VECTOR）
- 用宏替代门类型标志（GATE_INTERRUPT、GATE_USER）
- PIC 初始化使用宏替代魔法数字

## 2026-07-27(小规模更新)

- 修复了已知问题
- 添加部分新的功能
- 优化了代码


## 2026-07-27-22:06（小型更新） 
- 修复了已知漏洞
- 添加了新的功能，具体如下：
#### 添加了虚拟文件系统（VFS）
#### 初始内存文件系统
#### 内核字符串库
#### 多了一些屎山代码

## 2026-07-28(小版本（等等，我没有定版本号）更新)
- 新增抢占式调度框架（PIT 时钟中断 + 优先级轮转）
- 任务结构体扩展（priority、timeslice、state）
- 修复 `idt_set_gate` 静态声明问题
## 2026-07-29(小规模更新)

- 重写了build.sh，现在支持 `./build.sh iso` 生成 GRUB 镜像
- 经过测试，GRUB正式可以使用
- 抢占式调度框架调通
- 优化了PIT 时钟中断
- 多写了屎山代码

## 2026-08-01（重大更新）

- 全局代码都重写了一遍（作者重写代码文件，提供基础，剩下交给ai优化）
- 正式实现用户态
- 修补了许多bug
- 宣布正式版本号： V26.1.0（年+大版本+小版本）
- 全局代码的风格统一：花符号另起，8位缩进，极少注释
## 2026-08-03(V26.1.1)
- 修补了些bug
- VM虚拟内存
- 重新写了键盘驱动，感谢linux的馈赠（参考linux-0.99）
- 重写makefile
- 经过慎重考虑，删掉部分代码

## 2026-08-04(V26.2)
- 异常处理框架（trap.c 完整实现）
- capability 权限模型(细粒度,syscall权限控制)
- 沙盒支持
- 代码规范工具
- shell.c：新增 tier/wait 命令，fork/exec/wait 流程完善
- 四级 Tier 权限系统
#### TIER_KERNEL > TIER_SYSTEM > TIER_USER > TIER_CRITICAL，不仅用于调度优先级，还控制内存压力响应、capability 分配和系统调用权限。
-  Capability 能力模型
#### 15 种 capability 位掩码（CAP_WRITE/CAP_READ/CAP_FORK/CAP_EXEC/CAP_REBOOT/CAP_POWEROFF 等），用户态默认仅基础 IO 权限，特权操作需显式授权。





 接下来是重要的东西:
 内核实现：src/kernel/process.c:165 — sys_fork()
- 收集父进程用户页表
- 创建子任务槽，继承/覆盖沙盒配置（caps/mem_limit/cpu_quota/root_path/tier）
- 创建子进程页目录（paging_create_task_pd()）
- 复制用户物理页（vm_copy_phys()）
- 构造子进程内核栈（fork_return + pusha + int0x80 帧）
- 用户态调用：src/user/lib/syscall.h:83 — sys_fork()
- 通过 int $0x80 触发，系统调用号 SYS_FORK = 10
调度器 (kernel/sched.c)
- 按 tier 分配时间片（内核1/系统2/用户3）
- set_task_timeslice() 初始化/切换时更新时间片
- PIT 中断集成看门狗 tick（每 50 tick 触发）
任务管理 (kernel/task.c)
- 新增 tier_override、mlfq_level、caps、mem_limit、cpu_quota、root_path、wd_managed 等字段
- task_set_tier() / task_set_mlfq_level()：动态调整任务等级和 MLFQ 队列
- task_quota_period_reset()：CPU 配额周期重置
- task_reap_orphans()：孤儿进程回收
- task_find_child() / task_exit_handler()：子进程查找和退出处理
系统调用 (kernel/syscall.c)
- 新增 user_range_valid()：用户指针合法性校验（逐页检查 PTE_PRESENT + PTE_USER）
- 新增 cap_for()：系统调用 → capability 映射
- 新增 SYS_FORK_WITH_SANDBOX、SYS_EXEC、SYS_WAIT 三个系统调用
- SYS_TIER_QUERY / SYS_TIER_REQUEST：查询/设置任务等级
- SYS_REBOOT / SYS_POWEROFF：ACPI 关机/重启
构建系统
- makefile 重构：支持 fs/ 目录编译、bin/ 目标文件目录
- user/makefile：用户程序独立构建
- 新增 tools/check-format.sh：代码格式检查（尾随空格/Tab/clang-format）
头文件体系
- 新增 30+ 个头文件，按模块分类（driver/fs/kernel/vm/user/init/sys）
- sys/queue.h：BSD TAILQ 双向链表宏（来自 Berkeley）

代码行数超过6500
