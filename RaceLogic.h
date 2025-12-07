#ifndef RACELOGIC_H
#define RACELOGIC_H

#include <stddef.h>

// --- Configuration Constants ---
const int NUM_RACERS = 4;           // Number of competing processes
const int RACE_DISTANCE = 100;      // The finish line position
const size_t SHM_SIZE = NUM_RACERS * sizeof(int); // Size of shared memory segment

// --- Function Prototypes ---

/**
 * @brief Initializes shared memory, forks racer processes, runs the display, and cleans up.
 */
void startRaceSimulation();

/**
 * @brief Cleans up and removes the shared memory segment.
 * @param shmid The Shared Memory ID to destroy.
 */
void cleanupShm(int shmid);

#endif // RACELOGIC_H