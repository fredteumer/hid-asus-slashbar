// Simple userspace helper for slashbar control
// Compile: gcc -o slashbar-helper slashbar-helper.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define REPORT_ID 0x5D
#define HIDRAW_DEVICE "/dev/hidraw8"

int send_packet(int fd, const unsigned char *payload, size_t payload_len) {
    unsigned char buf[64] = {0};
    buf[0] = REPORT_ID;
    if (payload_len > 0) {
        memcpy(&buf[1], payload, payload_len < 63 ? payload_len : 63);
    }
    return write(fd, buf, 64);
}

int initialize_slashbar(int fd) {
    // Wake-up sequence
    const unsigned char wakeup1[] = "ASUS Tech.Inc.";
    if (send_packet(fd, wakeup1, strlen((char*)wakeup1)) < 0) return -1;
    usleep(10000);
    
    const unsigned char wakeup2[] = {0xC2};
    if (send_packet(fd, wakeup2, sizeof(wakeup2)) < 0) return -1;
    usleep(10000);
    
    const unsigned char wakeup3[] = {0xD1, 0x01, 0x00, 0x01};
    if (send_packet(fd, wakeup3, sizeof(wakeup3)) < 0) return -1;
    usleep(10000);
    
    // Initialization sequence
    const unsigned char init1[] = {0xD7, 0x00, 0x00, 0x01, 0xAC};
    if (send_packet(fd, init1, sizeof(init1)) < 0) return -1;
    usleep(10000);
    
    const unsigned char init2[] = {0xD2, 0x02, 0x01, 0x08, 0xAB};
    if (send_packet(fd, init2, sizeof(init2)) < 0) return -1;
    usleep(10000);
    
    return 0;
}

int control_slashbar(int fd, int enable) {
    const unsigned char cmd[] = {0xD8, 0x02, 0x00, 0x01, enable ? 0x00 : 0x80};
    return send_packet(fd, cmd, sizeof(cmd));
}

int set_slashbar_mode_static(int fd) {
    // Set static mode (mode 0x06 based on typical RGB LED controllers)
    const unsigned char mode_cmd[] = {0xD4, 0x02, 0x06, 0x01, 0xFF, 0xFF, 0xFF}; // Static white
    if (send_packet(fd, mode_cmd, sizeof(mode_cmd)) < 0) return -1;
    usleep(10000);
    
    // Enable the mode
    const unsigned char enable_cmd[] = {0xD8, 0x02, 0x00, 0x01, 0x00};
    return send_packet(fd, enable_cmd, sizeof(enable_cmd));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <init|on|off|static>\n", argv[0]);
        return 1;
    }
    
    int fd = open(HIDRAW_DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open hidraw device");
        return 1;
    }
    
    int ret = 0;
    if (strcmp(argv[1], "init") == 0) {
        ret = initialize_slashbar(fd);
        if (ret >= 0) printf("Slashbar initialized\n");
    } else if (strcmp(argv[1], "on") == 0) {
        ret = control_slashbar(fd, 1);
        if (ret >= 0) printf("Slashbar enabled\n");
    } else if (strcmp(argv[1], "off") == 0) {
        ret = control_slashbar(fd, 0);
        if (ret >= 0) printf("Slashbar disabled\n");
    } else if (strcmp(argv[1], "static") == 0) {
        ret = set_slashbar_mode_static(fd);
        if (ret >= 0) printf("Slashbar set to static mode\n");
    } else {
        fprintf(stderr, "Invalid command. Use init, on, off, or static\n");
        ret = -1;
    }
    
    close(fd);
    return ret < 0 ? 1 : 0;
}