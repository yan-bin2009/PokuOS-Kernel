CC = gcc
CXX = g++
LD = ld
CARGO = $(HOME)/.cargo/bin/cargo +nightly
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector -mno-red-zone -mno-sse -mno-sse2 -mno-mmx -mno-3dnow -msoft-float -I. -I../include
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -fno-builtin -nostdinc++
LDFLAGS = -m elf_i386 -T user.ld -nostdlib

SRCS = $(filter-out tss.c,$(wildcard *.c))
TARGETS = $(SRCS:.c=.elf)
POVI_SRCS = $(wildcard povi/*.cpp)
POVI_OBJS = $(POVI_SRCS:.cpp=.o)
CPP_SRCS = $(filter-out povi/%,$(wildcard *.cpp))
CPP_ELFS = $(CPP_SRCS:.cpp=.elf)
CPP_LIB_OBJS = cpp_lib/runtime.o
RUST_UNAME = rust/uname/target/i686-unknown-none/release/uname
ALL_TARGETS = $(TARGETS) $(CPP_ELFS) povi.elf uname

all: $(ALL_TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

povi/%.o: povi/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cpp_lib/%.o: cpp_lib/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Icpp_lib -c $< -o $@

lib/crt0.o: lib/crt0.c
	$(CC) $(CFLAGS) -c $< -o $@

%.elf: %.o lib/crt0.o
	$(LD) $(LDFLAGS) -o $@ $^

$(CPP_ELFS): %.elf: %.o $(CPP_LIB_OBJS) lib/crt0.o
	$(LD) $(LDFLAGS) -o $@ $^

povi.elf: $(POVI_OBJS) lib/crt0.o
	$(LD) $(LDFLAGS) -o $@ $^

$(RUST_UNAME): rust/uname/src/main.rs rust/uname/Cargo.toml \
	rust/uname/.cargo/config.toml rust/uname/uname.ld ../rust/i686-unknown-none.json
	cd rust/uname && $(CARGO) build --quiet --release -Z build-std=core -Zjson-target-spec

# cargo 编译较慢，禁止与其它目标并行，避免 -j 下争抢/读到半成品
.NOTPARALLEL: $(RUST_UNAME)

uname: $(RUST_UNAME)
	cp $< $@

# 保留中间 .o：避免每次 make 都删除并全量重编用户程序
.SECONDARY:

clean:
	rm -f *.o *.elf lib/*.o povi/*.o cpp_lib/*.o cppdemo.elf
	rm -rf rust/uname/target uname

initrd: $(TARGETS)
	cp *.elf ../initrd/

.PHONY: all clean initrd
