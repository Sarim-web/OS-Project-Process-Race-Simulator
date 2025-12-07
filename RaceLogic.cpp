#include "RaceLogic.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <string>

using namespace std;

// --- Internal Function Prototypes ---
void runRacer(int racer_id, int shmid);
void runDisplayParent(int shmid);
void drawRaceTrack(int *positions, int winner_id);

// --- CLEANUP FUNCTION ---
void cleanupShm(int shmid) {
    if (shmctl(shmid, IPC_RMID, nullptr) == -1) {
        perror("shmctl (IPC_RMID) failed");
    }
}

// --- RACER PROCESS LOGIC (CHILD) ---
void runRacer(int racer_id, int shmid) {
    // Attach to the shared memory segment
    int *positions = (int *)shmat(shmid, nullptr, 0);
    if (positions == (int *)-1) {
        perror("shmat failed in racer");
        return;
    }

    // Initialize own position to 0
    positions[racer_id] = 0;

    // Race loop
    while (positions[racer_id] < RACE_DISTANCE) {
        // Generate random speed (1 to 5 units per tick)
        int move_distance = (std::rand() % 5) + 1;
        positions[racer_id] += move_distance;

        // Clamp position to the finish line
        if (positions[racer_id] > RACE_DISTANCE) {
            positions[racer_id] = RACE_DISTANCE;
        }

        // Slow down the simulation tick
        usleep( (std::rand() % 100000) + 50000 ); // 50ms to 150ms delay
    }

    // Detach from the shared memory segment
    if (shmdt(positions) == -1) {
        perror("shmdt failed in racer");
    }
}

// --- DISPLAY LOGIC (PARENT) ---
void runDisplayParent(int shmid) {
    // Attach to the shared memory segment
    int *positions = (int *)shmat(shmid, nullptr, 0);
    if (positions == (int *)-1) {
        perror("shmat failed in display");
        return;
    }

    int winner_id = -1;
    bool race_active = true;

    // Main display loop
    while (race_active) {
        // Clear the screen for a dynamic effect (ANSI escape code)
        // \033[2J clears the screen, \033[1;1H moves cursor to top-left
        cout << "\033[2J\033[1;1H";

        // Check for a winner
        for (int i = 0; i < NUM_RACERS; i++) {
            if (positions[i] >= RACE_DISTANCE) {
                winner_id = i + 1;
                race_active = false; // End the loop
                break;
            }
        }

        drawRaceTrack(positions, winner_id);

        if (!race_active) {
            cout << "\n\n🎉🎉 RACE FINISHED! WINNER IS RACER " << winner_id << "! 🎉🎉" << endl;
            break;
        }

        // Control the display refresh rate
        usleep(50000); // 50 milliseconds
    }

    // Detach from the shared memory segment
    if (shmdt(positions) == -1) {
        perror("shmdt failed in display");
    }
}

// --- VISUALIZATION FUNCTION ---
void drawRaceTrack(int *positions, int winner_id) {
    cout << "==============================================================" << endl;
    cout << "             🏆 PROCESS RACE SIMULATOR 🏆" << endl;
    cout << "        (Processes competing for the finish line)" << endl;
    cout << "==============================================================" << endl;
    cout << "RACE DISTANCE: " << RACE_DISTANCE << " units" << endl << endl;

    // Draw the track for each racer
    for (int i = 0; i < NUM_RACERS; i++) {
        int current_pos = positions[i];
        int track_length = RACE_DISTANCE / 2; // Scale down for terminal display

        // Calculate the scaled position for display (must be >= 0)
        int scaled_pos = (current_pos * track_length) / RACE_DISTANCE;
        if (scaled_pos >= track_length) scaled_pos = track_length;

        // Note: PID is estimated here for simplicity in a multi-fork scenario
        cout << "RACER " << i + 1 << " (PID ~" << getpid() + i + 1 << "): [";

        // Print track progress (dashes before racer symbol)
        for (int j = 0; j < scaled_pos; j++) {
            cout << "-";
        }

        // Print the racer symbol
        if (winner_id == i + 1) {
             cout << "🏁"; // Flag symbol for winner
        } else {
             cout << "🚗"; // Car symbol
        }


        // Print remaining track (spaces after racer symbol)
        for (int j = scaled_pos + 1; j < track_length; j++) {
            cout << " ";
        }

        cout << "] " << current_pos << "/" << RACE_DISTANCE << endl;
    }
    cout << "==============================================================" << endl;
}


// --- MAIN EXECUTION FUNCTION ---
void startRaceSimulation() {
    // 1. Seed the random number generator
    srand(time(NULL) + getpid());

    // 2. Create the Shared Memory segment (IPC Concept)
    int shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return;
    }

    cout << "Shared Memory ID created: " << shmid << ". Starting race..." << endl;

    // 3. Create all racer processes (Process Management Concept)
    for (int i = 0; i < NUM_RACERS; i++) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            cleanupShm(shmid);
            return;
        }

        if (pid == 0) {
            // Child Process (Racer)
            runRacer(i, shmid);
            exit(0);
        }
    }

    // 4. Parent Process (The Display/Monitor)
    runDisplayParent(shmid);

    // 5. Wait for all children to finish (Cleanup)
    int status;
    while (wait(&status) > 0);

    // 6. Clean up the shared memory segment
    cleanupShm(shmid);

    cout << "\nRace simulation finished. Shared memory released." << endl;
}