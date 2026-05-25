#ifndef MAZE_H
#define MAZE_H

#include "types.h"

void init_maze(Maze* maze, Config* config);
void destroy_maze(Maze* maze);
bool is_valid_position(Maze* maze, Position pos);
bool is_obstacle(Maze* maze, Position pos);
int collect_bananas_from_cell(Maze* maze, Position pos, int amount);
int get_bananas_at_position(Maze* maze, Position pos);

#endif
