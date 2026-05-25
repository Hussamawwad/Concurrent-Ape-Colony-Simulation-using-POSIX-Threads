#include "graphics.h"
#include <GL/glut.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static Simulation* sim_ptr = NULL;
static int window_id = 0;

static void draw_cell(int x, int y, float r, float g, float b) {
    int cell_size = sim_ptr->config.cell_size;
    
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x * cell_size, y * cell_size);
    glVertex2f((x + 1) * cell_size, y * cell_size);
    glVertex2f((x + 1) * cell_size, (y + 1) * cell_size);
    glVertex2f(x * cell_size, (y + 1) * cell_size);
    glEnd();
}

static void draw_circle(float cx, float cy, float radius) {
    int segments = 20;
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159f * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        glVertex2f(x, y);
    }
    glEnd();
}

static void display_callback() {
    if (!sim_ptr || !sim_ptr->simulation_running) {
        return;
    }
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    int cell_size = sim_ptr->config.cell_size;
    
    for (int y = 0; y < sim_ptr->maze.height; y++) {
        for (int x = 0; x < sim_ptr->maze.width; x++) {
            if (sim_ptr->maze.cells[y][x].is_obstacle) {
                draw_cell(x, y, 
                    sim_ptr->config.color_obstacle_r,
                    sim_ptr->config.color_obstacle_g,
                    sim_ptr->config.color_obstacle_b);
            } else if (sim_ptr->maze.cells[y][x].bananas > 0) {
                draw_cell(x, y, 
                    sim_ptr->config.color_banana_r,
                    sim_ptr->config.color_banana_g,
                    sim_ptr->config.color_banana_b);
            }
        }
    }
    
    glColor3f(sim_ptr->config.color_grid_r, 
              sim_ptr->config.color_grid_g, 
              sim_ptr->config.color_grid_b);
    glBegin(GL_LINES);
    for (int i = 0; i <= sim_ptr->maze.width; i++) {
        glVertex2f(i * cell_size, 0);
        glVertex2f(i * cell_size, sim_ptr->maze.height * cell_size);
    }
    for (int i = 0; i <= sim_ptr->maze.height; i++) {
        glVertex2f(0, i * cell_size);
        glVertex2f(sim_ptr->maze.width * cell_size, i * cell_size);
    }
    glEnd();
    
    // Draw exit point
    glColor3f(0.0f, 1.0f, 0.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0, 0);
    glVertex2f(cell_size, 0);
    glVertex2f(cell_size, cell_size);
    glVertex2f(0, cell_size);
    glEnd();
    glLineWidth(1.0f);
    
    for (int i = 0; i < sim_ptr->config.num_families; i++) {
        if (!sim_ptr->baskets[i].withdrawn) {
            float cx = (sim_ptr->baskets[i].pos.x + 0.5f) * cell_size;
            float cy = (sim_ptr->baskets[i].pos.y + 0.5f) * cell_size;
            
            glColor3f(sim_ptr->config.color_basket_r,
                     sim_ptr->config.color_basket_g,
                     sim_ptr->config.color_basket_b);
            
            glBegin(GL_QUADS);
            glVertex2f(cx - cell_size * 0.35f, cy - cell_size * 0.35f);
            glVertex2f(cx + cell_size * 0.35f, cy - cell_size * 0.35f);
            glVertex2f(cx + cell_size * 0.35f, cy + cell_size * 0.35f);
            glVertex2f(cx - cell_size * 0.35f, cy + cell_size * 0.35f);
            glEnd();
            
            if (sim_ptr->baskets[i].bananas > 0) {
                glColor3f(1.0f, 1.0f, 0.0f);
                draw_circle(cx, cy, cell_size * 0.2f);
            }
        }
    }
    
    for (int i = 0; i < sim_ptr->total_apes; i++) {
        if (sim_ptr->apes[i]->active) {
            float cx = (sim_ptr->apes[i]->pos.x + 0.5f) * cell_size;
            float cy = (sim_ptr->apes[i]->pos.y + 0.5f) * cell_size;
            
            if (sim_ptr->apes[i]->type == APE_FEMALE) {
                if (sim_ptr->apes[i]->state == STATE_FIGHTING) {
                    glColor3f(1.0f, 0.0f, 0.0f);
                } else {
                    glColor3f(sim_ptr->config.color_female_r,
                             sim_ptr->config.color_female_g,
                             sim_ptr->config.color_female_b);
                }
                draw_circle(cx, cy, cell_size * 0.28f);
                
                if (sim_ptr->apes[i]->collected_bananas > 0) {
                    glColor3f(1.0f, 1.0f, 0.0f);
                    draw_circle(cx, cy, cell_size * 0.12f);
                }
            } else if (sim_ptr->apes[i]->type == APE_MALE) {
                if (sim_ptr->apes[i]->state == STATE_FIGHTING) {
                    glColor3f(1.0f, 0.0f, 0.0f);
                } else {
                    glColor3f(sim_ptr->config.color_male_r,
                             sim_ptr->config.color_male_g,
                             sim_ptr->config.color_male_b);
                }
                draw_circle(cx, cy, cell_size * 0.35f);
                
                glColor3f(1.0f, 1.0f, 1.0f);
                glBegin(GL_QUADS);
                glVertex2f(cx - cell_size * 0.15f, cy - cell_size * 0.05f);
                glVertex2f(cx + cell_size * 0.15f, cy - cell_size * 0.05f);
                glVertex2f(cx + cell_size * 0.15f, cy + cell_size * 0.05f);
                glVertex2f(cx - cell_size * 0.15f, cy + cell_size * 0.05f);
                glEnd();
            } else if (sim_ptr->apes[i]->type == APE_BABY) {
                glColor3f(sim_ptr->config.color_baby_r,
                         sim_ptr->config.color_baby_g,
                         sim_ptr->config.color_baby_b);
                draw_circle(cx, cy, cell_size * 0.18f);
                
                if (sim_ptr->apes[i]->eaten_bananas > 0) {
                    glColor3f(1.0f, 0.5f, 0.0f);
                    draw_circle(cx, cy, cell_size * 0.08f);
                }
            }
        }
    }
    
    glutSwapBuffers();
}

static void timer_callback(int value) {
    if (sim_ptr->simulation_running) {
        glutPostRedisplay();
        glutTimerFunc(sim_ptr->config.update_interval_ms, timer_callback, 0);
    } else {
        // Properly close window and exit GLUT
        if (window_id > 0) {
            glutDestroyWindow(window_id);
        }
        exit(0);
    }
}

static void reshape_callback(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    int maze_pixel_width = sim_ptr->maze.width * sim_ptr->config.cell_size;
    int maze_pixel_height = sim_ptr->maze.height * sim_ptr->config.cell_size;
    
    glOrtho(0, maze_pixel_width, maze_pixel_height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init_graphics(int* argc, char** argv, Simulation* sim) {
    sim_ptr = sim;
    
    glutInit(argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(sim->config.window_width, sim->config.window_height);
    window_id = glutCreateWindow("Apes Collecting Bananas");
    
    glClearColor(sim->config.color_background_r,
                 sim->config.color_background_g,
                 sim->config.color_background_b,
                 1.0f);
    
    glutDisplayFunc(display_callback);
    glutReshapeFunc(reshape_callback);
    glutTimerFunc(sim->config.update_interval_ms, timer_callback, 0);
}

void start_graphics_loop() {
    glutMainLoop();
}
