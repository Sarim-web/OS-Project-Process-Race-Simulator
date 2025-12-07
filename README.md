# 🏎️ OS Project: The Process Race Simulator (C++ & Shared Memory IPC)

## 💡 Project Overview
[cite_start]A dynamic, terminal-based simulation written in C++ that models a race between **multiple independent processes (racers)**[cite: 7]. [cite_start]The simulation relies entirely on **Shared Memory (Inter-Process Communication - IPC)** to update and track each racer's position in real-time[cite: 8].

[cite_start]This project demonstrates mastery of foundational OS concepts, specifically in the areas of process control and synchronization[cite: 10].

## ⚙️ Core Technical Concepts Implemented

| Concept | Demonstration | POSIX System Calls |
| :--- | :--- | :--- |
| **Process Management** | [cite_start]Creating and managing four distinct, autonomous racer processes[cite: 12]. | `fork()` |
| **Inter-Process Communication (IPC)** | [cite_start]Establishing shared memory as a centralized data structure where all processes write their positions[cite: 13, 17]. | `shmget`, `shmat` |
| **Real-Time Simulation** | [cite_start]The Parent process acts as the scheduler/monitor, continuously redrawing the track based on shared memory data[cite: 14, 18]. | (Implicit Concurrency) |
| **Cleanup** | [cite_start]Implementing termination conditions for child processes (finish line) and ensuring the shared memory resource is deallocated[cite: 19, 29]. | `wait`, `shmctl` |

## 📁 Module Structure

[cite_start]The project is divided into three logical modules[cite: 21]:
* [cite_start]**`RaceLogic.h & RaceLogic.cpp`**: Focuses on **POSIX System Calls** (`fork`, `shmget`, `shmat`, `shmctl`, `wait`)[cite: 23]. [cite_start]Handles process creation, child/parent logic, and cleanup[cite: 25, 26, 27, 28, 29].
* [cite_start]**`main.cpp`**: Focuses on **Standard C++ I/O** and program flow[cite: 31]. [cite_start]Provides an interactive menu for starting and exiting the simulation[cite: 33].

## 🛠️ How to Compile and Run (WSL/Linux)
This project requires a Linux environment (like WSL).

1.  **Clone the Repository:**
    `git clone [your-repository-link]`
2.  **Navigate to the Directory:**
    `cd [your-repository-name]`
3.  **Compile the C++ code:** (You may need to include the `-lrt` flag for shared memory depending on the system, but this is the standard compile for your files)
    `g++ main.cpp RaceLogic.cpp -o race_sim`
4.  **Run the Simulation:**
    `./race_sim`

---

5.  Scroll to the bottom of the page and click the green **"Commit changes"** button.

---

## 💼 Part 2: Showcasing on LinkedIn

Now you can officially turn this project into the centerpiece of your professional profile.

### Step 1: Copy the Project URL

Go back to the main page of your repository and **copy the full URL** from your browser's address bar.

### Step 2: Update Your LinkedIn Profile

1.  Log into **LinkedIn**.
2.  Go to your profile page.
3.  Scroll down to the **Projects** section (or click **Add profile section** > **Accomplishments** > **Projects**).
4.  Click the **+** button to add a new project.
    * **Project Name:** `C++ Operating System Project: Inter-Process Communication (IPC)`
    * **Description:** Use a concise version of the project's value:
        > "Semester project demonstrating mastery of Operating System fundamentals. Designed and built a dynamic process race simulator using **C++** that relies entirely on **Shared Memory (IPC)** for real-time process synchronization. Key skills used: `fork()`, `shmget()`, Process Management, and Concurrency."
    * **Project URL:** Paste the GitHub URL you copied in Step 1.
    * **Team Members:** (Optional, but highly recommended) List your group member(s) and their roles. This shows teamwork skill.

### Step 3: Update Your Headline

1.  Edit your profile intro section.
2.  Make sure your Headline (the text under your name) includes these keywords to make you searchable:
    > **Software Engineering Student | Specializing in C++, IPC & Systems Programming | Experienced with Process Management | Seeking 202X Internships**

You now have a complete, professional portfolio piece that is visible to every recruiter who visits your LinkedIn profile. Great work!
