//Assessment 5 
//Helitha Cooray 
//Student No: 24161798

#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>     

//game screen dimensions
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 30

//entity values that are constants
#define NAME_LENGTH 20
#define MAX_HOSTILES 50
#define DELAY 1

//Structure containing the player's information 
typedef struct { 
    char name[NAME_LENGTH];
    int score;
    int lives;
} player;

//structure containing the robot's data
typedef struct {
    int head_x, head_y; //head position
    int body_x, body_y; //body position 
    char head;          
    int dx, dy;         //robot speed
} robot;


//structure for soldier's position 
typedef struct {       
    int x, y;
} soldier;

//structure for hostiles' positions
typedef struct {
    int x, y;
} hostile;


//Game function prototypes 
void draw_title_screen(player *player);
void draw_game_screen(player *player, int level);
void spawn_robot(robot *robot);
void spawn_soldier(soldier *soldier);
void spawn_hostiles(hostile *hostiles, int hostile_count, robot *robot, soldier *soldier);
void move_robot(robot *robot, int vertical_slowdown, int *loop_count);
void reset_game_state(player *player, robot *robot, soldier *soldier, hostile *hostiles, int *hostile_count);
int check_collision(robot *robot, soldier *soldier);
int check_mine_collision(robot *robot, hostile *hostiles, int hostile_count);
void handle_input(robot *robot, int input);
void move_towards_soldier(robot *robot, soldier *soldier); 
int is_position_safe(int x, int y, hostile *hostiles, int hostile_count);
void save_score_to_file(player *player);
void typing_animation(const char *text, int y, int x, int delay_ms);

int main() {
    player player = { "", 0, 3 }; // Initializes the player with an empty name, score 0, and 3 lives
    robot robot = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, '>', 1, 0}; //initializes the robot 
    soldier soldier = {0, 0};
    hostile *hostiles = NULL;
    int hostile_count = 5;
    int level = 1;
    int timeout_ms = 200;
    int input;
    int loop_counter = 0;
    const int vertical_slowdown = 2;

    initscr();

    if (has_colors()) {

        //color sets for different entities in the game
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);   // Red for hostiles
        init_pair(2, COLOR_CYAN, COLOR_BLACK);  // Cyan for robot head
        init_pair(3, COLOR_GREEN, COLOR_BLACK); // Green for robot body
        init_pair(4, COLOR_GREEN, COLOR_BLACK); // Green for soldiers
    }

    noecho();       //ncurses settings 
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    srand(time(NULL));

    hostiles = (hostile *)malloc(MAX_HOSTILES * sizeof(hostile)); // Dynamically allocates memory for hostiles
    if (hostiles == NULL) {
        endwin();
        printf("Could not load hostiles. Please restart the game.\n");
        return 1;
    }

    draw_title_screen(&player); //displays the title screen 

    while (1) {
        reset_game_state(&player, &robot, &soldier, hostiles, &hostile_count);

        while (player.lives > 0) {             //run game as long as the player has lives remaining
            clear();
            draw_game_screen(&player, level);
            spawn_robot(&robot);
            spawn_soldier(&soldier);
            for (int i = 0; i < hostile_count; i++) {
                attron(COLOR_PAIR(1));  
                mvprintw(hostiles[i].y, hostiles[i].x, "X");
                attroff(COLOR_PAIR(1)); 
            }
            refresh();

            timeout(timeout_ms);
            input = getch();

            if (input == 'q') {  //Quits the game
                free(hostiles); // Frees the allocated memory
                endwin();
                return 0;
            }

            // If there's no user input, move the robot towards the soldier (AI) following a simple greedy algorithm 
            if (input == ERR) {
                move_towards_soldier(&robot, &soldier);
            } else {
                handle_input(&robot, input);  // Handles user input to move robot
            }

            move_robot(&robot, vertical_slowdown, &loop_counter);

            if (check_collision(&robot, &soldier)) {
                player.score += 10;
                level = player.score / 50 + 1;
                timeout_ms = (200 - (level * 10)) > 50 ? (200 - (level * 10)) : 50; // Ensure timeout doesn't go below 50 ms

                // Adds two new hostiles each level, up to the maximum permitted
                if (hostile_count < MAX_HOSTILES) {
                    hostile_count += 2;
                    if (hostile_count > MAX_HOSTILES) {
                        hostile_count = MAX_HOSTILES;
                    }
                }
                spawn_hostiles(hostiles, hostile_count, &robot, &soldier);

                soldier.x = rand() % (SCREEN_WIDTH - 3) + 1;
                soldier.y = rand() % (SCREEN_HEIGHT - 3) + 1;
            }

            if (check_mine_collision(&robot, hostiles, hostile_count)) {
                player.lives--;
                break;
            }

            if (robot.head_x < 0 || robot.head_x == SCREEN_WIDTH || robot.head_y < 0 || robot.head_y == SCREEN_HEIGHT) {
                player.lives--;
                reset_game_state(&player, &robot, &soldier, hostiles, &hostile_count); // Reset game state after collision
                break;
            }
        }

        if (player.lives <= 0) {
            clear();
            mvprintw(10, 10, "GAME OVER!");
            mvprintw(12, 10, "Final Score: %d", player.score);
            refresh();
            sleep(5);
            free(hostiles); // Frees the allocated memory
            getch();
            break;



        }
    }
    save_score_to_file(&player);
    endwin();
    return 0;
}


void typing_animation(const char *text, int y, int x, int delay_ms) {
    while (*text) {
        mvprintw(y, x++, "%c", *text++);
        refresh();
        usleep(delay_ms * 1000);   //types text in real time for the story before the game starts 
    }
}


void draw_title_screen(player *player) {
    clear();

    //Game title in ASCII art
    const char *menu_logo[] = {
        " ____          _      _     _____             ",
        "/ ___|__ _  __| | ___| |_  |__  /___ _ __ ___ ",
        "| |   / _` |/ _` |/ _ \\ __|   / // _ \\ '__/ _ \\ ",
        "| |__| (_| | (_| |  __/ |_   / /|  __/ | | (_) |",
        "\\____\\__,_|\\__,_|\\___| \\ __| /____\\___|_| \\___/"
    };

    int start_y = (SCREEN_HEIGHT / 2) - 3;
    int start_x = (SCREEN_WIDTH - strlen(menu_logo[0])) / 2;

    // Print ASCII art
    for (int i = 0; i < 5; i++) {
        mvprintw(start_y + i, start_x, "%s", menu_logo[i]);
    }

    mvprintw(start_y + 10, 10, "Press any key to start the begin...");
    refresh();
    getch();  // Waits for the user to provide an input

    // Clears screen before showing the story 
    clear();
    refresh();
    
    // Story introduction (displaying part by part with delay)
    typing_animation("\"The New Republic's first troop of experimental clone soldiers has been captured.\"", start_y, 10, 50);
    refresh();
    sleep(DELAY);  // Delay for 2 seconds before the next line of text

    typing_animation("\"They're being held hostage by the Separatists.\"", start_y + 2, 10, 50);
    sleep(DELAY);  

    typing_animation("\"We have no available forces to send to recover them.\"", start_y + 4, 10, 50);
    refresh();
    sleep(DELAY);  

    typing_animation("\"Hang on, there's one last option...\"", start_y + 6, 10, 50);
    refresh();
    sleep(DELAY);  

    typing_animation("\"But Colonel, isn't this the very first failed protoype of our troops, Cadet Zero?\"", start_y + 8, 10, 50);
    refresh();
    sleep(DELAY);  

    typing_animation("\"That's our only hope now...\"", start_y + 10, 10, 50);
    refresh();
    sleep(DELAY);  

    typing_animation("Cadet Zero, this is your redemption... we trust you to bring our soldiers back... GOOD LUCK SOLDIER!", start_y + 12, 10, 50);
    refresh();
    sleep(DELAY);

    typing_animation("Press any key to begin...", start_y + 14, 10, 50);
    refresh();
    
    getch();  // Waits for the user to press a key before starting the game

    clear();  // Clear the screen before starting the game

    mvprintw(start_y + 4, 10, "Enter player name: ");

    refresh();

    // Gets player name and validates
    if (getstr(player->name) == ERR) {
        endwin();
        printf("Error getting player name.\n");
        exit(1);
    }

    player->score = 0;   //initializes player data 
    player->lives = 3;
    clear();
}

void draw_game_screen(player *player, int level) {
    for (int i = 0; i < SCREEN_HEIGHT; i++) {
        for (int j = 0; j < SCREEN_WIDTH; j++) {
            if (i == 0 || i == SCREEN_HEIGHT - 1 || j == 0 || j == SCREEN_WIDTH - 1) {
                mvprintw(i, j, "#");
            }
        }
    }
    mvprintw(1, SCREEN_WIDTH + 2, "Player: %s", player->name);
    mvprintw(2, SCREEN_WIDTH + 2, "Score: %d", player->score);
    mvprintw(3, SCREEN_WIDTH + 2, "Lives: %d", player->lives);
    mvprintw(4, SCREEN_WIDTH + 2, "Level: %d", level);
}

void spawn_robot(robot *robot) {
    attron(COLOR_PAIR(2));  
    mvprintw(robot->head_y, robot->head_x, "%c", robot->head);
    attroff(COLOR_PAIR(2));


    attron(COLOR_PAIR(3));  
    mvprintw(robot->body_y, robot->body_x, "O");
    attroff(COLOR_PAIR(3));
}

void spawn_soldier(soldier *soldier) {
    
    attron(COLOR_PAIR(4));  
    mvprintw(soldier->y, soldier->x, "O");
    attroff(COLOR_PAIR(4)); 
}

void spawn_hostiles(hostile *hostiles, int hostile_count, robot *robot, soldier *soldier) {
    for (int i = 0; i < hostile_count; i++) {
        int x, y;
        do {
            x = rand() % (SCREEN_WIDTH - 2) + 1;
            y = rand() % (SCREEN_HEIGHT - 2) + 1;   //spawns hostiles in random locations within the boundaries
        } while ((x == robot->head_x && y == robot->head_y) || (x == soldier->x && y == soldier->y));
        hostiles[i].x = x;
        hostiles[i].y = y;

        attron(COLOR_PAIR(1));  
        mvprintw(hostiles[i].y, hostiles[i].x, "X");
        attroff(COLOR_PAIR(1)); 
    }
}


void move_robot(robot *robot, int vertical_slowdown, int *loop_counter) {
    (*loop_counter)++;
    if (robot->dx != 0 || (*loop_counter) % vertical_slowdown == 0) {
        robot->body_x = robot->head_x;
        robot->body_y = robot->head_y;
        robot->head_x += robot->dx;
        robot->head_y += robot->dy;
    }
}


void handle_input(robot *robot, int input) {
    switch (input) {
        case KEY_UP:
            robot->dx = 0;
            robot->dy = -1;
            robot->head = '^';
            break;
        case KEY_DOWN:
            robot->dx = 0;
            robot->dy = 1;
            robot->head = 'v';
            break;
        case KEY_LEFT:
            robot->dx = -1;
            robot->dy = 0;
            robot->head = '<';
            break;
        case KEY_RIGHT:
            robot->dx = 1;
            robot->dy = 0;
            robot->head = '>';
            break;
    }
}


void reset_game_state(player *player, robot *robot, soldier *soldier, hostile *hostiles, int *hostile_count) {
    robot->head_x = SCREEN_WIDTH / 2;
    robot->head_y = SCREEN_HEIGHT / 2;
    robot->body_x = SCREEN_WIDTH / 2 - 1;
    robot->body_y = SCREEN_HEIGHT / 2;
    robot->dx = 1;
    robot->dy = 0;
    robot->head = '>';

    soldier->x = rand() % (SCREEN_WIDTH - 2) + 1;
    soldier->y = rand() % (SCREEN_HEIGHT - 2) + 1;

    spawn_hostiles(hostiles, *hostile_count, robot, soldier);
}

int check_collision(robot *robot, soldier *soldier) {
    return (robot->head_x == soldier->x && robot->head_y == soldier->y);
}



int check_mine_collision(robot *robot, hostile *hostiles, int hostile_count) {
    for (int i = 0; i < hostile_count; i++) {
        if (robot->head_x == hostiles[i].x && robot->head_y == hostiles[i].y) {
            return 1; // Returns 1 if there will be a collision 
        }
    }
    return 0; // Returns 0 if there is no collision
}


int is_position_safe(int x, int y, hostile *hostiles, int hostile_count) {
    for (int i = 0; i < hostile_count; i++) {
        if (hostiles[i].x == x && hostiles[i].y == y) {
            return 0; // return 0 if the position is occupied by a hostile
        }
    }
    return 1; // Position is safe
}



// New function to move the robot towards the soldier
void move_towards_soldier(robot *robot, soldier *soldier) {
    int dx = 0, dy = 0;

    // Determine horizontal direction
    if (robot->head_x < soldier->x) {
        dx = 1;  // Robot moves right
    } else if (robot->head_x > soldier->x) {
        dx = -1; // Robot moves left
    }

    // Determine vertical direction
    if (robot->head_y < soldier->y) {
        dy = 1;  // Robot moves down
    } else if (robot->head_y > soldier->y) {
        dy = -1; // Robot moves up
    }

    // If vertical and horizontal movement are both possible, one direction can be prioritized  
    if (dx != 0) {
        robot->dx = dx;
        robot->dy = 0;
        robot->head = (dx == 1) ? '>' : '<';  // Changes the head direction based on movement (left/right)
    } else if (dy != 0) {
        robot->dx = 0;
        robot->dy = dy;
        robot->head = (dy == 1) ? 'v' : '^';  // Changes the head direction based on movement (up/down)
    }
}



void save_score_to_file(player *player) {
    // Define a struct to hold the leaderboard entries
    typedef struct {
        char name[NAME_LENGTH];
        int score;
    } leaderboard_entry;

    leaderboard_entry leaderboard[MAX_HOSTILES];  // Array to hold leaderboard entries
    int entry_count = 0;  // To keep track of the number of leaderboard entries

    // Open the file in read mode to load existing leaderboard
    FILE *file = fopen("leaderboard.txt", "r");
    if (file != NULL) {
        // Read existing leaderboard from the file
        while (fscanf(file, "%s %d", leaderboard[entry_count].name, &leaderboard[entry_count].score) == 2) {
            entry_count++;
        }
        fclose(file);
    }

    // Add the current player's score to the leaderboard
    strncpy(leaderboard[entry_count].name, player->name, NAME_LENGTH - 1);
    leaderboard[entry_count].name[NAME_LENGTH - 1] = '\0';  // Ensure null-termination
    leaderboard[entry_count].score = player->score;
    entry_count++;

    // Bubble sort the leaderboard in descending order of scores
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = 0; j < entry_count - i - 1; j++) {
            if (leaderboard[j].score < leaderboard[j + 1].score) {
                // Swap the entries
                leaderboard_entry temp = leaderboard[j];
                leaderboard[j] = leaderboard[j + 1];
                leaderboard[j + 1] = temp;
            }
        }
    }


    // Save the sorted leaderboard back to the file in write mode (scores are sorted in descending order using bubble sort)
    file = fopen("leaderboard.txt", "w");  // Open the file for writing 
    if (file == NULL) {
        endwin();  // Terminate if the program fails to open the file 
        printf("Error opening score file for writing.\n");
        exit(1);
    } else {
        printf("File opened successfully for writing.\n");  
    }

    // Write the sorted leaderboard to the file
    for (int i = 0; i < entry_count; i++) {
        fprintf(file, "%s %d\n", leaderboard[i].name, leaderboard[i].score);
    }

    fflush(file);  // Flush the output to the file to make sure it's written
    fclose(file);  // Close the file

    printw("File written");
}

