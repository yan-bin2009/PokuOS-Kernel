# 更新日志

## 2026-07-25

- 项目结构整理，头文件移入 `include/` 目录
- 所有源码缩进统一为 8 格制表符
- 实现系统调用（int 0x80），支持 sys_write 和 sys_exit
- 添加用户态切换框架（switch_to_user）
- 完善 init 命令行（help/clear/reboot）
- 修复多处编译错误（重复 include、类型未定义等）
