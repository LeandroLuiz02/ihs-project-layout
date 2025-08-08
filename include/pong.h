#ifndef __PONG_H__
#define __PONG_H__

#include <SDL2/SDL.h>
#include <pthread.h>
#include <stdint.h>

// Game constants
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PADDLE_WIDTH 20
#define PADDLE_HEIGHT 100
#define BALL_SIZE 15
#define PADDLE_SPEED 5
#define BALL_SPEED 3

// Game states
typedef enum {
    GAME_MENU,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameState;

// Game objects
typedef struct {
    float x, y;
    float vel_x, vel_y;
} Ball;

typedef struct {
    float x, y;
    int score;
} Paddle;

// Game data shared between threads
typedef struct {
    GameState state;
    Ball ball;
    Paddle player1, player2;
    int winner;
    
    // Hardware state
    uint32_t switches;
    uint32_t buttons;
    int fpga_fd;
    
    // Synchronization
    pthread_mutex_t mutex;
    int running;
} GameData;

// Function prototypes
void* hardware_thread(void* arg);
int init_hardware(GameData* game);
void cleanup_hardware(GameData* game);
void update_leds(GameData* game);
void update_displays(GameData* game);

#endif /* __PONG_H__ */
