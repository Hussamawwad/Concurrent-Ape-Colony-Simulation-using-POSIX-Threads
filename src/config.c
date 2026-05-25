#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_config(const char* filename, Config* config) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open configuration file '%s'\n", filename);
        return -1;
    }
    
    char line[256];
    int params_loaded = 0;
    
    while (fgets(line, sizeof(line), file)) {
        char key[128];
        char value[128];
        
        if (sscanf(line, "%s %s", key, value) == 2) {
            if (strcmp(key, "maze_width") == 0) {
                config->maze_width = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "maze_height") == 0) {
                config->maze_height = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "obstacle_probability") == 0) {
                config->obstacle_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "banana_probability") == 0) {
                config->banana_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "min_bananas_per_cell") == 0) {
                config->min_bananas_per_cell = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "max_bananas_per_cell") == 0) {
                config->max_bananas_per_cell = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "num_families") == 0) {
                config->num_families = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "babies_per_family") == 0) {
                config->babies_per_family = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_target_bananas") == 0) {
                config->female_target_bananas = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_initial_energy") == 0) {
                config->female_initial_energy = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_energy_per_step") == 0) {
                config->female_energy_per_step = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_energy_per_fight") == 0) {
                config->female_energy_per_fight = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_rest_threshold") == 0) {
                config->female_rest_threshold = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_rest_duration_ms") == 0) {
                config->female_rest_duration_ms = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_move_delay_ms") == 0) {
                config->female_move_delay_ms = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "female_exit_fight_probability") == 0) {
                config->female_exit_fight_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "male_initial_energy") == 0) {
                config->male_initial_energy = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "male_energy_per_guard_cycle") == 0) {
                config->male_energy_per_guard_cycle = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "male_energy_per_fight") == 0) {
                config->male_energy_per_fight = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "male_fight_base_probability") == 0) {
                config->male_fight_base_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "male_fight_probability_increment") == 0) {
                config->male_fight_probability_increment = atof(value);
                params_loaded++;
            } else if (strcmp(key, "male_fight_check_delay_ms") == 0) {
                config->male_fight_check_delay_ms = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "male_guard_delay_ms") == 0) {
                config->male_guard_delay_ms = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "baby_steal_probability") == 0) {
                config->baby_steal_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "baby_eat_probability") == 0) {
                config->baby_eat_probability = atof(value);
                params_loaded++;
            } else if (strcmp(key, "baby_fight_participation_distance") == 0) {
                config->baby_fight_participation_distance = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "max_babies_per_fight") == 0) {
                config->max_babies_per_fight = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "simulation_max_time_seconds") == 0) {
                config->simulation_max_time_seconds = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "withdrawn_families_threshold") == 0) {
                config->withdrawn_families_threshold = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "family_basket_threshold") == 0) {
                config->family_basket_threshold = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "baby_eaten_threshold") == 0) {
                config->baby_eaten_threshold = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "window_width") == 0) {
                config->window_width = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "window_height") == 0) {
                config->window_height = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "cell_size") == 0) {
                config->cell_size = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "update_interval_ms") == 0) {
                config->update_interval_ms = atoi(value);
                params_loaded++;
            } else if (strcmp(key, "color_background_r") == 0) {
                config->color_background_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_background_g") == 0) {
                config->color_background_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_background_b") == 0) {
                config->color_background_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_obstacle_r") == 0) {
                config->color_obstacle_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_obstacle_g") == 0) {
                config->color_obstacle_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_obstacle_b") == 0) {
                config->color_obstacle_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_banana_r") == 0) {
                config->color_banana_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_banana_g") == 0) {
                config->color_banana_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_banana_b") == 0) {
                config->color_banana_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_female_r") == 0) {
                config->color_female_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_female_g") == 0) {
                config->color_female_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_female_b") == 0) {
                config->color_female_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_male_r") == 0) {
                config->color_male_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_male_g") == 0) {
                config->color_male_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_male_b") == 0) {
                config->color_male_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_baby_r") == 0) {
                config->color_baby_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_baby_g") == 0) {
                config->color_baby_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_baby_b") == 0) {
                config->color_baby_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_basket_r") == 0) {
                config->color_basket_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_basket_g") == 0) {
                config->color_basket_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_basket_b") == 0) {
                config->color_basket_b = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_grid_r") == 0) {
                config->color_grid_r = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_grid_g") == 0) {
                config->color_grid_g = atof(value);
                params_loaded++;
            } else if (strcmp(key, "color_grid_b") == 0) {
                config->color_grid_b = atof(value);
                params_loaded++;
            }
        }
    }
    
    fclose(file);
    
    if (params_loaded < 60) {
        fprintf(stderr, "Error: Configuration file is incomplete (loaded %d/60 parameters)\n", params_loaded);
        return -1;
    }
    
    printf("Configuration loaded successfully: %d parameters\n", params_loaded);
    
    if (config->maze_width <= 0 || config->maze_height <= 0 ||
        config->num_families <= 0 || config->babies_per_family < 0) {
        fprintf(stderr, "Error: Invalid configuration values\n");
        return -1;
    }
    
    return 0;
}
