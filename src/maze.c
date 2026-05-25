#include "maze.h"
#include <stdlib.h>
#include <time.h>

void init_maze(Maze* maze, Config* config) {
    maze->width = config->maze_width;
    maze->height = config->maze_height;
    pthread_mutex_init(&maze->lock, NULL);
    
    maze->cells = (Cell**)malloc(maze->height * sizeof(Cell*));
    for (int i = 0; i < maze->height; i++) {
        maze->cells[i] = (Cell*)malloc(maze->width * sizeof(Cell));
    }
    
    srand(time(NULL));
    
    for (int y = 0; y < maze->height; y++) {
        for (int x = 0; x < maze->width; x++) {
            pthread_mutex_init(&maze->cells[y][x].lock, NULL);
            
            if (x == 0 && y == 0) {
                maze->cells[y][x].is_obstacle = false;
                maze->cells[y][x].bananas = 0;
                continue;
            }
            
            if ((x == 0 && y <= 2) || (y == 0 && x <= 2)) {
                maze->cells[y][x].is_obstacle = false;
                maze->cells[y][x].bananas = 0;
                continue;
            }
            
            double rand_val = (double)rand() / RAND_MAX;
            if (rand_val < config->obstacle_probability) {
                maze->cells[y][x].is_obstacle = true;
                maze->cells[y][x].bananas = 0;
            } else {
                maze->cells[y][x].is_obstacle = false;
                
                rand_val = (double)rand() / RAND_MAX;
                if (rand_val < config->banana_probability) {
                    int range = config->max_bananas_per_cell - config->min_bananas_per_cell + 1;
                    maze->cells[y][x].bananas = config->min_bananas_per_cell + (rand() % range);
                } else {
                    maze->cells[y][x].bananas = 0;
                }
            }
        }
    }
}

void destroy_maze(Maze* maze) {
    for (int y = 0; y < maze->height; y++) {
        for (int x = 0; x < maze->width; x++) {
            pthread_mutex_destroy(&maze->cells[y][x].lock);
        }
        free(maze->cells[y]);
    }
    free(maze->cells);
    pthread_mutex_destroy(&maze->lock);
}

bool is_valid_position(Maze* maze, Position pos) {
    return pos.x >= 0 && pos.x < maze->width && pos.y >= 0 && pos.y < maze->height;
}

bool is_obstacle(Maze* maze, Position pos) {
    if (!is_valid_position(maze, pos)) {
        return true;
    }
    
    pthread_mutex_lock(&maze->cells[pos.y][pos.x].lock);
    bool obstacle = maze->cells[pos.y][pos.x].is_obstacle;
    pthread_mutex_unlock(&maze->cells[pos.y][pos.x].lock);
    
    return obstacle;
}

int collect_bananas_from_cell(Maze* maze, Position pos, int amount) {
    if (!is_valid_position(maze, pos)) {
        return 0;
    }
    
    pthread_mutex_lock(&maze->cells[pos.y][pos.x].lock);
    
    if (maze->cells[pos.y][pos.x].is_obstacle) {
        pthread_mutex_unlock(&maze->cells[pos.y][pos.x].lock);
        return 0;
    }
    
    int available = maze->cells[pos.y][pos.x].bananas;
    int collected = (amount < available) ? amount : available;
    maze->cells[pos.y][pos.x].bananas -= collected;
    
    pthread_mutex_unlock(&maze->cells[pos.y][pos.x].lock);
    
    return collected;
}

int get_bananas_at_position(Maze* maze, Position pos) {
    if (!is_valid_position(maze, pos)) {
        return 0;
    }
    
    pthread_mutex_lock(&maze->cells[pos.y][pos.x].lock);
    int bananas = maze->cells[pos.y][pos.x].bananas;
    pthread_mutex_unlock(&maze->cells[pos.y][pos.x].lock);
    
    return bananas;
}
