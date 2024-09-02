#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>  // For int64_t and ssize_t

#define DEVICE_FILE "/dev/rc_hs04"

int main() {
    int fd;
    ssize_t distance_cm;
    ssize_t bytes_read;

    fd = open(DEVICE_FILE, O_RDONLY);
    if (fd < 0) {
        perror("無法打開設備檔");
        return 1;
    }

    while (1) {
        // 讀取數據
        bytes_read = read(fd, &distance_cm, sizeof(distance_cm));
        if (bytes_read < 0) {
            perror("讀取設備檔失敗");
            close(fd);
            return 1;
        } else if (bytes_read != sizeof(distance_cm)) {
            fprintf(stderr, "讀取的數據大小不正確\n");
            close(fd);
            return 1;
        }

        // 打印距離
        printf("距離: %zd 公分\n", distance_cm);

        // 暫停1秒鐘
        sleep(1);
    }

    close(fd);
    return 0;
}
