// ASUS Slashbar Helper v2 - Using correct Report ID 0x5A
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define REPORT_ID 0x5A  // Discovered correct report ID!
#define HIDRAW_DEVICE "/dev/hidraw8"

int send_packet(int fd, const unsigned char *payload, size_t payload_len) {
    unsigned char buf[64] = {0};
    buf[0] = REPORT_ID;
    if (payload_len > 0) {
        memcpy(&buf[1], payload, payload_len < 63 ? payload_len : 63);
    }
    
    printf("Sending: ");
    for (int i = 0; i < (payload_len + 1 < 16 ? payload_len + 1 : 16); i++) {
        printf("%02X ", buf[i]);
    }
    printf("\n");
    
    int ret = write(fd, buf, 64);
    usleep(50000); // 50ms delay
    
    // Try to read response
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000; // 100ms
    
    if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
        unsigned char response[64];
        int len = read(fd, response, sizeof(response));
        if (len > 0) {
            printf("Response: ");
            for (int j = 0; j < (len < 16 ? len : 16); j++) {
                printf("%02X ", response[j]);
            }
            printf("\n");
        }
    }
    
    return ret;
}

int initialize_slashbar(int fd) {
    printf("=== Initializing Slashbar ===\n");
    
    // Wake-up sequence with 0x5A
    const unsigned char wakeup1[] = "ASUS Tech.Inc.";
    send_packet(fd, wakeup1, strlen((char*)wakeup1));
    
    const unsigned char wakeup2[] = {0xC2};
    send_packet(fd, wakeup2, sizeof(wakeup2));
    
    const unsigned char wakeup3[] = {0xD1, 0x01, 0x00, 0x01};
    send_packet(fd, wakeup3, sizeof(wakeup3));
    
    // Initialization sequence
    const unsigned char init1[] = {0xD7, 0x00, 0x00, 0x01, 0xAC};
    send_packet(fd, init1, sizeof(init1));
    
    const unsigned char init2[] = {0xD2, 0x02, 0x01, 0x08, 0xAB};
    send_packet(fd, init2, sizeof(init2));
    
    return 0;
}

int control_slashbar(int fd, int enable) {
    printf("=== %s Slashbar ===\n", enable ? "Enabling" : "Disabling");
    const unsigned char cmd[] = {0xD8, 0x02, 0x00, 0x01, enable ? 0x00 : 0x80};
    return send_packet(fd, cmd, sizeof(cmd));
}

int set_slashbar_static(int fd) {
    printf("=== Setting Static Mode ===\n");
    // Try different static mode commands
    const unsigned char mode1[] = {0xD4, 0x02, 0x06, 0x01, 0xFF, 0xFF, 0xFF};
    send_packet(fd, mode1, sizeof(mode1));
    
    const unsigned char mode2[] = {0xD3, 0x02, 0x06, 0x01, 0xFF};
    send_packet(fd, mode2, sizeof(mode2));
    
    const unsigned char enable[] = {0xD8, 0x02, 0x00, 0x01, 0x00};
    return send_packet(fd, enable, sizeof(enable));
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <init|on|off|static|test>\n", argv[0]);
        return 1;
    }
    
    int fd = open(HIDRAW_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open hidraw device");
        return 1;
    }
    
    printf("=== ASUS Slashbar Helper v2 (Report ID 0x%02X) ===\n", REPORT_ID);
    
    int ret = 0;
    if (strcmp(argv[1], "init") == 0) {
        ret = initialize_slashbar(fd);
    } else if (strcmp(argv[1], "on") == 0) {
        ret = control_slashbar(fd, 1);
    } else if (strcmp(argv[1], "off") == 0) {
        ret = control_slashbar(fd, 0);
    } else if (strcmp(argv[1], "static") == 0) {
        ret = set_slashbar_static(fd);
    } else if (strcmp(argv[1], "test") == 0) {
        printf("=== Test Mode - Simple Enable ===\n");
        const unsigned char test[] = {0x01, 0xFF};
        ret = send_packet(fd, test, sizeof(test));
    } else {
        fprintf(stderr, "Invalid command. Use init, on, off, static, or test\n");
        ret = -1;
    }
    
    close(fd);
    return ret < 0 ? 1 : 0;
}