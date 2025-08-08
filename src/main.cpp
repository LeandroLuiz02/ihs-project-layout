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
#include <SDL2/SDL.h>

#include "ioctl_cmds.h"
#include "display.h"
#include "pong.h"

// Global game data
GameData game_data;

// Initialize SDL and create window
int init_graphics(SDL_Window** window, SDL_Renderer** renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL init failed: %s\n", SDL_GetError());
        return -1;
    }
    
    *window = SDL_CreateWindow("FPGA Pong", 
                              SDL_WINDOWPOS_CENTERED, 
                              SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, 
                              SDL_WINDOW_SHOWN);
    if (!*window) {
        printf("Window creation failed: %s\n", SDL_GetError());
        return -1;
    }
    
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
    if (!*renderer) {
        printf("Renderer creation failed: %s\n", SDL_GetError());
        return -1;
    }
    
    return 0;
}

// Initialize game objects
void init_game(GameData* game) {
    pthread_mutex_init(&game->mutex, NULL);
    
    game->state = GAME_MENU;
    game->running = 1;
    
    // Initialize ball at center
    game->ball.x = WINDOW_WIDTH / 2;
    game->ball.y = WINDOW_HEIGHT / 2;
    game->ball.vel_x = BALL_SPEED;
    game->ball.vel_y = BALL_SPEED;
    
    // Initialize paddles
    game->player1.x = 50;
    game->player1.y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    game->player1.score = 0;
    
    game->player2.x = WINDOW_WIDTH - 50 - PADDLE_WIDTH;
    game->player2.y = WINDOW_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    game->player2.score = 0;
}

// Update game logic
void update_game(GameData* game) {
    pthread_mutex_lock(&game->mutex);
    
    if (game->state != GAME_PLAYING) {
        pthread_mutex_unlock(&game->mutex);
        return;
    }
    
    // Update ball position
    game->ball.x += game->ball.vel_x;
    game->ball.y += game->ball.vel_y;
    
    // Ball collision with top/bottom walls
    if (game->ball.y <= 0 || game->ball.y >= WINDOW_HEIGHT - BALL_SIZE) {
        game->ball.vel_y = -game->ball.vel_y;
    }
    
    // Ball collision with paddles
    SDL_Rect ball_rect = {(int)game->ball.x, (int)game->ball.y, BALL_SIZE, BALL_SIZE};
    SDL_Rect p1_rect = {(int)game->player1.x, (int)game->player1.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_Rect p2_rect = {(int)game->player2.x, (int)game->player2.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    
    if (SDL_HasIntersection(&ball_rect, &p1_rect) || 
        SDL_HasIntersection(&ball_rect, &p2_rect)) {
        game->ball.vel_x = -game->ball.vel_x;
    }
    
    // Score detection
    if (game->ball.x < 0) {
        game->player2.score++;
        game->ball.x = WINDOW_WIDTH / 2;
        game->ball.y = WINDOW_HEIGHT / 2;
        game->ball.vel_x = BALL_SPEED;
    }
    
    if (game->ball.x > WINDOW_WIDTH) {
        game->player1.score++;
        game->ball.x = WINDOW_WIDTH / 2;
        game->ball.y = WINDOW_HEIGHT / 2;
        game->ball.vel_x = -BALL_SPEED;
    }
    
    // Check for winner
    if (game->player1.score >= 5 || game->player2.score >= 5) {
        game->state = GAME_OVER;
        game->winner = (game->player1.score >= 5) ? 1 : 2;
    }
    
    pthread_mutex_unlock(&game->mutex);
}

// Handle keyboard input
void handle_input(GameData* game, const Uint8* keystate) {
    pthread_mutex_lock(&game->mutex);
    
    // Player 1 controls (W/S)
    if (keystate[SDL_SCANCODE_W] && game->player1.y > 0) {
        game->player1.y -= PADDLE_SPEED;
    }
    if (keystate[SDL_SCANCODE_S] && game->player1.y < WINDOW_HEIGHT - PADDLE_HEIGHT) {
        game->player1.y += PADDLE_SPEED;
    }
    
    // Player 2 controls (Arrow keys)
    if (keystate[SDL_SCANCODE_UP] && game->player2.y > 0) {
        game->player2.y -= PADDLE_SPEED;
    }
    if (keystate[SDL_SCANCODE_DOWN] && game->player2.y < WINDOW_HEIGHT - PADDLE_HEIGHT) {
        game->player2.y += PADDLE_SPEED;
    }
    
    // Game state controls
    if (keystate[SDL_SCANCODE_SPACE]) {
        if (game->state == GAME_MENU) {
            game->state = GAME_PLAYING;
        } else if (game->state == GAME_PLAYING) {
            game->state = GAME_PAUSED;
        } else if (game->state == GAME_PAUSED) {
            game->state = GAME_PLAYING;
        }
    }
    
    if (keystate[SDL_SCANCODE_R] && game->state == GAME_OVER) {
        init_game(game);
    }
    
    pthread_mutex_unlock(&game->mutex);
}

// Render game
void render_game(SDL_Renderer* renderer, GameData* game) {
    pthread_mutex_lock(&game->mutex);
    
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    // Draw white objects
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    
    // Draw paddles
    SDL_Rect p1_rect = {(int)game->player1.x, (int)game->player1.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_Rect p2_rect = {(int)game->player2.x, (int)game->player2.y, PADDLE_WIDTH, PADDLE_HEIGHT};
    SDL_RenderFillRect(renderer, &p1_rect);
    SDL_RenderFillRect(renderer, &p2_rect);
    
    // Draw ball
    SDL_Rect ball_rect = {(int)game->ball.x, (int)game->ball.y, BALL_SIZE, BALL_SIZE};
    SDL_RenderFillRect(renderer, &ball_rect);
    
    // Draw center line
    for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
        SDL_Rect line_rect = {WINDOW_WIDTH/2 - 2, y, 4, 10};
        SDL_RenderFillRect(renderer, &line_rect);
    }
    
    pthread_mutex_unlock(&game->mutex);
    
    SDL_RenderPresent(renderer);
}

int main(int argc, char** argv) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    SDL_Event event;
    pthread_t hardware_thread_id;
    
    printf("FPGA Pong Game Starting...\n");
    
    // Initialize game
    init_game(&game_data);
    
    // Initialize hardware
    if (init_hardware(&game_data) < 0) {
        printf("Warning: Hardware initialization failed, continuing without FPGA features\n");
    }
    
    // Initialize graphics
    if (init_graphics(&window, &renderer) < 0) {
        return -1;
    }
    
    // Start hardware thread
    pthread_create(&hardware_thread_id, NULL, hardware_thread, &game_data);
    
    printf("Game initialized. Press SPACE to start, W/S and UP/DOWN to control paddles\n");
    
    // Main game loop
    while (game_data.running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                game_data.running = 0;
            }
        }
        
        // Handle continuous keyboard input
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        handle_input(&game_data, keystate);
        
        // Update game logic
        update_game(&game_data);
        
        // Render
        render_game(renderer, &game_data);
        
        // Control frame rate
        SDL_Delay(16); // ~60 FPS
    }
    
    // Cleanup
    pthread_join(hardware_thread_id, NULL);
    cleanup_hardware(&game_data);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    pthread_mutex_destroy(&game_data.mutex);
    
    printf("Game finished.\n");
    return 0;
}
