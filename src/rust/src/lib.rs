//! 内核字符串/内存库（替代 kernel/string.c）。
//!
//! 全部以 `extern "C"` 导出，符号与 C 头文件 include/kernel/kstring.h 一致。
//! memcpy/memset/memcmp 用 volatile 逐字节实现，避免被 LLVM 识别为内建调用
//! 而退化成对自身符号的递归。

#![no_std]
#![no_main]

#[panic_handler]
fn panic(_info: &core::panic::PanicInfo) -> !
{
        loop {}
}

#[no_mangle]
pub unsafe extern "C" fn memcpy(dest: *mut core::ffi::c_void, src: *const core::ffi::c_void, n: usize) -> *mut core::ffi::c_void
{
        let mut i: usize = 0;

        while i < n
        {
                core::ptr::write_volatile((dest as *mut u8).add(i), core::ptr::read_volatile((src as *const u8).add(i)));
                i += 1;
        }
        dest
}

#[no_mangle]
pub unsafe extern "C" fn memset(s: *mut core::ffi::c_void, c: i32, n: usize) -> *mut core::ffi::c_void
{
        let v: u8 = c as u8;
        let mut i: usize = 0;

        while i < n
        {
                core::ptr::write_volatile((s as *mut u8).add(i), v);
                i += 1;
        }
        s
}

#[no_mangle]
pub unsafe extern "C" fn memcmp(s1: *const core::ffi::c_void, s2: *const core::ffi::c_void, n: usize) -> i32
{
        let mut i: usize = 0;

        while i < n
        {
                let a: u8 = core::ptr::read_volatile((s1 as *const u8).add(i));
                let b: u8 = core::ptr::read_volatile((s2 as *const u8).add(i));

                if a != b
                {
                        return a as i32 - b as i32;
                }
                i += 1;
        }
        0
}

#[no_mangle]
pub unsafe extern "C" fn strcmp(s1: *const u8, s2: *const u8) -> i32
{
        let mut i: usize = 0;

        loop
        {
                let a: u8 = *s1.add(i);
                let b: u8 = *s2.add(i);

                if a == 0 || a != b
                {
                        return a as i32 - b as i32;
                }
                i += 1;
        }
}

#[no_mangle]
pub unsafe extern "C" fn strncmp(s1: *const u8, s2: *const u8, n: usize) -> i32
{
        let mut i: usize = 0;

        while i < n
        {
                let a: u8 = *s1.add(i);
                let b: u8 = *s2.add(i);

                if a != b || a == 0
                {
                        return a as i32 - b as i32;
                }
                i += 1;
        }
        0
}

#[no_mangle]
pub unsafe extern "C" fn strcpy(dest: *mut u8, src: *const u8) -> *mut u8
{
        let mut i: usize = 0;

        loop
        {
                let c: u8 = *src.add(i);

                *dest.add(i) = c;
                if c == 0
                {
                        break;
                }
                i += 1;
        }
        dest
}

#[no_mangle]
pub unsafe extern "C" fn strlen(s: *const i8) -> usize
{
        let mut len: usize = 0;

        while *s.add(len) != 0
        {
                len += 1;
        }
        len
}

#[no_mangle]
pub unsafe extern "C" fn strchr(s: *const u8, c: i32) -> *mut u8
{
        let target: u8 = c as u8;
        let mut i: usize = 0;

        loop
        {
                let ch: u8 = *s.add(i);

                if ch == target
                {
                        return s.add(i) as *mut u8;
                }
                if ch == 0
                {
                        return core::ptr::null_mut();
                }
                i += 1;
        }
}
