#include "RaceLogic.h"
#include <iostream>
#include <limits>

using namespace std;

/**
 * @brief Displays the main menu options to the user.
 */
void displayMenu() {
    cout << "-----------------------------------------------" << endl;
    cout << "        OS PROJECT: PROCESS RACE SIMULATOR" << endl;
    cout << "-----------------------------------------------" << endl;
    cout << "1. Start Race Simulation" << endl;
    cout << "2. Exit" << endl;
    cout << "-----------------------------------------------" << endl;
    cout << "Enter choice (1-2): ";
}

/**
 * @brief Main execution loop handling the menu and choices.
 */
void runMenuLoop() {
    int choice;
    bool running = true;

    // Clear the screen initially
    cout << "\033[2J\033[1;1H";

    while (running) {
        displayMenu();
        
        // Input validation loop
        if (!(cin >> choice)) {
            cout << "\n[!] Invalid input. Please enter a number.\n" << endl;
            cin.clear(); // Clear the error flags
            // Ignore the rest of the line buffer to prevent infinite loop
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
            continue;
        }

        switch (choice) {
            case 1:
                // Clear screen before starting the dynamic race
                cout << "\033[2J\033[1;1H"; 
                startRaceSimulation();
                // Pause for a moment after race completion, then clear screen for menu
                cout << "\nPress ENTER to return to the menu...";
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Consume newline from previous input
                cin.get(); 
                cout << "\033[2J\033[1;1H";
                break;
            case 2:
                running = false;
                cout << "\nThank you for using the Process Race Simulator. Goodbye!" << endl;
                break;
            default:
                cout << "\n[!] Invalid choice. Please select 1 or 2.\n" << endl;
                break;
        }
    }
}

int main() {
    runMenuLoop();
    return 0;
}