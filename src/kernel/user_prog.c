// kernel/user_prog.c
// 用户程序入口（运行在 Ring 3）
void user_program() {
    // 调用 sys_write
    const char* msg = "Hello from User Mode!\n";
    int len = 22;
    __asm__ volatile (
	"mov $4, %%eax\n"   // SYS_WRITE
	"mov $1, %%ebx\n"   // stdout
	"mov %0, %%ecx\n"
	"mov %1, %%edx\n"
	"int $0x80\n"
	"mov $1, %%eax\n"   // SYS_EXIT
	"int $0x80\n"
	: : "r"(msg), "r"(len)
	: "eax", "ebx", "ecx", "edx"
    );
    // 如果退出失败，死循环
    while (1) __asm__ ("hlt");
}
