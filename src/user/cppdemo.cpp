/* C++ 用户程序模板示例：演示 String/Vector/syscall 的用法。 */
#include "syscall.hpp"
#include "string.hpp"
#include "vector.hpp"

static void print_dec(int v)
{
        char buf[12];
        int n = 0;
        int i;
        int neg = v < 0;

        if (neg)
                v = -v;
        do
        {
                buf[n++] = (char)('0' + v % 10);
                v /= 10;
        } while (v > 0);
        if (neg)
                buf[n++] = '-';
        for (i = n - 1; i >= 0; i--)
                sys_putchar(buf[i]);
}

static void print_str(const char *s)
{
        sys_write_fd(1, s, cpp_strlen(s));
}

extern "C" int main(int argc, char *argv[])
{
        int i;
        String who("world");
        Vector<int> nums;

        print_str("hello ");
        print_str(who.c_str());
        print_str("\n");

        for (i = 0; i < 3; i++)
                nums.push_back(i * 10);
        for (i = 0; i < nums.size(); i++)
        {
                print_dec(nums[i]);
                if (i + 1 < nums.size())
                        sys_putchar(',');
        }
        sys_putchar('\n');

        if (argc > 1)
        {
                print_str("arg: ");
                print_str(argv[1]);
                print_str("\n");
        }
        return 0;
}
