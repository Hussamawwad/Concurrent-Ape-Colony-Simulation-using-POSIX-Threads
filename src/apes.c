#include "apes.h"
#include "maze.h"
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>

extern Simulation* g_sim;

static Position get_random_valid_position(Maze* maze) {
    Position pos;
    do {
        pos.x = rand() % maze->width;
        pos.y = rand() % maze->height;
    } while (is_obstacle(maze, pos));
    return pos;
}

static Position move_randomly(Position current, Maze* maze) {
    Position directions[4] = {
        {current.x + 1, current.y},
        {current.x - 1, current.y},
        {current.x, current.y + 1},
        {current.x, current.y - 1}
    };
    
    int valid_dirs[4];
    int valid_count = 0;
    
    for (int i = 0; i < 4; i++) {
        if (is_valid_position(maze, directions[i]) && !is_obstacle(maze, directions[i])) {
            valid_dirs[valid_count++] = i;
        }
    }
    
    if (valid_count == 0) {
        return current;
    }
    
    int chosen = valid_dirs[rand() % valid_count];
    return directions[chosen];
}

static Position move_towards_exit(Position current, Maze* maze) {
    if (current.x == 0 && current.y == 0) {
        return current;
    }
    
    int current_dist = current.x + current.y;
    Position improving[4];
    int improving_count = 0;
    Position valid[4];
    int valid_count = 0;
    
    Position moves[4] = {
        {current.x - 1, current.y},
        {current.x, current.y - 1},
        {current.x + 1, current.y},
        {current.x, current.y + 1}
    };
    
    for (int i = 0; i < 4; i++) {
        if (is_valid_position(maze, moves[i]) && !is_obstacle(maze, moves[i])) {
            valid[valid_count++] = moves[i];
            int dist = moves[i].x + moves[i].y;
            if (dist < current_dist) {
                improving[improving_count++] = moves[i];
            }
        }
    }
    
    // Prefer moves that reduce distance
    if (improving_count > 0) {
        return improving[rand() % improving_count];
    }
    
    // Otherwise, take any valid move (allows exploration)
    if (valid_count > 0) {
        return valid[rand() % valid_count];
    }
    
    // Completely stuck (shouldn't happen in valid maze)
    return current;
}

static bool check_female_fight(Ape* female, Simulation* sim) {
    // Only fight when exiting AND has bananas
    if (female->state != STATE_EXITING || female->collected_bananas == 0) {
        return false;
    }
    
    for (int i = 0; i < sim->total_apes; i++) {
        Ape* other = sim->apes[i];
        if (other->type == APE_FEMALE && other->active && 
            other->family_id != female->family_id &&
            other->state == STATE_EXITING &&
            other->collected_bananas > 0) {  // Other must also have bananas
            
            // Check if they encounter each other (same cell or adjacent)
            int dx = abs(other->pos.x - female->pos.x);
            int dy = abs(other->pos.y - female->pos.y);
            
            if (dx <= 1 && dy <= 1 && (dx + dy) > 0) {
                // Probabilistic fight
                double rand_val = (double)rand() / RAND_MAX;
                if (rand_val < sim->config.female_exit_fight_probability) {
                    return true;
                }
            }
        }
    }
    return false;
}

static void resolve_female_fight(Ape* female, Simulation* sim) {
    for (int i = 0; i < sim->total_apes; i++) {
        Ape* other = sim->apes[i];
        if (other->type == APE_FEMALE && other->active && 
            other->family_id != female->family_id &&
            other->state == STATE_EXITING) {
            
            // Check if they're nearby
            int dx = abs(other->pos.x - female->pos.x);
            int dy = abs(other->pos.y - female->pos.y);
            
            if (dx <= 1 && dy <= 1 && (dx + dy) > 0) {
                female->state = STATE_FIGHTING;
                other->state = STATE_FIGHTING;
                
                printf("[FIGHT] Female from Family %d vs Family %d at (%d,%d) vs (%d,%d)\n",
                       female->family_id, other->family_id, 
                       female->pos.x, female->pos.y, other->pos.x, other->pos.y);
                
                female->energy -= sim->config.female_energy_per_fight;
                other->energy -= sim->config.female_energy_per_fight;
                
                // Winner takes ALL bananas, loser gets nothing
                if (rand() % 2 == 0) {
                    printf("[FIGHT] Family %d female WON! Stole %d bananas (now has %d), Family %d has %d\n",
                           female->family_id, other->collected_bananas, 
                           female->collected_bananas + other->collected_bananas,
                           other->family_id, 0);
                    female->collected_bananas += other->collected_bananas;
                    other->collected_bananas = 0;
                } else {
                    printf("[FIGHT] Family %d female WON! Stole %d bananas (now has %d), Family %d has %d\n",
                           other->family_id, female->collected_bananas,
                           other->collected_bananas + female->collected_bananas,
                           female->family_id, 0);
                    other->collected_bananas += female->collected_bananas;
                    female->collected_bananas = 0;
                }
                
                // Both continue exiting
                female->state = STATE_EXITING;
                other->state = STATE_EXITING;
                
                return;
            }
        }
    }
}

void* female_ape_thread(void* arg) {
    Ape* ape = (Ape*)arg;
    Simulation* sim = g_sim;
    
    ape->pos = get_random_valid_position(&sim->maze);
    ape->state = STATE_COLLECTING;
    int stuck_counter = 0;
    
    while (sim->simulation_running && ape->active) {
        // Check if family withdrew
        if (sim->baskets[ape->family_id].withdrawn) {
            ape->active = false;
            printf("[Family %d] Female stopped - family withdrawn\n", ape->family_id);
            break;
        }
        
        // Rest if energy low - BUT NOT if close to exit
        int dist_to_exit = (ape->state == STATE_EXITING) 
                          ? (ape->pos.x + ape->pos.y) : 999;
        
        if (ape->energy < sim->config.female_rest_threshold && dist_to_exit > 10) {
            printf("[REST] Family %d Female at (%d,%d) RESTING (energy: %d, distance to exit: %d)\n", 
                   ape->family_id, ape->pos.x, ape->pos.y, ape->energy, dist_to_exit);
            usleep(sim->config.female_rest_duration_ms * 1000);
            ape->energy = sim->config.female_initial_energy;
            printf("[REST] Family %d Female RESUMED (energy restored to: %d)\n", 
                   ape->family_id, ape->energy);
        }
        
        // Phase 1: Collect bananas IN the maze
        if (ape->state == STATE_COLLECTING) {
            // Collect from current cell
            int collected = collect_bananas_from_cell(&sim->maze, ape->pos, 1);
            if (collected > 0) {
                ape->collected_bananas += collected;
            }
            
            // Check if target reached - start EXITING
            if (ape->collected_bananas >= sim->config.female_target_bananas) {
                ape->state = STATE_EXITING;
                printf("[Family %d] Female collected %d bananas at (%d,%d), EXITING maze\n", 
                       ape->family_id, ape->collected_bananas, ape->pos.x, ape->pos.y);
            } else {
                // Move randomly to explore
                ape->pos = move_randomly(ape->pos, &sim->maze);
                ape->energy -= sim->config.female_energy_per_step;
            }
        }
        // Phase 2: EXITING the maze with bananas
        else if (ape->state == STATE_EXITING) {
            // Already at exit - deposit bananas
            if (ape->pos.x == 0 && ape->pos.y == 0) {
                pthread_mutex_lock(&sim->baskets[ape->family_id].lock);
                sim->baskets[ape->family_id].bananas += ape->collected_bananas;
                int basket_total = sim->baskets[ape->family_id].bananas;
                pthread_mutex_unlock(&sim->baskets[ape->family_id].lock);
                
                printf("[DEPOSIT] Family %d Female deposited %d bananas! Basket now: %d/%d\n",
                       ape->family_id, ape->collected_bananas, basket_total, sim->config.family_basket_threshold);
                
                // Enter maze again for new collection cycle
                ape->collected_bananas = 0;
                ape->pos = get_random_valid_position(&sim->maze);
                ape->energy = sim->config.female_initial_energy;
                ape->state = STATE_COLLECTING;
                stuck_counter = 0;
            }
            // Move towards exit - check for fights on the way out
            else {
                Position old_pos = ape->pos;
                Position next;
                
                // Check for encounters with other exiting females
                if (check_female_fight(ape, sim)) {
                    resolve_female_fight(ape, sim);
                }
                
                // If stuck for too long, force random movement
                if (stuck_counter > 2) {
                    next = move_randomly(ape->pos, &sim->maze);
                } else {
                    // Move towards (0,0)
                    next = move_towards_exit(ape->pos, &sim->maze);
                }
                
                ape->pos = next;
                
                // Check if we actually moved
                if (ape->pos.x == old_pos.x && ape->pos.y == old_pos.y) {
                    stuck_counter++;
                } else {
                    stuck_counter = 0;
                    ape->energy -= sim->config.female_energy_per_step;
                }
            }
        }
        
        usleep(sim->config.female_move_delay_ms * 1000);
    }
    
    printf("[Family %d] Female thread ended\n", ape->family_id);
    return NULL;
}

void* male_ape_thread(void* arg) {
    Ape* ape = (Ape*)arg;
    Simulation* sim = g_sim;
    
    ape->pos = sim->baskets[ape->family_id].pos;
    ape->state = STATE_COLLECTING;
    
    printf("[Family %d] Male started guarding basket at (%d,%d) with energy: %d\n",
           ape->family_id, ape->pos.x, ape->pos.y, ape->energy);
    
    while (sim->simulation_running && ape->active) {
        if (sim->baskets[ape->family_id].withdrawn) {
            ape->active = false;
            break;
        }
        
        if (ape->energy < 10) {
            pthread_mutex_lock(&sim->sim_lock);
            sim->baskets[ape->family_id].withdrawn = true;
            sim->withdrawn_families_count++;
            int final_bananas = sim->baskets[ape->family_id].bananas;
            pthread_mutex_unlock(&sim->sim_lock);
            
            printf("[WITHDRAW] Family %d WITHDRAWN! Male energy too low (%d). Final basket: %d bananas\n",
                   ape->family_id, ape->energy, final_bananas);
            
            for (int i = 0; i < sim->total_apes; i++) {
                if (sim->apes[i]->family_id == ape->family_id) {
                    sim->apes[i]->active = false;
                }
            }
            
            ape->active = false;
            break;
        }
        
        ape->energy -= sim->config.male_energy_per_guard_cycle;
        
        pthread_mutex_lock(&sim->baskets[ape->family_id].lock);
        int basket_count = sim->baskets[ape->family_id].bananas;
        pthread_mutex_unlock(&sim->baskets[ape->family_id].lock);
        
        static int guard_count = 0;
        guard_count++;
        if (guard_count % 50 == 0) {
            printf("[GUARD] Family %d Male protecting basket (%d bananas), energy: %d\n",
                   ape->family_id, basket_count, ape->energy);
        }
        
        double fight_prob = sim->config.male_fight_base_probability + 
                           (basket_count * sim->config.male_fight_probability_increment);
        
        double rand_val = (double)rand() / RAND_MAX;
        if (rand_val < fight_prob) {
            pthread_mutex_lock(&ape->fight_lock);
            if (ape->in_fight) {
                pthread_mutex_unlock(&ape->fight_lock);
                usleep(sim->config.male_guard_delay_ms * 1000);
                continue;
            }
            
            for (int i = 0; i < sim->total_apes; i++) {
                Ape* other = sim->apes[i];
                if (other->type != APE_MALE || !other->active || 
                    other->family_id == ape->family_id ||
                    sim->baskets[other->family_id].withdrawn) {
                    continue;
                }
                
                pthread_mutex_lock(&other->fight_lock);
                if (other->in_fight) {
                    pthread_mutex_unlock(&other->fight_lock);
                    continue;
                }
                
                int dx = abs(other->pos.x - ape->pos.x);
                int dy = abs(other->pos.y - ape->pos.y);
                
                if (dx <= 3 && dy <= 3) {
                    ape->in_fight = true;
                    other->in_fight = true;
                    ape->state = STATE_FIGHTING;
                    other->state = STATE_FIGHTING;
                    pthread_mutex_unlock(&other->fight_lock);
                    pthread_mutex_unlock(&ape->fight_lock);
                    
                    FightContext* fight = (FightContext*)malloc(sizeof(FightContext));
                    fight->family_a = ape->family_id;
                    fight->family_b = other->family_id;
                    fight->basket_a = &sim->baskets[ape->family_id];
                    fight->basket_b = &sim->baskets[other->family_id];
                    fight->active = true;
                    fight->babies_participated = 0;
                    fight->max_babies = sim->config.max_babies_per_fight;
                    pthread_mutex_init(&fight->fight_mutex, NULL);
                    pthread_cond_init(&fight->fight_cond, NULL);
                    fight->next = NULL;
                    
                    pthread_mutex_lock(&sim->fights_lock);
                    fight->next = sim->active_fights;
                    sim->active_fights = fight;
                    pthread_mutex_unlock(&sim->fights_lock);
                    
                    pthread_cond_broadcast(&fight->fight_cond);
                    
                    printf("[MALE FIGHT START] Family %d Male (energy:%d, basket:%d) vs Family %d Male (energy:%d, basket:%d) - Distance:(%d,%d)\n",
                           ape->family_id, ape->energy, basket_count, 
                           other->family_id, other->energy, 
                           sim->baskets[other->family_id].bananas, dx, dy);
                    
                    usleep(300000);
                    
                    ape->energy -= sim->config.male_energy_per_fight;
                    other->energy -= sim->config.male_energy_per_fight;
                    
                    pthread_mutex_lock(&fight->basket_a->lock);
                    pthread_mutex_lock(&fight->basket_b->lock);
                    
                    int ape_bananas = fight->basket_a->bananas;
                    int other_bananas = fight->basket_b->bananas;
                    
                    if (rand() % 2 == 0) {
                        fight->basket_a->bananas += other_bananas;
                        fight->basket_b->bananas = 0;
                        printf("[MALE FIGHT END] Family %d male WON! Stole %d bananas, basket now: %d\n",
                               ape->family_id, other_bananas, fight->basket_a->bananas);
                    } else {
                        fight->basket_b->bananas += ape_bananas;
                        fight->basket_a->bananas = 0;
                        printf("[MALE FIGHT END] Family %d male WON! Stole %d bananas, basket now: %d\n",
                               other->family_id, ape_bananas, fight->basket_b->bananas);
                    }
                    
                    pthread_mutex_unlock(&fight->basket_b->lock);
                    pthread_mutex_unlock(&fight->basket_a->lock);
                    
                    pthread_mutex_lock(&fight->fight_mutex);
                    fight->active = false;
                    pthread_mutex_unlock(&fight->fight_mutex);
                    
                    pthread_mutex_lock(&sim->fights_lock);
                    FightContext** ptr = &sim->active_fights;
                    while (*ptr && *ptr != fight) {
                        ptr = &((*ptr)->next);
                    }
                    if (*ptr) {
                        *ptr = fight->next;
                    }
                    pthread_mutex_unlock(&sim->fights_lock);
                    
                    pthread_mutex_destroy(&fight->fight_mutex);
                    pthread_cond_destroy(&fight->fight_cond);
                    free(fight);
                    
                    pthread_mutex_lock(&ape->fight_lock);
                    ape->in_fight = false;
                    ape->state = STATE_COLLECTING;
                    pthread_mutex_unlock(&ape->fight_lock);
                    
                    pthread_mutex_lock(&other->fight_lock);
                    other->in_fight = false;
                    other->state = STATE_COLLECTING;
                    pthread_mutex_unlock(&other->fight_lock);
                    
                    break;
                }
                pthread_mutex_unlock(&other->fight_lock);
            }
            pthread_mutex_unlock(&ape->fight_lock);
        }
        
        usleep(sim->config.male_guard_delay_ms * 1000);
    }
    
    printf("[Family %d] Male thread ended\n", ape->family_id);
    return NULL;
}

void* baby_ape_thread(void* arg) {
    Ape* ape = (Ape*)arg;
    Simulation* sim = g_sim;
    
    ape->pos = sim->baskets[ape->family_id].pos;
    ape->state = STATE_ENTERING;
    
    printf("[Family %d] Baby %d started at basket position (%d,%d)\n",
           ape->family_id, ape->id, ape->pos.x, ape->pos.y);
    
    while (sim->simulation_running && ape->active) {
        if (sim->baskets[ape->family_id].withdrawn) {
            ape->active = false;
            printf("[Family %d] Baby %d stopped - family withdrawn\n", ape->family_id, ape->id);
            break;
        }
        
        pthread_mutex_lock(&sim->fights_lock);
        FightContext* fight = sim->active_fights;
        pthread_mutex_unlock(&sim->fights_lock);
        
        if (fight) {
            pthread_mutex_lock(&fight->fight_mutex);
            if (fight->active && fight->babies_participated < fight->max_babies) {
                int my_basket_x = sim->baskets[ape->family_id].pos.x;
                int my_basket_y = sim->baskets[ape->family_id].pos.y;
                
                int fight_a_x = fight->basket_a->pos.x;
                int fight_a_y = fight->basket_a->pos.y;
                int fight_b_x = fight->basket_b->pos.x;
                int fight_b_y = fight->basket_b->pos.y;
                
                int dist_a = abs(my_basket_x - fight_a_x) + abs(my_basket_y - fight_a_y);
                int dist_b = abs(my_basket_x - fight_b_x) + abs(my_basket_y - fight_b_y);
                int min_dist = (dist_a < dist_b) ? dist_a : dist_b;
                
                if (min_dist <= sim->config.baby_fight_participation_distance) {
                    fight->babies_participated++;
                    pthread_mutex_unlock(&fight->fight_mutex);
                    
                    double steal_rand = (double)rand() / RAND_MAX;
                    if (steal_rand < sim->config.baby_steal_probability) {
                        FamilyBasket* target = NULL;
                        int target_family = -1;
                        
                        if (fight->family_a != ape->family_id) {
                            target = fight->basket_a;
                            target_family = fight->family_a;
                        } else if (fight->family_b != ape->family_id) {
                            target = fight->basket_b;
                            target_family = fight->family_b;
                        }
                        
                        if (target) {
                            pthread_mutex_lock(&target->lock);
                            if (target->bananas > 0) {
                                target->bananas--;
                                int remaining = target->bananas;
                                pthread_mutex_unlock(&target->lock);
                                
                                double eat_rand = (double)rand() / RAND_MAX;
                                if (eat_rand < sim->config.baby_eat_probability) {
                                    ape->eaten_bananas++;
                                    printf("[BABY EAT] Family %d Baby %d ATE banana from Family %d fighting basket (remaining:%d)! Baby eaten total: %d/%d\n",
                                           ape->family_id, ape->id, target_family, remaining,
                                           ape->eaten_bananas, sim->config.baby_eaten_threshold);
                                } else {
                                    pthread_mutex_lock(&sim->baskets[ape->family_id].lock);
                                    sim->baskets[ape->family_id].bananas++;
                                    int own_total = sim->baskets[ape->family_id].bananas;
                                    pthread_mutex_unlock(&sim->baskets[ape->family_id].lock);
                                    printf("[BABY STEAL] Family %d Baby %d stole from Family %d fighting basket (remaining:%d) to own (now:%d)\n",
                                           ape->family_id, ape->id, target_family, remaining, own_total);
                                }
                            } else {
                                pthread_mutex_unlock(&target->lock);
                            }
                        }
                    }
                } else {
                    pthread_mutex_unlock(&fight->fight_mutex);
                }
            } else {
                pthread_mutex_unlock(&fight->fight_mutex);
            }
            usleep(50000);
        } else {
            usleep(100000);
        }
    }
    
    printf("[Family %d] Baby %d thread ended with %d bananas eaten\n", 
           ape->family_id, ape->id, ape->eaten_bananas);
    return NULL;
}
