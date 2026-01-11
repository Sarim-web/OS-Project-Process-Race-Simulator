#include "RaceLogic.h"
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <signal.h>

using namespace std;

// Global vector to hold child PIDs
vector<pid_t> children;

/**
 * @brief Kills and cleans up all running racer child processes.
 * @param shmid Shared memory ID.
 */
void cleanup_children(int shmid) {
    if (children.empty()) return;

    cout << "\nTerminating racer processes...\n";

    for (pid_t child_pid : children) {
        int status;
        // Check if the child is still running (waitpid with WNOHANG)
        if (waitpid(child_pid, &status, WNOHANG) == 0) {
            // If running, kill it forcefully
            if (kill(child_pid, SIGKILL) == 0) {
                // Wait for the killed process to be reaped
                waitpid(child_pid, &status, 0);
            } else {
                perror("kill failed");
            }
        }
    }
    children.clear();
}

/**
 * @brief Forks the racer processes. (Called by RaceLogic.cpp when 'S' is pressed)
 * @param shmid Shared memory ID.
 */
void start_race_processes(int shmid) {
    // 1. Cleanup any previous children first (if the user restarted before cleanup)
    cleanup_children(shmid);

    // 2. Re-initialize shared memory positions/PIDs before forking
    int* shm_ptr = (int*)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int*)-1) {
        perror("shmat failed during start_race_processes");
        return;
    }

    // Clear old racer PIDs and positions
    for (int i = 0; i < NUM_RACERS; ++i) {
        shm_ptr[PID_OFFSET + i] = 0;
        shm_ptr[POS_OFFSET + i] = 0;
    }
    shm_ptr[STATUS_INDEX] = READY; // Reset status before fork
    shmdt(shm_ptr);

    // 3. Fork new children
    for (int i = 1; i <= NUM_RACERS; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            perror("fork failed");
            cleanup_shm(shmid);
            exit(1);
        }

        if (pid == 0) {
            // Child Process runs racer logic
            runRacer(i, shmid);
            exit(EXIT_SUCCESS);
        } else {
            // Parent Process stores child PID
            children.push_back(pid);
        }
    }
}

int main() {
    // 1. Create Shared Memory Segment
    // SHM_SIZE is now correctly calculated and defined in RaceLogic.cpp
    int shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        return 1;
    }

    // 2. Attach and Initialize Status to READY
    int* shm_ptr = (int*)shmat(shmid, nullptr, 0);
    if (shm_ptr == (int*)-1) {
        perror("shmat failed during initialization");
        cleanup_shm(shmid);
        return 1;
    }

    shm_ptr[STATUS_INDEX] = READY; // Initial state is READY
    shmdt(shm_ptr);

    cout << "Shared Memory segment created with ID: " << shmid << "\n";
    cout << "Starting TUI (Text User Interface)...\n";

    // 3. Start the GUI loop (Parent process handles GUI, state, and key inputs)
    runDisplayParent(shmid);

    // After GUI exits (STATUS_INDEX == EXITING):

    // 4. Cleanup Child Processes
    cleanup_children(shmid);

    // 5. Cleanup Shared Memory
    cleanup_shm(shmid);

    cout << "\nProgram finished.\n";
    return 0;
}