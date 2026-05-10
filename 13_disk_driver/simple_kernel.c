/* simple_kernel.c - 简单测试内核 */

#define VIDEO_MEMORY 0xB8000

void kernel_main(void) {
    char *video = (char *)VIDEO_MEMORY;

    /* 清屏 */
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = 0x0F;
    }

    /* 输出消息 */
    const char *msg = "Disk Driver Test - Kernel Loaded!";
    int i = 0;
    while (msg[i]) {
        video[i * 2] = msg[i];
        video[i * 2 + 1] = 0x0A;  /* 绿色 */
        i++;
    }

    /* 停机 */
    while (1) {
        __asm__ volatile("hlt");
    }
}
