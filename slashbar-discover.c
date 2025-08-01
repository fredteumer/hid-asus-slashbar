// Slashbar Discovery Tool - Try different report IDs and commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define HIDRAW_DEVICE "/dev/hidraw8"

int send_test_packet(int fd, unsigned char report_id, const unsigned char *payload, size_t payload_len) {
    unsigned char buf[64] = {0};
    buf[0] = report_id;
    if (payload_len > 0) {
        memcpy(&buf[1], payload, payload_len < 63 ? payload_len : 63);
    }
    
    printf("Testing Report ID 0x%02X with payload: ", report_id);
    for (int i = 0; i < (payload_len < 8 ? payload_len : 8); i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
    
    int ret = write(fd, buf, 64);
    usleep(100000); // 100ms delay
    
    return ret;
}

int main() {
    int fd = open(HIDRAW_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open hidraw device");
        return 1;
    }
    
    printf("=== ASUS Slashbar Discovery Tool ===\n");
    printf("Device: %s\n\n", HIDRAW_DEVICE);
    
    // Test different report IDs with basic enable command
    unsigned char test_reports[] = {0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61};
    unsigned char enable_cmd[] = {0xD8, 0x02, 0x00, 0x01, 0x00};
    
    printf("Testing different Report IDs:\n");
    for (int i = 0; i < sizeof(test_reports); i++) {
        send_test_packet(fd, test_reports[i], enable_cmd, sizeof(enable_cmd));
        
        // Try to read response
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000; // 50ms
        
        if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            unsigned char response[64];
            int len = read(fd, response, sizeof(response));
            if (len > 0) {
                printf("  -> Got response (%d bytes): ", len);
                for (int j = 0; j < (len < 16 ? len : 16); j++) {
                    printf("%02X ", response[j]);
                }
                printf("\n");
            }
        }
    }
    
    printf("\nTesting different command patterns:\n");
    
    // Test with Report ID 0x5D (original)
    unsigned char commands[][8] = {
        {0xD1, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}, // Wake command variant
        {0xD2, 0x02, 0x01, 0x08, 0xAB, 0x00, 0x00, 0x00}, // Init command  
        {0xD4, 0x02, 0x06, 0x01, 0xFF, 0xFF, 0xFF, 0x00}, // Mode static
        {0xD7, 0x00, 0x00, 0x01, 0xAC, 0x00, 0x00, 0x00}, // Init variant
        {0xD8, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}, // Enable
        {0xD8, 0x02, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00}, // Disable
    };
    
    for (int i = 0; i < 6; i++) {
        send_test_packet(fd, 0x5D, commands[i], 8);
        
        // Check for response
        fd_set readfds;
        struct timeval timeout;
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;
        
        if (select(fd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            unsigned char response[64];
            int len = read(fd, response, sizeof(response));
            if (len > 0) {
                printf("  -> Response: ");
                for (int j = 0; j < (len < 16 ? len : 16); j++) {
                    printf("%02X ", response[j]);
                }
                printf("\n");
            }
        }
    }
    
    close(fd);
    printf("\nDiscovery complete. Check dmesg for any kernel messages.\n");
    return 0;
}