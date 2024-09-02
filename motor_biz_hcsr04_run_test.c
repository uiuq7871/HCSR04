#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pigpio.h>
#include <pthread.h>


#define SERVO_PIN 18
#define BUZZER_DEVICE_FILE "/dev/buzzer"
#define ULTRASONIC_DEVICE_FILE "/dev/rc_hs04"


// 全局變數
int distance_cm = -1;
int fd_ultrasonic, fd_buzzer;
pthread_mutex_t distance_mutex = PTHREAD_MUTEX_INITIALIZER;


// 設置伺服馬達角度
void set_servo_angle(int angle) {
    int min_pulse_width = 500; // 最小脈衝寬度
    int max_pulse_width = 2500; // 最大脈衝寬度
    int pulse_width = (angle * (max_pulse_width - min_pulse_width) / 180) + min_pulse_width;
    gpioServo(SERVO_PIN, pulse_width);
}


// 伺服馬達控制執行緒
void *servo_control_thread(void *arg) {
    while (1) {
        // 設置伺服馬達的初始角度為 0 度
        set_servo_angle(0);
        sleep(5);


        // 設置伺服馬達轉到 90 度
        set_servo_angle(90);
        sleep(5);  // 停頓 15 秒


        // 轉回 0 度
        set_servo_angle(0);
        sleep(5);  // 等待 5 秒


        // 再轉回 90 度
        set_servo_angle(90);
    }


    return NULL;
}


// 讀取超聲波距離的執行緒
void *ultrasonic_thread(void *arg) {
    ssize_t bytes_read;


    while (1) {
        pthread_mutex_lock(&distance_mutex);
        bytes_read = read(fd_ultrasonic, &distance_cm, sizeof(distance_cm));
        pthread_mutex_unlock(&distance_mutex);


        if (bytes_read < 0) {
            perror("Failed to read from ultrasonic device file");
        }
        usleep(100000); // 等待 0.1 秒
    }


    return NULL;
}


// 控制蜂鳴器的執行緒
void *buzzer_thread(void *arg) {
    while (1) {
        pthread_mutex_lock(&distance_mutex);
        if (distance_cm <= 7) {
            if (write(fd_buzzer, "ON", 2) != 2) {
                perror("Failed to write to buzzer device file");
            }
            usleep(2000000); // 保持蜂鳴器開啟 2 秒
        } else {
            if (write(fd_buzzer, "OFF", 3) != 3) {
                perror("Failed to write to buzzer device file");
            }
        }
        pthread_mutex_unlock(&distance_mutex);
        usleep(100000); // 等待 0.1 秒
    }


    return NULL;
}


int main() {
    pthread_t servo_tid, ultrasonic_tid, buzzer_tid;


    if (gpioInitialise() < 0) {
        fprintf(stderr, "pigpio 初始化失敗\n");
        return 1;
    }


    // 開啟超聲波和蜂鳴器裝置文件
    fd_ultrasonic = open(ULTRASONIC_DEVICE_FILE, O_RDONLY);
    if (fd_ultrasonic < 0) {
        perror("Failed to open ultrasonic device file");
        gpioTerminate();
        return 1;
    }


    fd_buzzer = open(BUZZER_DEVICE_FILE, O_WRONLY);
    if (fd_buzzer < 0) {
        perror("Failed to open buzzer device file");
        close(fd_ultrasonic);
        gpioTerminate();
        return 1;
    }


    // 設置伺服馬達的初始角度為 0 度
    set_servo_angle(0);
    sleep(1);


    // 創建伺服馬達控制執行緒
    if (pthread_create(&servo_tid, NULL, servo_control_thread, NULL) != 0) {
        perror("Failed to create servo control thread");
        close(fd_ultrasonic);
        close(fd_buzzer);
        gpioTerminate();
        return 1;
    }


    // 創建超聲波感測器和蜂鳴器控制執行緒
    if (pthread_create(&ultrasonic_tid, NULL, ultrasonic_thread, NULL) != 0) {
        perror("Failed to create ultrasonic thread");
        close(fd_ultrasonic);
        close(fd_buzzer);
        gpioTerminate();
        return 1;
    }


    if (pthread_create(&buzzer_tid, NULL, buzzer_thread, NULL) != 0) {
        perror("Failed to create buzzer thread");
        close(fd_ultrasonic);
        close(fd_buzzer);
        gpioTerminate();
        return 1;
    }


    // 主要執行緒打印伺服馬達位置和距離
    while (1) {
        // 打印距離
        pthread_mutex_lock(&distance_mutex);
        printf("Distance: %d cm\n", distance_cm);
        pthread_mutex_unlock(&distance_mutex);


        sleep(1); // 每秒打印一次
    }


    // 清理
    close(fd_ultrasonic);
    close(fd_buzzer);
    gpioTerminate();


    // 等待執行緒結束
    pthread_cancel(ultrasonic_tid);
    pthread_cancel(buzzer_tid);
    pthread_join(ultrasonic_tid, NULL);
    pthread_join(buzzer_tid, NULL);


    pthread_mutex_destroy(&distance_mutex);


    return 0;
}
