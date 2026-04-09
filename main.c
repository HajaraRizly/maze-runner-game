#include "type.h"
#include "logic.c"

/*defined players*/
Player PLAYERS[] = {
    { "A", 0, 6,12, 6,12, 5,12, NORTH, false, 0, 100, NORMAL, 0, 0 },
    { "B", 0, 9,8, 9,8, 9,7, WEST,  false, 0, 100, NORMAL, 0, 0 },
    { "C", 0, 9,16, 9,16, 9,17, EAST,  false, 0, 100, NORMAL, 0, 0 }
};

const int N_PLAYERS = sizeof(PLAYERS)/sizeof(PLAYERS[0]);

int main(void) {
    srand((unsigned)time(NULL));

    init_maze_cells();

    if (!load_seed("seed.txt")) {
        printf("Failed to load seed.txt\n");
        return 1;
    }
    if (!load_stairs("stair.txt")) {
        printf("Exiting due to stair load failure.\n");
        return 1;
    }
    if (!load_poles("pole.txt")) {
        printf("No poles loaded!\n");
    }
    if (!load_flag("flag.txt")) {
        printf("Exiting due to flag load failure.\n");
        return 1;
    }
    if (!load_walls("walls.txt")) {
        printf("No walls loaded!\n");
    }

    mark_wall_cells();

    // INVALID_CELLS 
    for (int f = 0; f < F; f++) {
        for (int y = 0; y < L; y++) {
            for (int x = 0; x < W; x++) {
                if (cell_occupied(f, x, y)) {
                    CellCoord *tmp = realloc(INVALID_CELLS, (N_INVALID_CELLS + 1) * sizeof(CellCoord));
                        if (!tmp) {
                            fprintf(stderr, "Memory allocation failed for INVALID_CELLS\n");
                            exit(1);
                        }
                        INVALID_CELLS = tmp;
                        INVALID_CELLS[N_INVALID_CELLS++] = (CellCoord){ f, x, y };

                }
            }
        }
    }

    /*initializing player stuff*/
    count_blocks();

    int round = 1,
    winner = -1;

    /*game loop*/
    while (winner == -1) {
        printf("\n=== Round %d ===\n", round);

        /* Change stair directions  */
        if (round % STAIRS_CHANGE_INTERVAL == 0) {
            printf("\n--- Stair directions change! ---\n");
            for (int i = 0; i < N_STAIRS; i++) {
                STAIRS_DIR[i] = rand() % 2;
                printf("Stair %d is now %s\n", i,
                       (STAIRS_DIR[i] == 0) ? "f1->f2 (down)" : "f2->f1 (up)");
            }
        }
        /*player turns*/
        for (int i = 0; i < N_PLAYERS; ++i) {
            Player *p = &PLAYERS[i];

            int move_roll = roll_movement_dice();
            int dir_roll = 0;
            if (p->in_maze && (p->turn_count % 4 == 0)) {
                dir_roll = roll_direction_dice();
            }

            move_player(p, move_roll, dir_roll);

            if (is_flag_cell(p->f, p->x, p->y)) {
                winner = i;
                break;
            }
        }

        round++;
    }

    printf("\n*** Player %s captured the flag at [%d,%d,%d]! Game over. ***\n",
           PLAYERS[winner].name, FLAG.f, FLAG.x, FLAG.y);

    return 0;
}
