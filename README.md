# 🎲 Multithreaded Ludo Game - OS Project

A complete Ludo board game implementation in C++ that demonstrates operating system concepts including multithreading, synchronization, semaphores, and process management using pthreads.

##  Features
- **Multithreading**: Each player runs in a separate thread
- **Synchronization**: Semaphore-based turn management
- **Concurrency Control**: Master thread for game state management
- **Process Simulation**: Player threads with shared memory access
- **Race Condition Prevention**: Safe board state updates

## Game Rules Implemented
- 2-4 players with 1-4 tokens each
- Dice rolling with special rules (three 6's = skip turn)
- Safe house positions
- Token elimination mechanics
- Block formation rules
- Winning conditions with ranking

## Architecture
Main Process

├── Master Thread (Game State Manager)

├── Player Threads (Concurrent Players)

├── Shared Memory (Board State)

└── Semaphore (Turn Synchronization)


## Technical Details
- **Language**: C++
- **Libraries**: pthread, semaphore.h
- **Concepts**: Thread creation/joining, semaphore operations, critical sections, race condition prevention
- **Platform**: Linux/Ubuntu

## Project Structure
ludo-game-os/
├── main.cpp # Main game implementation

├── Makefile # Compilation instructions

├── README.md # This file

└── run.sh # Execution script


## Compilation & Execution

### Prerequisites
```bash
sudo apt-get update
sudo apt-get install g++ build-essential
Compile and run: g++ -pthread -o ludo_game main.cpp
                 ./ludo_game
