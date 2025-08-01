// Monitor HID traffic to capture actual working commands
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define HIDRAW_DEVICE "/dev/hidraw8"

void print_timestamp() {
    time_t now;
    struct tm *tm_info;
    char buffer[26];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(buffer, 26, "%H:%M:%S", tm_info);
    printf("[%s] ", buffer);
}

int main() {
    int fd = open(HIDRAW_DEVICE, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("Failed to open hidraw device");
        return 1;
    }
    
    printf("=== HID Traffic Monitor ===\n");
    printf("Monitoring %s for incoming data...\n", HIDRAW_DEVICE);
    printf("NOW PLUG/UNPLUG YOUR CHARGER TO TRIGGER SLASHBAR!\n");
    printf("Press Ctrl+C to stop monitoring\n\n");
    
    unsigned char buffer[64];
    int bytes_read;
    int packet_count = 0;
    
    while (1) {
        bytes_read = read(fd, buffer, sizeof(buffer));
        
        if (bytes_read > 0) {
            packet_count++;
            print_timestamp();
            printf("Packet #%d (%d bytes): ", packet_count, bytes_read);
            
            for (int i = 0; i < bytes_read; i++) {
                printf("%02X ", buffer[i]);
            }
            printf("\n");
            
            // Check for interesting patterns
            if (buffer[0] == 0x5A || buffer[0] == 0x5D) {
                printf("  -> Slashbar Report ID detected!\n");
            }
            if (bytes_read > 5 && (buffer[1] == 0xD8 || buffer[1] == 0xD4 || buffer[1] == 0xD1)) {
                printf("  -> Possible slashbar command pattern!\n");
            }
        }
        
        usleep(1000); // 1ms delay
    }
    
    close(fd);
    return 0;
}