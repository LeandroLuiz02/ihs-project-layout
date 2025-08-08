#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>

#include "ioctl_cmds.h"
#include "display.h"
#include "pong.h"

// Initialize hardware connection
int init_hardware(GameData* game) {
    printf("Initializing FPGA hardware....\n");
    
    // Try to open the PCI device file
    game->fpga_fd = open("/dev/de2i-150", O_RDWR);
    if (game->fpga_fd < 0) {
        printf("Failed to open FPGA device: %s\n", strerror(errno));
        printf("Make sure driver is loaded and device file exists\n");
        return -1;
    }
    
    printf("FPGA device opened successfully (fd=%d)\n", game->fpga_fd);
    
    // Test initial communication with VISIBLE patterns
    printf("=== TESTING HARDWARE WITH VISIBLE PATTERNS ===\n");

    // Test red LEDs with a visible pattern
    printf("Testing WR_RED_LEDS with visible pattern...\n");
    if (ioctl(game->fpga_fd, WR_RED_LEDS) < 0) {
        printf("❌ IOCTL WR_RED_LEDS failed: %s\n", strerror(errno));
    } else {
        printf("✓ IOCTL WR_RED_LEDS success\n");
        uint32_t red_pattern = 0xAAAAAAAA; // Alternating pattern
        ssize_t written = write(game->fpga_fd, &red_pattern, sizeof(red_pattern));
        printf("✓ Red LEDs written: %ld bytes, pattern=0x%X\n", written, red_pattern);
        printf(">> CHECK: Red LEDs should show alternating pattern!\n");
    }

    // Wait to see the effect
    sleep(2);

    // Test green LEDs
    printf("Testing WR_GREEN_LEDS with visible pattern...\n");
    if (ioctl(game->fpga_fd, WR_GREEN_LEDS) < 0) {
        printf("❌ IOCTL WR_GREEN_LEDS failed: %s\n", strerror(errno));
    } else {
        printf("✓ IOCTL WR_GREEN_LEDS success\n");
        uint32_t green_pattern = 0x55555555; // Different alternating pattern
        ssize_t written = write(game->fpga_fd, &green_pattern, sizeof(green_pattern));
        printf("✓ Green LEDs written: %ld bytes, pattern=0x%X\n", written, green_pattern);
        printf(">> CHECK: Green LEDs should show different pattern!\n");
    }

    sleep(2);

    // Test 7-segment displays with number
    printf("Testing displays with number 8...\n");
    uint32_t display_8 = 0xFFFFFF80; // Number 8
    if (ioctl(game->fpga_fd, WR_L_DISPLAY) >= 0) {
        write(game->fpga_fd, &display_8, sizeof(display_8));
        printf("✓ Left display should show '8'\n");
    }
    if (ioctl(game->fpga_fd, WR_R_DISPLAY) >= 0) {
        write(game->fpga_fd, &display_8, sizeof(display_8));
        printf("✓ Right display should show '8'\n");
    }

    printf("=== HARDWARE TEST COMPLETE ===\n");
    printf("If you see the patterns/numbers, hardware is working!\n");
    printf("Hardware initialization complete\n");
    return 0;
}

// Cleanup hardware resources
void cleanup_hardware(GameData* game) {
    if (game->fpga_fd >= 0) {
        printf("Cleaning up FPGA hardware...\n");
        
        // Turn off all LEDs and displays before closing
        uint32_t zero = 0;
        uint32_t display_off = 0xFFFFFFFF;
        
        ioctl(game->fpga_fd, WR_RED_LEDS);
        write(game->fpga_fd, &zero, sizeof(zero));
        
        ioctl(game->fpga_fd, WR_GREEN_LEDS);
        write(game->fpga_fd, &zero, sizeof(zero));
        
        ioctl(game->fpga_fd, WR_L_DISPLAY);
        write(game->fpga_fd, &display_off, sizeof(display_off));
        
        ioctl(game->fpga_fd, WR_R_DISPLAY);
        write(game->fpga_fd, &display_off, sizeof(display_off));
        
        close(game->fpga_fd);
        printf("FPGA device closed\n");
    }
}

// Update LEDs based on game state
void update_leds(GameData* game) {
    if (game->fpga_fd < 0) return;
    
    uint32_t red_pattern = 0;
    uint32_t green_pattern = 0;
    
    switch (game->state) {
        case GAME_MENU:
            // Blinking pattern for menu
            green_pattern = 0x55555555; // Alternating LEDs
            break;
            
        case GAME_PLAYING:
            // Show ball position with LEDs
            // Red LEDs represent ball X position (left side)
            // Green LEDs represent ball Y position (relative)
            red_pattern = (uint32_t)(game->ball.x / WINDOW_WIDTH * 32) & 0xFFFFFFFF;
            green_pattern = (uint32_t)(game->ball.y / WINDOW_HEIGHT * 32) & 0xFFFFFFFF;
            break;
            
        case GAME_PAUSED:
            // All red for pause
            red_pattern = 0xFFFFFFFF;
            break;
            
        case GAME_OVER:
            // Victory pattern - winner's color
            if (game->winner == 1) {
                green_pattern = 0xFFFFFFFF; // Player 1 wins
            } else {
                red_pattern = 0xFFFFFFFF;   // Player 2 wins
            }
            break;
    }
    
    // Write to hardware - CRITICAL: Always call ioctl() before write()
    if (ioctl(game->fpga_fd, WR_RED_LEDS) >= 0) {
        write(game->fpga_fd, &red_pattern, sizeof(red_pattern));
    }
    
    if (ioctl(game->fpga_fd, WR_GREEN_LEDS) >= 0) {
        write(game->fpga_fd, &green_pattern, sizeof(green_pattern));
    }
}

// Convert score to 7-segment display pattern
uint32_t score_to_display(int score) {
    uint32_t patterns[] = {
        HEX_0, HEX_1, HEX_2, HEX_3, HEX_4,
        HEX_5, HEX_6, HEX_7, HEX_8, HEX_9
    };
    
    if (score >= 0 && score <= 9) {
        return patterns[score];
    }
    
    // For scores > 9, show last digit
    return patterns[score % 10];
}

// Update 7-segment displays with scores
void update_displays(GameData* game) {
    if (game->fpga_fd < 0) return;
    
    // Left display shows Player 1 score
    uint32_t left_pattern = score_to_display(game->player1.score);
    
    // Right display shows Player 2 score  
    uint32_t right_pattern = score_to_display(game->player2.score);
    
    // Write to hardware - CRITICAL: Always call ioctl() before write()
    if (ioctl(game->fpga_fd, WR_L_DISPLAY) >= 0) {
        write(game->fpga_fd, &left_pattern, sizeof(left_pattern));
    }
    
    if (ioctl(game->fpga_fd, WR_R_DISPLAY) >= 0) {
        write(game->fpga_fd, &right_pattern, sizeof(right_pattern));
    }
}

// Read switches and buttons (for future features)
void read_hardware_inputs(GameData* game) {
    if (game->fpga_fd < 0) return;
    
    uint32_t data;
    
    // Read switches
    if (ioctl(game->fpga_fd, RD_SWITCHES) >= 0) {
        if (read(game->fpga_fd, &data, sizeof(data)) > 0) {
            game->switches = data;
        }
    }
    
    // Read push buttons
    if (ioctl(game->fpga_fd, RD_PBUTTONS) >= 0) {
        if (read(game->fpga_fd, &data, sizeof(data)) > 0) {
            game->buttons = data;
        }
    }
    
    // You can use switches/buttons for:
    // - Switch 0: Change ball speed
    // - Switch 1: Enable/disable AI for player 2
    // - Button 0: Pause game
    // - Button 1: Reset game
}

// Hardware monitoring thread
void* hardware_thread(void* arg) {
    GameData* game = (GameData*)arg;
    
    printf("Hardware thread started\n");
    
    while (game->running) {
        pthread_mutex_lock(&game->mutex);
        
        // Read inputs
        read_hardware_inputs(game);
        
        // Update outputs
        update_leds(game);
        update_displays(game);
        
        pthread_mutex_unlock(&game->mutex);
        
        // Update at 30Hz to avoid overwhelming the hardware
        usleep(33333); // ~30 FPS
    }
    
    printf("Hardware thread finished\n");
    return NULL;
}

