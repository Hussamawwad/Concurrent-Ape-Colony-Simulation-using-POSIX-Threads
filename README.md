# Concurrent-Ape-Colony-Simulation-using-POSIX-Threads

## Overview
This project is a real-time multi-threaded simulation of ape families collecting bananas inside a maze environment. The simulation models interactions between female, male, and baby apes while demonstrating synchronization, concurrency, resource sharing, and thread coordination using POSIX threads.

The system combines multi-threading with OpenGL-based visualization to simulate dynamic ape behavior in a shared environment.

---

## Features
- Real-time 2D maze simulation
- Randomized banana and obstacle generation
- Multi-threaded ape family behavior:
  - Female apes collect bananas
  - Male apes protect baskets
  - Baby apes steal/eat bananas during conflicts
- Dynamic ape interactions:
  - Basket fights between male apes
  - Female ape encounters and conflicts
  - Energy depletion and resting behavior
- OpenGL visualization for real-time simulation
- Configurable simulation parameters via external file

---

## Tech Stack
- **Language:** C  
- **Platform:** Linux  
- **Libraries:**
  - POSIX Threads (`pthread`)
  - OpenGL / GLUT

### Concepts Used
- Multi-threading
- Mutexes & synchronization
- Shared resources
- Real-time simulation
- Concurrent event handling
- Graphics rendering

---

## System Architecture


Main Simulation Thread
│
├── Female Ape Threads
│ → Maze traversal
│ → Banana collection
│
├── Male Ape Threads
│ → Basket protection
│ → Inter-family fights
│
├── Baby Ape Threads
│ → Stealing/eating bananas
│
└── OpenGL Rendering Loop


---

## Simulation Rules
- Female apes collect bananas and return them to family baskets
- Male apes protect baskets and may fight neighboring males
- Baby apes exploit conflicts to steal bananas
- Apes lose energy during actions and may rest or withdraw
- The simulation ends when configurable thresholds are reached

---

## How to Run

### Build
```bash
make
Run
./ape_simulation config.txt

If no configuration file is provided, default values are used.

Example Output
[Female Ape 2] Collected 5 bananas
[Male Ape 1] Fighting neighboring ape
[Baby Ape 3] Stole 2 bananas
[Family 4] Withdrawn from simulation
Visualization

The simulation uses OpenGL to visualize:

Maze layout
Obstacles
Banana locations
Ape movement
Basket states
Design Highlights
Thread-per-ape simulation model
Shared basket synchronization using mutexes
Event-driven interactions between ape families
Real-time rendering integrated with concurrent logic
Project Structure
src/
├── main.c
├── maze.c
├── ape.c
├── simulation.c
├── graphics.c
├── config.c
Future Improvements
Advanced AI-based ape behavior
Smarter maze pathfinding
Dynamic weather/environment effects
Improved OpenGL animations
Performance optimization with thread pools
