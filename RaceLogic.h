#ifndef RACELOGIC_H
#define RACELOGIC_H

#include <sys/types.h>
#include <cstdlib>
#include <unistd.h> // Include for usleep, often needed in logic files

// --- Configuration Constants (Declarations Only) ---
extern const int RACE_LENGTH;
extern const int NUM_RACERS;

// Enums for Race Status (Stored in Shared Memory)
enum RaceStatus {
    READY = 0,
    RUNNING = 1,
    PAUSED = 2,
    FINISHED = 3,
    EXITING = 4
};

// --- Shared Memory Structure ---
// SHM_SIZE: Defined in RaceLogic.cpp, declared here.
extern const size_t SHM_SIZE;

// --- Shared Memory Index Definitions (Declarations) ---
// These indices must be defined in RaceLogic.cpp, but declared here for use in main.cpp and RaceLogic.cpp
extern const int POS_OFFSET;       // Start of racer positions (0)
extern const int PID_OFFSET;       // Start of racer PIDs (NUM_RACERS)
extern const int STATUS_INDEX;     // Index for RaceStatus enum (NUM_RACERS * 2)
extern const int START_TIME_INDEX; // Index for race start time (STATUS_INDEX + 1)

// --- Function Prototypes ---
void runRacer(int racer_id, int shmid);
void runDisplayParent(int shmid);
void cleanup_shm(int shmid);
void logRaceResult(int winner_id, long duration_ms);
void start_race_processes(int shmid);

#endif // RACELOGIC_H