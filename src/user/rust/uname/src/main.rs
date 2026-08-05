//! uname —— 外置 Rust 用户程序：打印内核名称。
//!
//! 演示如何用 Rust 编写 PokuOS 用户态程序：
//!   - no_std / no_main，panic 直接 halt
//!   - 自写 `_start`（等价 C 的 crt0.c），从栈读 argc/argv 后调用 main
//!   - 用内联汇编 `int $0x80` 发起系统调用
//!   - rust-lld + uname.ld 直接产出静态 ELF32，由内核 elf_load 加载

#![no_std]
#![no_main]

const SYS_WRITE: u32 = 0;
const SYS_EXIT: u32 = 2;

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> !
{
        loop {}
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn sys_write_fd(fd: u32, buf: *const u8, len: u32) -> i32
{
        let ret: u32;

        unsafe {
                core::arch::asm!(
                        "int $0x80",
                        inlateout("eax") SYS_WRITE => ret,
                        in("ebx") fd,
                        in("ecx") buf,
                        in("edx") len,
                        options(nostack)
                );
        }
        ret as i32
}

#[unsafe(no_mangle)]
pub unsafe extern "C" fn _exit(code: u32) -> !
{
        unsafe {
                core::arch::asm!(
                        "int $0x80",
                        in("eax") SYS_EXIT,
                        in("ebx") code,
                        options(noreturn, nostack)
                );
        }
}

#[unsafe(no_mangle)]
pub extern "C" fn main(_argc: i32, _argv: *const *const u8) -> i32
{
        const MSG: &[u8] = b"PokuOS\n";

        unsafe {
                sys_write_fd(1, MSG.as_ptr(), MSG.len() as u32);
        }
        0
}

#[unsafe(naked)]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn _start() -> !
{
        core::arch::naked_asm!(
                "mov 0(%esp), %eax",          /* argc */
                "lea 4(%esp), %ebx",          /* argv */
                "push %ebx",
                "push %eax",
                "call main",
                "add $8, %esp",
                "push %eax",
                "call _exit",
                "hlt"
                ,
                options(att_syntax)
        );
}
