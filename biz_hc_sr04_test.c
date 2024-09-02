#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>  // For int64_t and ssize_t

#define ULTRASONIC_DEVICE_FILE "/dev/rc_hs04"
#define BUZZER_DEVICE_FILE "/dev/buzzer"
#define DATA_FILE "distance_data.csv"

int main() {
    int fd_ultrasonic, fd_buzzer;
   int distance_cm;
    ssize_t bytes_read;

    // 開啟超音波和蜂鳴器設備文件
    fd_ultrasonic = open(ULTRASONIC_DEVICE_FILE, O_RDONLY);
    if (fd_ultrasonic < 0) {
        perror("Failed to open ultrasonic device file");
        return 1;
    }

    fd_buzzer = open(BUZZER_DEVICE_FILE, O_WRONLY);
    if (fd_buzzer < 0) {
        perror("Failed to open buzzer device file");
        close(fd_ultrasonic);
        return 1;
    }

    // 開啟CSV文件（覆蓋模式）
    FILE *datafile = fopen(DATA_FILE, "w");
    if (datafile == NULL) {
        perror("Failed to open data file");
        close(fd_ultrasonic);
        close(fd_buzzer);
        return 1;
    }

    // 寫入CSV標題
    fprintf(datafile, "Distance(cm)\n");

    while (1) {
        // 從超音波感測器讀取距離
        bytes_read = read(fd_ultrasonic, &distance_cm, sizeof(distance_cm));
        if (bytes_read < 0) {
            perror("Failed to read from ultrasonic device file");
            close(fd_ultrasonic);
            close(fd_buzzer);
            fclose(datafile);
            return 1;
        }
        // 輸出並記錄距離
        fprintf(datafile, "%d\n", distance_cm);
        // 根據距離控制蜂鳴器
        if (distance_cm <= 3) {
            if (write(fd_buzzer, "ON", 2) != 2) {
                perror("Failed to write to buzzer device file");
            }
        } else {
            if (write(fd_buzzer, "OFF", 3) != 3) {
                perror("Failed to write to buzzer device file");
            }
        }

    }

    // 清理資源
    close(fd_ultrasonic);
    close(fd_buzzer);
    fclose(datafile);

    return 0;
}
