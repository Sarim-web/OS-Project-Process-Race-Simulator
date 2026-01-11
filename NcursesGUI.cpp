#include "RaceLogic.h"
#include <ncurses.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

// --- CONFIGURATION (LOCAL CONSTANT) ---
const int RACE_LENGTH_DISPLAY = 60;

// --- Ncurses Windows ---
WINDOW *header_win;
WINDOW *race_win;
WINDOW *control_win;

// --- INITIALIZATION AND DRAWING ---

/**
 * @brief Initializes the ncurses environment and defines windows.
 */
void initNcurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE); // Set non-blocking input for getch()

    if (has_colors()) {
        start_color();
        // Define color pairs for Racers and UI Elements
        init_pair(1, COLOR_WHITE, COLOR_RED);     // Racer 1
        init_pair(2, COLOR_WHITE, COLOR_BLUE);    // Racer 2
        init_pair(3, COLOR_WHITE, COLOR_GREEN);   // Racer 3
        init_pair(4, COLOR_BLACK, COLOR_YELLOW);  // Racer 4
        init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Winner Color
        init_pair(6, COLOR_CYAN, COLOR_BLACK);    // Status Text
        init_pair(7, COLOR_WHITE, COLOR_RED);     // Finish Line
        init_pair(8, COLOR_BLACK, COLOR_GREEN);   // Button: Start/Resume
        init_pair(9, COLOR_BLACK, COLOR_YELLOW);  // Button: Pause
        init_pair(10, COLOR_WHITE, COLOR_RED);    // Button: Exit
        init_pair(11, COLOR_BLACK, COLOR_CYAN);   // Button: Results/Back
        init_pair(12, COLOR_WHITE, COLOR_MAGENTA); // Header Background
        init_pair(13, COLOR_WHITE, COLOR_BLACK);  // Default Text (for disabled/track)
    }

    // --- Create and Draw Windows ---
    int max_y, max_x; // Variables for screen dimensions
    getmaxyx(stdscr, max_y, max_x);

    // Header Window
    header_win = newwin(3, max_x, 0, 0);

    // Race Window
    int race_win_height = 8 + NUM_RACERS * 2;
    int race_win_width = RACE_LENGTH_DISPLAY + 45;
    int race_win_y = 3;
    int race_win_x = (max_x - race_win_width) / 2;
    race_win = newwin(race_win_height, race_win_width, race_win_y, race_win_x);

    // Control/Status Window
    control_win = newwin(6, max_x, race_win_y + race_win_height, 0);
}

/**
 * @brief Cleans up the ncurses environment.
 */
void endNcurses() {
    delwin(header_win);
    delwin(race_win);
    delwin(control_win);
    endwin();
}


/**
 * @brief Draws the main race track and control buttons (TUI Menu).
 */
void drawRaceTrackGUI(int* shm_ptr, int winner_id, int current_view) {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    wclear(header_win);
    wclear(race_win);
    wclear(control_win);
    box(race_win, 0, 0); // Draw border

    // --- Header Content ---
    wattron(header_win, A_BOLD | COLOR_PAIR(12));
    mvwprintw(header_win, 1, 2, " C++ Multiprocess Race Simulator (NCURSES TUI) ");
    wattroff(header_win, A_BOLD | COLOR_PAIR(12));
    mvwprintw(header_win, 2, 2, "Race Length: %d | Track Width: %d chars", RACE_LENGTH, RACE_LENGTH_DISPLAY);
    mvwprintw(header_win, 2, max_x - 30, "PID of Monitor: %d", getpid());


    // --- Race Window Content (Track and Racers) ---
    for (int i = 0; i < NUM_RACERS; ++i) {
        int racer_id = i + 1;
        int pos_100 = shm_ptr[POS_OFFSET + i];
        int pid = shm_ptr[PID_OFFSET + i];

        int pos_display = (pos_100 * RACE_LENGTH_DISPLAY) / RACE_LENGTH;
        int y_pos = 2 + i * 2;
        string car_icon = "(O=)";

        // 1. Print Status and PID
        wattron(race_win, COLOR_PAIR(13) | A_BOLD);
        mvwprintw(race_win, y_pos, 2, "Racer %d [PID: %d]:", racer_id, pid);
        wattroff(race_win, COLOR_PAIR(13) | A_BOLD);

        // 2. Print Track Start/End markers
        mvwaddch(race_win, y_pos, 23, '[');

        // Draw the Finish Line
        wattron(race_win, COLOR_PAIR(7) | A_BOLD);
        mvwaddch(race_win, y_pos, 24 + RACE_LENGTH_DISPLAY, ']'); // Using ']' as the finish line end
        wattroff(race_win, COLOR_PAIR(7) | A_BOLD);

        // 3. Draw Progress
        wattron(race_win, COLOR_PAIR(racer_id));
        for (int j = 0; j <= RACE_LENGTH_DISPLAY; ++j) { // Go up to RACE_LENGTH_DISPLAY to handle 100% position
            if (j < pos_display) {
                mvwaddch(race_win, y_pos, 24 + j, ACS_CKBOARD); // Traveled segment
            } else if (j == pos_display && pos_100 < RACE_LENGTH) {
                // Draw the active racer icon (car)
                mvwprintw(race_win, y_pos, 24 + j, "%s", car_icon.c_str());
            } else if (j > pos_display && j < RACE_LENGTH_DISPLAY) {
                wattron(race_win, COLOR_PAIR(13));
                mvwaddch(race_win, y_pos, 24 + j, ACS_HLINE); // Remaining track space
                wattroff(race_win, COLOR_PAIR(13));
            }
        }
        wattroff(race_win, COLOR_PAIR(racer_id));

        // 4. Print Percentage
        mvwprintw(race_win, y_pos, 30 + RACE_LENGTH_DISPLAY, "%3d / %d", pos_100, RACE_LENGTH);
    }

    // --- Control and Status Window ---
    int status = shm_ptr[STATUS_INDEX];
    string status_text;

    switch (status) {
        case READY:
            status_text = "RACE READY. Press 'S' to START the processes.";
            break;
        case RUNNING:
            status_text = "RACE IN PROGRESS. Press 'P' to PAUSE.";
            break;
        case PAUSED:
            status_text = "RACE PAUSED. Press 'S' to RESUME, or 'Q' to EXIT.";
            break;
        case FINISHED:
            status_text = "RACE FINISHED! Winner: Racer " + to_string(winner_id) + ". Press 'S' to reset or 'R' for results.";
            break;
        case EXITING:
            status_text = "EXITING...";
            break;
    }

    // Status Message (Centered and High Visibility)
    wattron(control_win, COLOR_PAIR(6) | A_BOLD);
    mvwprintw(control_win, 1, (max_x - status_text.length()) / 2, "STATUS: %s", status_text.c_str());
    wattroff(control_win, COLOR_PAIR(6) | A_BOLD);

    // --- Draw Buttons (Centered) ---

    // Define Button Texts and Widths
    string btn_s, btn_p, btn_r, btn_q;
    int pair_s, pair_p, pair_r, pair_q;

    // 1. START/RESUME Button (S)
    if (status == READY || status == FINISHED || status == PAUSED) {
        btn_s = (status == PAUSED) ? " S: RESUME " : " S: START ";
        pair_s = COLOR_PAIR(8); // Green (Active)
    } else { // RUNNING (S is inactive)
        btn_s = " S: START ";
        pair_s = COLOR_PAIR(13); // Default (Inactive)
    }

    // 2. PAUSE Button (P)
    if (status == RUNNING) {
        btn_p = " P: PAUSE ";
        pair_p = COLOR_PAIR(9); // Yellow (Active)
    } else {
        btn_p = " P: PAUSE ";
        pair_p = COLOR_PAIR(13); // Default (Inactive)
    }

    // 3. RESULTS Button (R)
    btn_r = " R: RESULTS ";
    pair_r = COLOR_PAIR(11); // Cyan (Active)

    // 4. EXIT Button (Q)
    btn_q = " Q: EXIT ";
    pair_q = COLOR_PAIR(10); // Red (Active)


    // Calculate total button width and starting X position to center
    int total_width = btn_s.length() + btn_p.length() + btn_r.length() + btn_q.length() + (3 * 3); // 3 spaces between 4 buttons
    int start_x = (max_x - total_width) / 2;
    int current_x = start_x;

    // Draw S Button
    wattron(control_win, pair_s | A_BOLD);
    mvwprintw(control_win, 3, current_x, "%s", btn_s.c_str());
    wattroff(control_win, pair_s | A_BOLD);
    current_x += btn_s.length() + 3;

    // Draw P Button
    wattron(control_win, pair_p | A_BOLD);
    mvwprintw(control_win, 3, current_x, "%s", btn_p.c_str());
    wattroff(control_win, pair_p | A_BOLD);
    current_x += btn_p.length() + 3;

    // Draw R Button
    wattron(control_win, pair_r | A_BOLD);
    mvwprintw(control_win, 3, current_x, "%s", btn_r.c_str());
    wattroff(control_win, pair_r | A_BOLD);
    current_x += btn_r.length() + 3;

    // Draw Q Button
    wattron(control_win, pair_q | A_BOLD);
    mvwprintw(control_win, 3, current_x, "%s", btn_q.c_str());
    wattroff(control_win, pair_q | A_BOLD);

    wrefresh(header_win);
    wrefresh(race_win);
    wrefresh(control_win);
}

/**
 * @brief Draws the results history screen.
 */
void drawResultsGUI() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    wclear(header_win);
    wclear(race_win);
    wclear(control_win);
    box(race_win, 0, 0);

    wattron(header_win, A_BOLD | COLOR_PAIR(12));
    mvwprintw(header_win, 1, 2, " RACE HISTORY & RESULTS ");
    wattroff(header_win, A_BOLD | COLOR_PAIR(12));
    mvwprintw(header_win, 2, 2, "File: race_results.txt | Press 'B' to go back.");

    // --- Results Display ---
    wattron(race_win, A_BOLD | COLOR_PAIR(6));
    mvwprintw(race_win, 1, 2, "DATE/TIME                  | WINNER   | DURATION");
    wattroff(race_win, A_BOLD | COLOR_PAIR(6));

    ifstream infile("race_results.txt");
    string line;
    vector<string> results;
    int max_lines = 14;

    // Read all results into a vector
    while (getline(infile, line)) {
        results.push_back(line);
    }

    // Print the last (most recent) results
    int start_index = results.size() > max_lines ? results.size() - max_lines : 0;
    int count = 0;

    for (size_t i = start_index; i < results.size(); ++i) {
        mvwprintw(race_win, 2 + count, 2, "%s", results[i].c_str());
        count++;
    }

    if (results.empty()) {
        mvwprintw(race_win, 2, 2, "No previous race results found. Run a race first!");
    }

    // --- Control Window (Back and Exit Buttons) ---
    string back_text = " B: BACK TO RACE ";
    string exit_text = " Q: EXIT ";

    int total_width = back_text.length() + exit_text.length() + 3;
    int start_x = (max_x - total_width) / 2;

    wattron(control_win, COLOR_PAIR(11) | A_BOLD);
    mvwprintw(control_win, 2, start_x, "%s", back_text.c_str());
    wattroff(control_win, COLOR_PAIR(11) | A_BOLD);

    wattron(control_win, COLOR_PAIR(10) | A_BOLD);
    mvwprintw(control_win, 2, start_x + back_text.length() + 3, "%s", exit_text.c_str());
    wattroff(control_win, COLOR_PAIR(10) | A_BOLD);

    wrefresh(header_win);
    wrefresh(race_win);
    wrefresh(control_win);
}