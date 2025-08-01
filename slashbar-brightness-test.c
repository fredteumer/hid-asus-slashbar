// Test different brightness levels and patterns
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define REPORT_ID 0x5A
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
    usleep(500000); // 500ms delay to see changes
    return ret;
}

int main() {
    int fd = open(HIDRAW_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open hidraw device");
        return 1;
    }
    
    printf("=== Testing Brightness Levels ===\n");
    printf("Look for LED changes on the laptop...\n\n");
    
    // Test different brightness values
    unsigned char brightness_tests[][8] = {
        {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // All FF
        {0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Test different first bytes
        {0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0xD0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Try D0 prefix
        {0xD1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0xD4, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
        {0xD8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    };
    
    for (int i = 0; i < 9; i++) {
        printf("Test %d: ", i + 1);
        send_packet(fd, brightness_tests[i], 8);
        printf("(Check for LED changes - Press Enter to continue)\n");
        getchar();
    }
    
    printf("\n=== Testing RGB Color Patterns ===\n");
    
    // Test RGB patterns
    unsigned char rgb_tests[][8] = {
        {0x01, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF}, // Red pattern
        {0x01, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00}, // Green pattern  
        {0x01, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00}, // Blue pattern
        {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // White pattern
        {0x02, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF}, // Red with prefix 02
        {0x10, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0xFF}, // Red with prefix 10
    };
    
    for (int i = 0; i < 6; i++) {
        printf("RGB Test %d: ", i + 1);
        send_packet(fd, rgb_tests[i], 8);
        printf("(Check for LED changes - Press Enter to continue)\n");
        getchar();
    }
    
    printf("\n=== Testing Simple On/Off ===\n");
    
    // Simple on/off tests
    printf("ALL ON: ");
    unsigned char all_on[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    send_packet(fd, all_on, 8);
    printf("(Press Enter to continue)\n");
    getchar();
    
    printf("ALL OFF: ");
    unsigned char all_off[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    send_packet(fd, all_off, 8);
    printf("(Press Enter to continue)\n");
    getchar();
    
    close(fd);
    printf("\nTest complete!\n");
    return 0;
}