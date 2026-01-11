#include "RaceLogic.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <string>
#include <signal.h>
#include <ncurses.h>
#include <chrono>

using namespace std;

// --- CONFIGURATION (DEFINITIONS) ---
const int RACE_LENGTH = 100;
const int NUM_RACERS = 4;

// --- Shared Memory Index Definitions (Definitions) ---
const int POS_OFFSET = 0;
const int PID_OFFSET = NUM_RACERS;
const int STATUS_INDEX = NUM_RACERS * 2;
const int START_TIME_INDEX = STATUS_INDEX + 1;

// SHM_SIZE: NUM_RACERS (positions) + NUM_RACERS (PIDs) + 1 (Status) + 1 (Start Time)
const size_t SHM_SIZE = sizeof(int) * (NUM_RACERS * 2) + sizeof(int) + sizeof(long);


// --- EXTERNAL FUNCTION PROTOTYPES (Defined elsewhere) ---
// Defined in NcursesGui.cpp
void initNcurses();
void endNcurses();
void drawRaceTrackGUI(int* shm_ptr, int winner_id, int current_view);
void drawResultsGUI();
// Defined in main.cpp
void start_race_processes(int shmid);

// ----------------------------------------------------------------------
// --- LOGGING ---
// ----------------------------------------------------------------------

/**
 * @brief Logs the race result (winner and duration) to a file.
 */
void logRaceResult(int winner_id, long duration_ms) {
    ofstream outfile("race_results.txt", ios::app);
    if (outfile.is_open()) {
        time_t now = time(nullptr);
        char dt[20];
        strftime(dt, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

        outfile << dt << " | Winner: Racer " << winner_id
                << " | Duration: " << (double)duration_ms / 1000.0 << "s" << endl;
        outfile.close();
    } else {
        cerr << "Error: Could not open race_results.txt for logging." << endl;
    }
}

// ----------------------------------------------------------------------
// --- RACER PROCESS LOGIC (CHILD) ---
// ----------------------------------------------------------------------

void runRacer(int racer_id, int shmid) {
    int* shm_ptr = (int*)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int*)-1) {
        perror("Racer shmat failed");
        exit(EXIT_FAILURE);
    }

    int position_index = POS_OFFSET + racer_id - 1;
    int pid_index = PID_OFFSET + racer_id - 1;

    // Store PID and initialize position
    shm_ptr[pid_index] = getpid();
    shm_ptr[position_index] = 0;

    srand(getpid() * time(NULL));

    // Race Loop: runs until position hits RACE_LENGTH or status is EXITING
    while (shm_ptr[position_index] < RACE_LENGTH && shm_ptr[STATUS_INDEX] != EXITING) {

        // PAUSE/RESUME Logic: loop while status is PAUSED
        while (shm_ptr[STATUS_INDEX] == PAUSED) {
            usleep(100000); // 100ms sleep while paused
        }

        // Only move if RUNNING
        if (shm_ptr[STATUS_INDEX] == RUNNING) {
            int step = (rand() % 4) + 1;
            int current_pos = shm_ptr[position_index];

            // Update position in shared memory
            shm_ptr[position_index] = current_pos + step;

            if (shm_ptr[position_index] >= RACE_LENGTH) {
                shm_ptr[position_index] = RACE_LENGTH;

                // Set race status to FINISHED
                if (shm_ptr[STATUS_INDEX] == RUNNING) {
                    shm_ptr[STATUS_INDEX] = FINISHED;
                }
            }

            // Delay
            usleep(250000 + (rand() % 150000));
        } else if (shm_ptr[STATUS_INDEX] == EXITING) {
            break;
        }
    }

    if (shmdt(shm_ptr) == -1) {
        perror("Racer shmdt failed");
    }
}

// ----------------------------------------------------------------------
// --- MONITOR/PARENT PROCESS LOGIC (GUI) ---
// ----------------------------------------------------------------------

void runDisplayParent(int shmid) {
    int* shm_ptr = (int*)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int*)-1) {
        perror("Monitor shmat failed");
        exit(EXIT_FAILURE);
    }

    // Initialize start time to zero
    // Note: The START_TIME_INDEX is sizeof(int) * 2 + sizeof(long) away from the start
    // We must cast the pointer appropriately to handle the long value correctly
    long* shm_long_ptr = (long*)(shm_ptr + START_TIME_INDEX); 
    *shm_long_ptr = 0;

    initNcurses();

    int current_view = 0; // 0: Race/Control, 1: Results

    // Main GUI Loop
    while (shm_ptr[STATUS_INDEX] != EXITING) {

        int ch = getch(); // Read non-blocking input

        // --- Global Input Handling ('Q' for exit/pause) ---
        if (ch == 'q' || ch == 'Q') {
            if (shm_ptr[STATUS_INDEX] == RUNNING) {
                // If running, 'Q' acts as a Pause/Soft Stop first
                shm_ptr[STATUS_INDEX] = PAUSED;
            } else {
                 // Exit immediately from READY, PAUSED, FINISHED, or HISTORY views
                 shm_ptr[STATUS_INDEX] = EXITING;
            }
        }

        // --- View Specific Input Handling ---

        if (current_view == 0) { // Race/Control View
            if (ch == 's' || ch == 'S') {
                if (shm_ptr[STATUS_INDEX] == READY || shm_ptr[STATUS_INDEX] == FINISHED) {
                    // Start new race: Fork processes
                    start_race_processes(shmid);
                    shm_ptr[STATUS_INDEX] = RUNNING;

                    // Record start time (use the long pointer)
                    auto start_time = chrono::system_clock::now().time_since_epoch().count() / 1000000;
                    *shm_long_ptr = start_time;
                } else if (shm_ptr[STATUS_INDEX] == PAUSED) {
                    // Resume race
                    shm_ptr[STATUS_INDEX] = RUNNING;
                }
            } else if (ch == 'p' || ch == 'P') {
                // Conditional Pause: only works when RUNNING
                if (shm_ptr[STATUS_INDEX] == RUNNING) {
                    shm_ptr[STATUS_INDEX] = PAUSED;
                }
            } else if (ch == 'r' || ch == 'R') {
                 // Switch to Results view
                current_view = 1;
            }
        } else { // Results View (current_view == 1)
            if (ch == 'b' || ch == 'B') {
                // Switch back to Race view
                current_view = 0;
            }
        }

        // --- Drawing Logic and Logging ---

        if (current_view == 0) {
            int winner_id = 0;
            // Get the start time from the long pointer
            long start_time_ms = *shm_long_ptr;

            // Log result only once when race finishes (using start_time_ms != 1 as a log flag)
            if (shm_ptr[STATUS_INDEX] == FINISHED && start_time_ms != 1) {
                for (int i = 0; i < NUM_RACERS; ++i) {
                    if (shm_ptr[POS_OFFSET + i] >= RACE_LENGTH) {
                        winner_id = i + 1;
                        break;
                    }
                }

                // Calculate duration and log
                auto end_time = chrono::system_clock::now().time_since_epoch().count() / 1000000;
                long duration_ms = end_time - start_time_ms;

                logRaceResult(winner_id, duration_ms);
                // Set start time to 1 to indicate 'logged'
                *shm_long_ptr = 1;
            }

            // Find winner ID for display, even if logged
            if (shm_ptr[STATUS_INDEX] == FINISHED) {
                for (int i = 0; i < NUM_RACERS; ++i) {
                    if (shm_ptr[POS_OFFSET + i] >= RACE_LENGTH) {
                        winner_id = i + 1;
                        break;
                    }
                }
            }

            drawRaceTrackGUI(shm_ptr, winner_id, current_view);
        } else {
            drawResultsGUI();
        }

        // Short delay for responsiveness
        usleep(100000);
    }

    endNcurses();

    if (shmdt(shm_ptr) == -1) {
        perror("Monitor shmdt failed");
    }
}

// ----------------------------------------------------------------------
// --- CLEANUP ---
// ----------------------------------------------------------------------

void cleanup_shm(int shmid) {
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        perror("shmctl (IPC_RMID) failed");
    } else {
        cout << "\nShared Memory ID " << shmid << " successfully removed.\n";
    }
}