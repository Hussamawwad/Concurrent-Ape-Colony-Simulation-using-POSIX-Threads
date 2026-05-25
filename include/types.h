#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>
#include <stdbool.h>

typedef struct FamilyBasket FamilyBasket;
typedef struct FightContext FightContext;

typedef struct {
    int bananas;
    bool is_obstacle;
    pthread_mutex_t lock;
} Cell;

typedef struct {
    int x;
    int y;
} Position;

typedef enum {
    APE_FEMALE,
    APE_MALE,
    APE_BABY
} ApeType;

typedef enum {
    STATE_ENTERING,
    STATE_COLLECTING,
    STATE_EXITING,
    STATE_RESTING,
    STATE_WITHDRAWN,
    STATE_FIGHTING
} ApeState;

typedef struct {
    int family_id;
    ApeType type;
    int id;
    Position pos;
    int collected_bananas;
    int eaten_bananas;
    int energy;
    ApeState state;
    pthread_t thread;
    bool active;
    bool in_fight;
    pthread_mutex_t fight_lock;
} Ape;

struct FightContext {
    int family_a;
    int family_b;
    FamilyBasket* basket_a;
    FamilyBasket* basket_b;
    bool active;
    int babies_participated;
    int max_babies;
    pthread_mutex_t fight_mutex;
    pthread_cond_t fight_cond;
    FightContext* next;
};

struct FamilyBasket {
    int family_id;
    int bananas;
    Position pos;
    pthread_mutex_t lock;
    bool withdrawn;
    bool fight_in_progress;
    pthread_cond_t fight_cond;
};

typedef struct {
    Cell** cells;
    int width;
    int height;
    pthread_mutex_t lock;
} Maze;

typedef struct {
    int maze_width;
    int maze_height;
    double obstacle_probability;
    double banana_probability;
    int min_bananas_per_cell;
    int max_bananas_per_cell;
    
    int num_families;
    int babies_per_family;
    
    int female_target_bananas;
    int female_initial_energy;
    int female_energy_per_step;
    int female_energy_per_fight;
    int female_rest_threshold;
    int female_rest_duration_ms;
    int female_move_delay_ms;
    double female_exit_fight_probability;
    
    int male_initial_energy;
    int male_energy_per_guard_cycle;
    int male_energy_per_fight;
    double male_fight_base_probability;
    double male_fight_probability_increment;
    int male_fight_check_delay_ms;
    int male_guard_delay_ms;
    
    double baby_steal_probability;
    double baby_eat_probability;
    int baby_fight_participation_distance;
    int max_babies_per_fight;
    
    int simulation_max_time_seconds;
    int withdrawn_families_threshold;
    int family_basket_threshold;
    int baby_eaten_threshold;
    
    int window_width;
    int window_height;
    int cell_size;
    int update_interval_ms;
    
    float color_background_r, color_background_g, color_background_b;
    float color_obstacle_r, color_obstacle_g, color_obstacle_b;
    float color_banana_r, color_banana_g, color_banana_b;
    float color_female_r, color_female_g, color_female_b;
    float color_male_r, color_male_g, color_male_b;
    float color_baby_r, color_baby_g, color_baby_b;
    float color_basket_r, color_basket_g, color_basket_b;
    float color_grid_r, color_grid_g, color_grid_b;
} Config;

typedef struct {
    Config config;
    Maze maze;
    Ape** apes;
    int total_apes;
    FamilyBasket* baskets;
    bool simulation_running;
    pthread_mutex_t sim_lock;
    time_t start_time;
    int withdrawn_families_count;
    FightContext* active_fights;
    pthread_mutex_t fights_lock;
} Simulation;

#endif
