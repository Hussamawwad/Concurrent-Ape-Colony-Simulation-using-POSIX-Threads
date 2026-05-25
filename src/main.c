#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "types.h"
#include "config.h"
#include "maze.h"
#include "apes.h"
#include "graphics.h"

Simulation* g_sim = NULL;

static void init_simulation(Simulation* sim, const char* config_file) {
    if (load_config(config_file, &sim->config) != 0) {
        fprintf(stderr, "Failed to load configuration. Exiting.\n");
        exit(1);
    }
    
    pthread_mutex_init(&sim->sim_lock, NULL);
    sim->simulation_running = true;
    sim->start_time = time(NULL);
    sim->withdrawn_families_count = 0;
    sim->active_fights = NULL;
    pthread_mutex_init(&sim->fights_lock, NULL);
    
    init_maze(&sim->maze, &sim->config);
    
    sim->baskets = (FamilyBasket*)malloc(sim->config.num_families * sizeof(FamilyBasket));
    for (int i = 0; i < sim->config.num_families; i++) {
        sim->baskets[i].family_id = i;
        sim->baskets[i].bananas = 0;
        sim->baskets[i].withdrawn = false;
        sim->baskets[i].fight_in_progress = false;
        pthread_mutex_init(&sim->baskets[i].lock, NULL);
        pthread_cond_init(&sim->baskets[i].fight_cond, NULL);
        
        // Place baskets at bottom of maze, ensuring they're not on obstacles
        int x = (i % (sim->maze.width / 2)) * 2;
        int y = sim->maze.height - 1 - (i / (sim->maze.width / 2)) * 2;
        
        // Make sure basket position is not an obstacle
        if (sim->maze.cells[y][x].is_obstacle) {
            sim->maze.cells[y][x].is_obstacle = false;
            sim->maze.cells[y][x].bananas = 0;
        }
        
        sim->baskets[i].pos.x = x;
        sim->baskets[i].pos.y = y;
        
        printf("Family %d basket placed at (%d,%d)\n", i, x, y);
    }
    
    int apes_per_family = 2 + sim->config.babies_per_family;
    sim->total_apes = sim->config.num_families * apes_per_family;
    sim->apes = (Ape**)malloc(sim->total_apes * sizeof(Ape*));
    
    int ape_index = 0;
    for (int fam = 0; fam < sim->config.num_families; fam++) {
        sim->apes[ape_index] = (Ape*)malloc(sizeof(Ape));
        sim->apes[ape_index]->family_id = fam;
        sim->apes[ape_index]->type = APE_FEMALE;
        sim->apes[ape_index]->id = ape_index;
        sim->apes[ape_index]->collected_bananas = 0;
        sim->apes[ape_index]->eaten_bananas = 0;
        sim->apes[ape_index]->energy = sim->config.female_initial_energy;
        sim->apes[ape_index]->active = true;
        sim->apes[ape_index]->state = STATE_ENTERING;
        sim->apes[ape_index]->in_fight = false;
        pthread_mutex_init(&sim->apes[ape_index]->fight_lock, NULL);
        ape_index++;
        
        sim->apes[ape_index] = (Ape*)malloc(sizeof(Ape));
        sim->apes[ape_index]->family_id = fam;
        sim->apes[ape_index]->type = APE_MALE;
        sim->apes[ape_index]->id = ape_index;
        sim->apes[ape_index]->collected_bananas = 0;
        sim->apes[ape_index]->eaten_bananas = 0;
        sim->apes[ape_index]->energy = sim->config.male_initial_energy;
        sim->apes[ape_index]->active = true;
        sim->apes[ape_index]->state = STATE_COLLECTING;
        sim->apes[ape_index]->in_fight = false;
        pthread_mutex_init(&sim->apes[ape_index]->fight_lock, NULL);
        ape_index++;
        
        for (int baby = 0; baby < sim->config.babies_per_family; baby++) {
            sim->apes[ape_index] = (Ape*)malloc(sizeof(Ape));
            sim->apes[ape_index]->family_id = fam;
            sim->apes[ape_index]->type = APE_BABY;
            sim->apes[ape_index]->id = ape_index;
            sim->apes[ape_index]->collected_bananas = 0;
            sim->apes[ape_index]->eaten_bananas = 0;
            sim->apes[ape_index]->energy = 0;
            sim->apes[ape_index]->active = true;
            sim->apes[ape_index]->state = STATE_ENTERING;
            sim->apes[ape_index]->in_fight = false;
            pthread_mutex_init(&sim->apes[ape_index]->fight_lock, NULL);
            ape_index++;
        }
    }
}

static void start_ape_threads(Simulation* sim) {
    for (int i = 0; i < sim->total_apes; i++) {
        if (sim->apes[i]->type == APE_FEMALE) {
            pthread_create(&sim->apes[i]->thread, NULL, female_ape_thread, sim->apes[i]);
        } else if (sim->apes[i]->type == APE_MALE) {
            pthread_create(&sim->apes[i]->thread, NULL, male_ape_thread, sim->apes[i]);
        } else if (sim->apes[i]->type == APE_BABY) {
            pthread_create(&sim->apes[i]->thread, NULL, baby_ape_thread, sim->apes[i]);
        }
    }
}

static bool check_termination_conditions(Simulation* sim) {
    time_t current_time = time(NULL);
    if (difftime(current_time, sim->start_time) > sim->config.simulation_max_time_seconds) {
        printf("Simulation ended: Time limit exceeded\n");
        return true;
    }
    
    if (sim->withdrawn_families_count >= sim->config.withdrawn_families_threshold) {
        printf("Simulation ended: Too many families withdrawn (%d)\n", sim->withdrawn_families_count);
        return true;
    }
    
    for (int i = 0; i < sim->config.num_families; i++) {
        pthread_mutex_lock(&sim->baskets[i].lock);
        int bananas = sim->baskets[i].bananas;
        pthread_mutex_unlock(&sim->baskets[i].lock);
        
        if (bananas >= sim->config.family_basket_threshold) {
            printf("Simulation ended: Family %d exceeded basket threshold (%d bananas)\n", i, bananas);
            return true;
        }
    }
    
    for (int i = 0; i < sim->total_apes; i++) {
        if (sim->apes[i]->type == APE_BABY && 
            sim->apes[i]->eaten_bananas >= sim->config.baby_eaten_threshold) {
            printf("Simulation ended: Baby ape %d exceeded eating threshold (%d bananas)\n", 
                   i, sim->apes[i]->eaten_bananas);
            return true;
        }
    }
    
    return false;
}

static void* monitor_thread_func(void* arg) {
    Simulation* sim = (Simulation*)arg;
    int check_count = 0;
    
    while (sim->simulation_running) {
        sleep(1);
        check_count++;
        
        // Log status every 10 seconds
        if (check_count % 10 == 0) {
            printf("\n========== STATUS UPDATE (Time: %d seconds) ==========\n", check_count);
            
            // Count active apes
            int active_females = 0, active_males = 0, active_babies = 0;
            for (int i = 0; i < sim->total_apes; i++) {
                if (sim->apes[i]->active) {
                    if (sim->apes[i]->type == APE_FEMALE) active_females++;
                    else if (sim->apes[i]->type == APE_MALE) active_males++;
                    else if (sim->apes[i]->type == APE_BABY) active_babies++;
                }
            }
            printf("Active apes: %d females, %d males, %d babies\n", 
                   active_females, active_males, active_babies);
            
            printf("\nWithdrawn families: %d / %d\n", 
                   sim->withdrawn_families_count, sim->config.withdrawn_families_threshold);
            
            for (int i = 0; i < sim->config.num_families; i++) {
                pthread_mutex_lock(&sim->baskets[i].lock);
                int basket = sim->baskets[i].bananas;
                bool withdrawn = sim->baskets[i].withdrawn;
                pthread_mutex_unlock(&sim->baskets[i].lock);
                
                // Get male energy
                int male_energy = 0;
                for (int j = 0; j < sim->total_apes; j++) {
                    if (sim->apes[j]->family_id == i && sim->apes[j]->type == APE_MALE) {
                        male_energy = sim->apes[j]->energy;
                        break;
                    }
                }
                
                if (!withdrawn) {
                    printf("Family %d: Basket=%d/%d bananas, Male energy=%d\n", 
                           i, basket, sim->config.family_basket_threshold, male_energy);
                } else {
                    printf("Family %d: WITHDRAWN (basket=%d bananas)\n", i, basket);
                }
            }
            
            printf("\nBaby statistics:\n");
            int max_eaten = 0;
            for (int i = 0; i < sim->total_apes; i++) {
                if (sim->apes[i]->type == APE_BABY && sim->apes[i]->active) {
                    printf("  Family %d Baby %d: eaten=%d bananas\n",
                           sim->apes[i]->family_id, sim->apes[i]->id, 
                           sim->apes[i]->eaten_bananas);
                    if (sim->apes[i]->eaten_bananas > max_eaten) {
                        max_eaten = sim->apes[i]->eaten_bananas;
                    }
                }
            }
            printf("Max baby eaten: %d / %d bananas (threshold)\n", 
                   max_eaten, sim->config.baby_eaten_threshold);
            printf("====================================================\n\n");
        }
        
        if (check_termination_conditions(sim)) {
            pthread_mutex_lock(&sim->sim_lock);
            sim->simulation_running = false;
            pthread_mutex_unlock(&sim->sim_lock);
            
            printf("\n*** TERMINATION CONDITION MET - EXITING PROGRAM ***\n");
            break;
        }
    }
    
    return NULL;
}

static void cleanup_simulation(Simulation* sim) {
    printf("Cleaning up simulation...\n");
    
    // Give threads 2 seconds to finish naturally
    sleep(2);
    
    // Force deactivate all apes
    for (int i = 0; i < sim->total_apes; i++) {
        sim->apes[i]->active = false;
    }
    
    // Try to join threads with short timeout approach
    for (int i = 0; i < sim->total_apes; i++) {
        pthread_detach(sim->apes[i]->thread);
        free(sim->apes[i]);
    }
    free(sim->apes);
    
    for (int i = 0; i < sim->config.num_families; i++) {
        pthread_mutex_destroy(&sim->baskets[i].lock);
        pthread_cond_destroy(&sim->baskets[i].fight_cond);
    }
    free(sim->baskets);
    
    destroy_maze(&sim->maze);
    pthread_mutex_destroy(&sim->sim_lock);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        fprintf(stderr, "Error: Configuration file is required\n");
        return 1;
    }
    
    Simulation sim;
    g_sim = &sim;
    
    init_simulation(&sim, argv[1]);
    
    printf("Starting simulation with %d families, %d apes\n", 
           sim.config.num_families, sim.total_apes);
    
    start_ape_threads(&sim);
    
    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, monitor_thread_func, &sim);
    
    init_graphics(&argc, argv, &sim);
    start_graphics_loop();
    
    pthread_mutex_lock(&sim.sim_lock);
    sim.simulation_running = false;
    pthread_mutex_unlock(&sim.sim_lock);
    
    pthread_join(monitor_thread, NULL);
    
    cleanup_simulation(&sim);
    
    printf("Simulation completed successfully\n");
    
    return 0;
}
