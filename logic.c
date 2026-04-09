#include "type.h"

/* Globals*/

Cell MAZE[F][L][W];

Stair *STAIRS = NULL;
int N_STAIRS = 0;
int *STAIRS_DIR = NULL;

Pole *POLES = NULL;
int N_POLES = 0;

Wall *WALLS = NULL;
int N_WALLS = 0;

CellCoord *INVALID_CELLS = NULL;
int N_INVALID_CELLS = 0;

Flag FLAG = { -1, -1, -1 };

extern Player PLAYERS[];
extern const int N_PLAYERS;

const char* dir_to_string(Dir d) {
    switch (d) {
        case NORTH: return "NORTH";
        case EAST:  return "EAST";
        case SOUTH: return "SOUTH";
        case WEST:  return "WEST";
        default:    return "?";
    }
}

bool is_bawana_cell(int f, int x, int y) {
    return (f == BAWANA_FLOOR &&
            IN_RANGE(x, BAWANA_X1, BAWANA_X2) &&
            IN_RANGE(y, BAWANA_Y1, BAWANA_Y2));
}

bool is_bawana_entrance(int f, int x, int y) {
    return (f == BAWANA_FLOOR &&
            x == BAWANA_ENTRANCE_X &&
            y == BAWANA_ENTRANCE_Y);
}

// Simple log function
void log_error(const char *fmt, ...) {
    FILE *fp = fopen("log.txt", "a");  // open in append mode
    if (!fp) {
        printf("Could not open log.txt for writing.\n");
        return;
    }

    va_list args;
    va_start(args, fmt);              
    vfprintf(fp, fmt, args);           
    va_end(args);                      

    fclose(fp);
}

void apply_bawana_effect(Player *p) {
    int cell_type = rand() % 12;

    /* place player at the entrance */
    p->f = BAWANA_FLOOR;
    p->x = BAWANA_ENTRANCE_X;
    p->y = BAWANA_ENTRANCE_Y;
    p->dir = NORTH;

    switch (cell_type) {
        case 0: case 1: /* Food Poisoning */
            printf("%s eats from Bawana and have a bad case of food poisoning. Will need three rounds to recover.\n", p->name);
            p->status = POISONED;
            p->skip_turns = 3;
            p->movement_points = 0;
            break;
        case 2: case 3: /* Disoriented */
            printf("%s eats from Bawana and is disoriented and is placed at the entrance of Bawana with 50 movement points.\n", p->name);
            p->status = DISORIENTED;
            p->disoriented_turns = 4;
            p->movement_points = 50;
            break;
        case 4: case 5: /* Triggered */
           printf("%s eats from Bawana and is triggered due to bad quality of food. %s is placed at the entrance of Bawana with 50 movement points.\n", p->name, p->name);
            p->status = TRIGGERED;
            p->movement_points = 50;
            break;
        case 6: case 7: /* Happy */
            printf("%s eats from Bawana and is happy. %s is placed at the entrance of Bawana with 200 movement points.\n", p->name, p->name);
            p->status = HAPPY;
            p->movement_points = 200;
            break;
        default: /* Random bonus */
            p->status = NORMAL;
             {
                int bonus = (rand() % 91) + 10; /* 10–100 */
                p->movement_points += bonus;
                CLAMP_MP(p->movement_points);
                 printf("%s eats from Bawana and earns %d movement points and is placed at the entrance of Bawana [0,9,19].\n",
                   p->name, bonus);
            }
            break;
    }
}

bool is_standing_area(int x, int y) {
    return IN_RANGE(x, 6, 9) && IN_RANGE(y, 8, 16);
}

bool present_floor0(int x, int y) {
    if (!IN_RANGE(x,0,W-1) || !IN_RANGE(y,0,L-1)) return false;
    return !is_standing_area(x,y);
}

bool present_floor1(int x, int y) {
    if (!IN_RANGE(x,0,W-1) || !IN_RANGE(y,0,L-1)) return false;
    if (IN_RANGE(y,0,7)) return true;
    if (IN_RANGE(y,17,24)) return true;
    if (IN_RANGE(y,8,16) && IN_RANGE(x,6,9)) return true; /* bridge */
    return false;
}

bool present_floor2(int x, int y) {
    if (!IN_RANGE(x,0,W-1) || !IN_RANGE(y,0,L-1)) return false;
    return IN_RANGE(y,8,16);
}

bool cell_present(int f, int x, int y) {
    if (!IN_RANGE(f,0,F-1)) return false;
    switch (f) {
        case 0: return present_floor0(x,y);
        case 1: return present_floor1(x,y);
        case 2: return present_floor2(x,y);
        default: return false;
    }
}

void count_blocks(void) {
    int cnt[F] = {0,0,0};
    for (int f=0; f<F; ++f) {
        for (int y=0; y<L; ++y)
            for (int x=0; x<W; ++x)
                if (cell_present(f,x,y)) cnt[f]++;
    }
    printf("Blocks per floor: %d, %d, %d\n", cnt[0], cnt[1], cnt[2]);
    printf("Sq ft per floor: %d, %d, %d\n",
           cnt[0]*4, cnt[1]*4, cnt[2]*4);
}

/* Dice */
int roll_movement_dice(void) { return (rand() % 6) + 1; }
int roll_direction_dice(void) { return (rand() % 6) + 1; }

bool cell_occupied(int f, int x, int y) {
    // Check stairs
    for (int i = 0; i < N_STAIRS; i++) {
        Stair *s = &STAIRS[i];
        if ((s->f1 == f && s->x1 == x && s->y1 == y) ||
            (s->f2 == f && s->x2 == x && s->y2 == y)) {
            return true;
        }
    }

    // Check poles
    for (int i = 0; i < N_POLES; i++) {
        Pole *p = &POLES[i];
        if ((p->f1 == f && p->x == x && p->y == y) ||
            (p->f2 == f && p->x == x && p->y == y)) {
            return true;
        }
    }

    // Check walls
    for (int i = 0; i < N_WALLS; i++) {
        Wall *w = &WALLS[i];
        if (f == w->f &&
            x >= w->x1 && x <= w->x2 &&
            y >= w->y1 && y <= w->y2) {
            return true;
        }
    }

    return false;
}
bool stairs_overlap(Stair *a, Stair *b) {
    return (a->f1 == b->f1 && a->x1 == b->x1 && a->y1 == b->y1) ||
           (a->f2 == b->f2 && a->x2 == b->x2 && a->y2 == b->y2);
}
bool pole_overlap(Pole *p) {
    // Check against stairs
    for (int i = 0; i < N_STAIRS; i++) {
        Stair *s = &STAIRS[i];
        if ((p->f1 == s->f1 && p->x == s->x1 && p->y == s->y1) ||
            (p->f2 == s->f2 && p->x == s->x2 && p->y == s->y2)) {
            return true;
        }
    }

    // Check against other poles
    for (int i = 0; i < N_POLES; i++) {
        Pole *other = &POLES[i];
        if (p != other &&
            ((p->f1 == other->f1 && p->x == other->x && p->y == other->y) ||
             (p->f2 == other->f2 && p->x == other->x && p->y == other->y))) {
            return true;
        }
    }

    return false;
}

bool wall_overlap(Wall *w) {
    for (int y = w->y1; y <= w->y2; y++) {
        for (int x = w->x1; x <= w->x2; x++) {
            if (cell_occupied(w->f, x, y)) {
                return true;
            }
        }
    }
    return false;
}
bool wall_valid(Wall *w) {
    int x, y;
    FILE *log = fopen("log.txt", "a");  // open log file in append mode
    if (!log) {
        perror("Failed to open log.txt");
        return false;
    }

    // 1. Must be vertical or horizontal
    if (w->x1 != w->x2 && w->y1 != w->y2) {
        fprintf(log, "Invalid wall [%d,%d,%d] -> [%d,%d,%d]: diagonal not allowed.\n",
                w->f, w->x1, w->y1, w->f, w->x2, w->y2);
        fclose(log);
        return false;
    }

    // 2. Cannot be in Bawana
    for (y = w->y1; y <= w->y2; y++) {
        for (x = w->x1; x <= w->x2; x++) {
            if (is_bawana_cell(w->f, x, y)) {
                fprintf(log, "Invalid wall [%d,%d,%d] -> [%d,%d,%d]: overlaps Bawana.\n",
                        w->f, w->x1, w->y1, w->f, w->x2, w->y2);
                fclose(log);
                return false;
            }
        }
    }

    // 3. Cannot be on invalid cells
    for (y = w->y1; y <= w->y2; y++) {
        for (x = w->x1; x <= w->x2; x++) {
            if (!cell_present(w->f, x, y)) {
                fprintf(log, "Invalid wall [%d,%d,%d] -> [%d,%d,%d]: cell not present.\n",
                        w->f, w->x1, w->y1, w->f, w->x2, w->y2);
                fclose(log);
                return false;
            }

            // 4. Check overlap with stairs
            for (int i = 0; i < N_STAIRS; i++) {
                Stair *s = &STAIRS[i];
                if ((w->f == s->f1 && x == s->x1 && y == s->y1) ||
                    (w->f == s->f2 && x == s->x2 && y == s->y2)) {
                    fprintf(log, "Invalid wall [%d,%d,%d] -> [%d,%d,%d]: overlaps stair.\n",
                            w->f, w->x1, w->y1, w->f, w->x2, w->y2);
                    fclose(log);
                    return false;
                }
            }

            // 5. Check overlap with poles
            for (int i = 0; i < N_POLES; i++) {
                Pole *p = &POLES[i];
                if ((w->f == p->f1 && x == p->x && y == p->y) ||
                    (w->f == p->f2 && x == p->x && y == p->y)) {
                    fprintf(log, "Invalid wall [%d,%d,%d] -> [%d,%d,%d]: overlaps pole.\n",
                            w->f, w->x1, w->y1, w->f, w->x2, w->y2);
                    fclose(log);
                    return false;
                }
            }
        }
    }

    fclose(log);
    return true;  // wall is valid
}

/* seed loader */
bool load_seed(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return false;
    unsigned int seed;
    if (fscanf(fp, "%u", &seed) != 1) {
        fclose(fp);
        return false;
    }
    srand(seed);
    fclose(fp);
    return true;
}

/* stairs loader */
bool load_stairs(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to read stair file");
        return false;
    }

    FILE *log = fopen("log.txt", "w");  // open log file in append mode
    if (!log) {
        perror("Failed to open log.txt");
        fclose(fp);
        return false;
    }

    int count = 0;
    char line[128];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '\n' && line[0] != '#') count++;
    }
    rewind(fp);

    STAIRS = malloc(count * sizeof(Stair));
    if (!STAIRS) {
        fprintf(log, "Memory allocation failed for STAIRS\n");
        fclose(fp);
        fclose(log);
        return false;
    }

    N_STAIRS = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#') continue;

        Stair s;
        if (sscanf(line, "[%d,%d,%d,%d,%d,%d]",
                   &s.f1, &s.x1, &s.y1, &s.f2, &s.x2, &s.y2) != 6) {
            fprintf(log, "Invalid format in line: %s\n", line);
            continue;
        }
        //validation 0: validating the cell
        if (!cell_present(s.f1,s.x1,s.y1) || !cell_present(s.f2,s.x2,s.y2)) {
             fprintf(log,"Invalid stair [%d,%d,%d]->[%d,%d,%d]: cell not present\n",
            s.f1,s.x1,s.y1,s.f2,s.x2,s.y2);
            continue;
        }
        // Validation 1: cannot go from higher floor to lower floor
        if (s.f1 >= s.f2) {
            fprintf(log,
                "Invalid stair: [%d,%d,%d] -> [%d,%d,%d]. "
                "Stairs must go from a lower floor to a higher floor.\n",
                s.f1, s.x1, s.y1, s.f2, s.x2, s.y2);
            continue;
        }

        // Validation 2: check overlap with existing stairs
        bool overlap = false;
        for (int i = 0; i < N_STAIRS; i++) {
            if (stairs_overlap(&s, &STAIRS[i])) {
                overlap = true;
                fprintf(log, "Stair [%d,%d,%d]->[%d,%d,%d] overlaps with existing stair [%d,%d,%d]->[%d,%d,%d]\n",
                        s.f1, s.x1, s.y1, s.f2, s.x2, s.y2,
                        STAIRS[i].f1, STAIRS[i].x1, STAIRS[i].y1,
                        STAIRS[i].f2, STAIRS[i].x2, STAIRS[i].y2);
                break;
            }
        }

        // Validation 3: check overlap with existing poles
        if (!overlap) {
            for (int i = 0; i < N_POLES; i++) {
                if (cell_occupied(s.f1, s.x1, s.y1) || cell_occupied(s.f2, s.x2, s.y2)) {
                    overlap = true;
                    fprintf(log, "Stair [%d,%d,%d]->[%d,%d,%d] overlaps with existing pole.\n",
                            s.f1, s.x1, s.y1, s.f2, s.x2, s.y2);
                    break;
                }
            }
        }

        if (overlap) continue;  // skip invalid stair

        // Add the valid stair
        STAIRS[N_STAIRS++] = s;
    }

    fclose(fp);
    fclose(log);  // close log file

    printf("Loaded %d valid stairs:\n", N_STAIRS);
    for (int i = 0; i < N_STAIRS; i++) {
        printf("  Stair %d: [%d,%d,%d] -> [%d,%d,%d]\n", i,
               STAIRS[i].f1, STAIRS[i].x1, STAIRS[i].y1,
               STAIRS[i].f2, STAIRS[i].x2, STAIRS[i].y2);
    }

    // Allocate STAIRS_DIR
    STAIRS_DIR = malloc(N_STAIRS * sizeof(int));
    if (!STAIRS_DIR) {
        log = fopen("log.txt", "a");
        if (log) {
            fprintf(log, "Memory allocation failed for STAIRS_DIR\n");
            fclose(log);
        }
        return false;
    }
    for (int i = 0; i < N_STAIRS; i++) {
        STAIRS_DIR[i] = rand() % 2;
    }

    return true;
}



bool load_poles(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to read pole file");
        return false;
    }

    FILE *log = fopen("log.txt", "a");  // open log file for appending
    if (!log) {
        perror("Failed to open log.txt");
        fclose(fp);
        return false;
    }

    char line[128];
    int count = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] != '\n' && line[0] != '#') count++;
    }
    rewind(fp);

    POLES = malloc(count * sizeof(Pole));
    if (!POLES) {
        fprintf(log, "Memory allocation failed for POLES\n");
        fclose(fp);
        fclose(log);
        return false;
    }

    N_POLES = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#') continue;

        Pole p;
        if (sscanf(line, "[%d,%d,%d,%d]", &p.f1, &p.f2, &p.x, &p.y) == 4) {

            // Validation: poles cannot overlap with stairs, walls, or other poles
            if (cell_occupied(p.f1, p.x, p.y) || cell_occupied(p.f2, p.x, p.y)) {
                fprintf(log,
                    "Invalid pole: [%d,%d,%d] -> [%d,%d,%d] overlaps with existing object.\n",
                    p.f1, p.x, p.y, p.f2, p.x, p.y);
                continue; // skip invalid pole
            }

            POLES[N_POLES++] = p; // only store if valid
        } else {
            fprintf(log, "Invalid format in line: %s\n", line);
        }
    }

    fclose(fp);
    fclose(log);

    printf("Loaded %d poles:\n", N_POLES);
    for (int i = 0; i < N_POLES; i++) {
        printf("  Pole %d: floor %d -> floor %d at [%d,%d]\n",
               i, POLES[i].f1, POLES[i].f2, POLES[i].x, POLES[i].y);
    }

    return true;
}


/* walls loader */
bool load_walls(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open wall file");
        return false;
    }

    FILE *log = fopen("log.txt", "a"); // open log file for appending
    if (!log) {
        perror("Failed to open log.txt");
        fclose(fp);
        return false;
    }

    char line[128];
    N_WALLS = 0; // reset wall count
    WALLS = NULL; // start with empty array

    while (fgets(line, sizeof(line), fp)) {
        // Skip empty lines and comments
        if (line[0] == '\n' || line[0] == '#') continue;

        int f, x1, y1, x2, y2;
        if (sscanf(line, "[%d,%d,%d,%d,%d]", &f, &x1, &y1, &x2, &y2) != 5) {
            fprintf(log, "Invalid wall line format: %s", line);
            continue;
        }

        Wall w = { f, x1, y1, x2, y2 };

        // Validate wall
        if (!wall_valid(&w)) continue;

        // expand WALLS array
        Wall *tmp = realloc(WALLS, (N_WALLS + 1) * sizeof(Wall));
        if (!tmp) {
            fprintf(log, "Memory allocation failed for WALLS\n");
            free(WALLS);
            fclose(fp);
            fclose(log);
            return false;
        }
        WALLS = tmp;
        WALLS[N_WALLS++] = w;
    }

    fclose(fp);
    fclose(log);

    printf("Loaded %d walls\n", N_WALLS);
    return true;
}

/* mark wall cells in MAZE */
void mark_wall_cells() {
    for (int i = 0; i < N_WALLS; i++) {
        Wall *w = &WALLS[i];
        for (int y = w->y1; y <= w->y2; y++) {
            for (int x = w->x1; x <= w->x2; x++) {
                if (cell_present(w->f, x, y)) {
                    MAZE[w->f][y][x].is_wall = true;
                }
            }
        }
    }

    /* Bawana wall strips  */
    for (int x = 6; x <= 9; x++) {
        MAZE[0][20][x].is_wall = true;
    }
    for (int y = 20; y <= 24; y++) {
        MAZE[0][y][6].is_wall = true;
    }

}

/* load flag */
bool load_flag(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to read flag file");
        return false;
    }

    FILE *log = fopen("log.txt", "a");  // open log file for appending
    if (!log) {
        perror("Failed to open log.txt");
        fclose(fp);
        return false;
    }

    int f, x, y;
    if (fscanf(fp, "[%d,%d,%d]", &f, &x, &y) != 3) {
        fprintf(log, "Invalid flag.txt format. Expected: [f,x,y]\n");
        fclose(fp);
        fclose(log);
        return false;
    }
    fclose(fp);

    if (!cell_present(f, x, y)) {
        fprintf(log, "Invalid flag position [%d,%d,%d]: cell not present\n", f, x, y);
        fclose(log);
        return false;
    }

    if (cell_occupied(f, x, y)) {
        fprintf(log, "Invalid flag position [%d,%d,%d]: overlaps with stairs/poles/walls\n", f, x, y);
        fclose(log);
        return false;
    }

    FLAG.f = f; 
    FLAG.x = x; 
    FLAG.y = y;

    printf("Flag loaded at [%d,%d,%d]\n", f, x, y);
    fclose(log);
    return true;
}

bool is_flag_cell(int f, int x, int y) {
    return (FLAG.f == f && FLAG.x == x && FLAG.y == y);
}


/* movement helpers */
bool apply_stairs(Player *p) {
    for (int i = 0; i < N_STAIRS; i++) {
        if (STAIRS_DIR[i] == 0 &&
            p->f == STAIRS[i].f1 && p->x == STAIRS[i].x1 && p->y == STAIRS[i].y1) {
            printf("Player %s lands on [%d,%d,%d] which is a stair cell.\n",
                   p->name, p->f, p->x, p->y);
            p->f = STAIRS[i].f2;
            p->x = STAIRS[i].x2;
            p->y = STAIRS[i].y2;
            printf("Player %s takes the stairs and now placed at [%d,%d,%d] in floor %d.\n",
                   p->name, p->f, p->x, p->y, p->f);
            return true;
        }
        if (STAIRS_DIR[i] == 1 &&
            p->f == STAIRS[i].f2 && p->x == STAIRS[i].x2 && p->y == STAIRS[i].y2) {
            printf("Player %s lands on [%d,%d,%d] which is a stair cell.\n",
                   p->name, p->f, p->x, p->y);
            p->f = STAIRS[i].f1;
            p->x = STAIRS[i].x1;
            p->y = STAIRS[i].y1;
            printf("Player %s takes the stairs and now placed at [%d,%d,%d] in floor %d.\n",
                   p->name, p->f, p->x, p->y, p->f);
            return true;
        }
    }
    return false;
}

bool apply_poles(Player *p) {
    for (int i = 0; i < N_POLES; i++) {
        Pole *pole = &POLES[i];
        if (p->x == pole->x && p->y == pole->y && p->f == pole->f1) {
            printf("Player %s lands on [%d,%d,%d] which is a pole cell.\n",
                   p->name, p->f, p->x, p->y);

            p->f = pole->f2;  /* slide down */
            printf("Player %s slides down and now placed at [%d,%d,%d] in floor %d.\n",
                   p->name, p->f, p->x, p->y, p->f);
            return true;
        }
    }
    return false;
}

inline bool apply_transport_if_present(Player *p) {
    if (apply_stairs(p)) return true;
    if (apply_poles(p))  return true;
    return false;
}

/*To capture players*/
void check_capture(Player *p) {
    for (int i = 0; i < N_PLAYERS; i++) {
        Player *other = &PLAYERS[i];
        if (other == p) continue;
        if (other->in_maze && other->f == p->f && other->x == p->x && other->y == p->y) {
            printf("Player %s lands on [%d,%d,%d] occupied by Player %s.\n",
                   p->name, p->f, p->x, p->y, other->name);
            printf("Player %s is captured and moved back to the starting area.\n", other->name);
            other->in_maze = false;
            other->x = other->spawn_x;
            other->y = other->spawn_y;
            other->f = 0;
            other->turn_count = 0;
        }
    }
}

/*Apply directions */
void apply_dir_roll(Player *p, int dir_roll) {
    switch (dir_roll) {
        case 2: p->dir = NORTH; break;
        case 3: p->dir = EAST;  break;
        case 4: p->dir = SOUTH; break;
        case 5: p->dir = WEST;  break;
        default: break;
    }
}

/* try enter maze (starting area) */
void try_enter_maze(Player *p, int move_roll) {
    if (p->in_maze) return;
    if (move_roll == 6) {
        p->x = p->first_x;
        p->y = p->first_y;
        p->in_maze = true;
        p->turn_count = 1;
        printf("Player %s is at the starting area and rolls 6 on the movement dice and is placed on [%d,%d] of the maze.\n",
               p->name, p->x, p->y);
    } else {
        printf("Player %s is at the starting area and rolls %d on the movement dice cannot enter the maze.\n",
               p->name, move_roll);
    }
}

/* move_player */
void move_player(Player *p, int move_roll, int dir_roll) {
    if (!p->in_maze) {
        try_enter_maze(p, move_roll);
        return;
    }

    Dir old_dir = p->dir;

    /* Food poisoning */
    if (p->status == POISONED) {
        if (p->skip_turns > 0) {
            printf("Player %s is still food poisoned and misses the turn. (%d turns left)\n",
                   p->name, p->skip_turns);
            p->skip_turns--;
            p->turn_count++;
            return;
        }
        printf("%s is now fit to proceed from the food poisoning episode and now placed on a Bawana cell and the effects take place.\n",
               p->name);
        apply_bawana_effect(p);
        return;
    }

    /* Disoriented*/
    if (p->status == DISORIENTED && p->disoriented_turns > 0) {
        p->dir = (Dir)(1 + rand() % 4);
        p->disoriented_turns--;
        printf("Player %s rolls and %d on the movement dice and is disoriented and move in the %s and moves %d cells and is placed at [%d,%d,%d].\n",
               p->name, move_roll, dir_to_string(p->dir), move_roll, p->f, p->x, p->y);
        
               p->disoriented_turns--;
        if (p->disoriented_turns == 0) {
            p->status = NORMAL;
            printf("%s has recovered from disorientation.\n", p->name);
        }
    } else {
        if (p->turn_count % 4 == 0) {
            apply_dir_roll(p, dir_roll);
        }
    }

    int effective_roll = move_roll;
    if (p->status == TRIGGERED) {
        effective_roll = move_roll * 2;
        printf("%s is triggered and rolls and %d on the movement dice and move in the %s direction and moves %d cells and is placed at [%d,%d,%d].\n",
               p->name, move_roll, dir_to_string(p->dir), effective_roll, p->f, p->x, p->y);
    } else if (p->turn_count % 4 == 0 && (dir_roll >= 2 && dir_roll <= 5)) {
        Dir old_dir = p->dir;
        apply_dir_roll(p, dir_roll);
        printf("%s rolls and %d on the movement dice and %s on the direction dice, changes direction to %s and moves %d cells and is now at [%d,%d,%d].\n",
               p->name, move_roll, dir_to_string(old_dir), dir_to_string(p->dir),
               effective_roll, p->f, p->x, p->y);
    } else {
        printf("%s rolls and %d on the movement dice and moves %s by %d cells and is now at [%d,%d,%d].\n",
               p->name, move_roll, dir_to_string(p->dir), effective_roll, p->f, p->x, p->y);
    }

    int dx = 0, dy = 0;
    switch (p->dir) {
        case NORTH: dx = -1; break;
        case EAST:  dy = +1; break;
        case SOUTH: dx = +1; break;
        case WEST:  dy = -1; break;
    }

    bool blocked = false;
    int steps_left = effective_roll;
    int steps_taken = 0;
    bool dir_changed_this_turn = (p->turn_count % 4 == 0) && (dir_roll >= 2 && dir_roll <= 5) && (p->status != DISORIENTED);

    while (steps_left > 0) {
        int from_x = p->x;
        int from_y = p->y;
        int from_f = p->f;

        int to_x = from_x + dx;
        int to_y = from_y + dy;

        /* Prevent walking directly into Bawana from the entrance */
        if (from_f == BAWANA_FLOOR &&
            from_x == BAWANA_ENTRANCE_X && from_y == BAWANA_ENTRANCE_Y &&
            to_x == BAWANA_ENTRANCE_X && to_y == BAWANA_Y1) {
            blocked = true;
            printf("Player %s cannot walk directly into Bawana from the entrance.\n", p->name);
            break;
        }

        /* Check boundaries and walls for this step */
        if (!cell_present(from_f, to_x, to_y) ||MAZE[from_f][to_y][to_x].is_wall) {
            blocked = true;
            break;
        }

        /* take the step */
        p->x = to_x;
        p->y = to_y;
        steps_left--;
        steps_taken++;

        Cell *c = &MAZE[p->f][p->y][p->x];

        if (c->consumable > 0) {
            p->movement_points -= c->consumable;
            printf("Cell [%d,%d,%d] consumes %d MP. Remaining MP = %d\n",
                   p->f, p->x, p->y, c->consumable, p->movement_points);
        }

        if (c->bonus > 0) {
            p->movement_points += c->bonus;
            CLAMP_MP(p->movement_points);
            printf("Cell [%d,%d,%d] grants +%d MP. Remaining MP = %d\n",
                p->f, p->x, p->y, c->bonus, p->movement_points);
        }


        if (c->multiplier > 1) {
            p->movement_points *= c->multiplier;
            CLAMP_MP(p->movement_points);
            printf("Cell [%d,%d,%d] multiplies MP by %d. Remaining MP = %d\n",
                p->f, p->x, p->y, c->multiplier, p->movement_points);
        }


        if (p->movement_points <= 0) {
            printf("%s movement points are depleted and requires replenishment. Transporting to Bawana.\n", p->name);
            printf("%s is place on a Bawana cell and effects take place.\n", p->name);
            apply_bawana_effect(p);
            return;
        }

        if (apply_stairs(p) || apply_poles(p)) {
            /*have changed position/floor*/
        }

        if (is_bawana_entrance(p->f, p->x, p->y)) {
            p->dir = NORTH;
            printf("Player %s stands at Bawana entrance [0,%d,%d], facing North.\n",
                   p->name, BAWANA_ENTRANCE_X, BAWANA_ENTRANCE_Y);
        }

        if (is_bawana_cell(p->f, p->x, p->y)) {
            printf("Player %s has entered Bawana area at [%d,%d,%d]!\n",
                   p->name, p->f, p->x, p->y);
            apply_bawana_effect(p);
            return;
        }
    } 

    if (!blocked) {
        if (dir_changed_this_turn) {
            printf("%s rolls %d on the movement dice and cannot move in the %s. Player remains at [%d,%d,%d].\n",
               p->name, move_roll, dir_to_string(p->dir), p->f, p->x, p->y);
        } else {
            printf("Player %s moves %s by %d cells and is now at [%d,%d,%d].\n",
                   p->name, dir_to_string(p->dir), steps_taken, p->f, p->x, p->y);
        }
        
    } else {
        printf("Player %s cannot move in %s, remains at [%d,%d,%d].\n",
               p->name, dir_to_string(p->dir), p->f, p->x, p->y);
        p->movement_points -= 2; /* wall penalty */
        printf("Player %s loses 2 MP due to wall block. Remaining MP = %d\n",
               p->name, p->movement_points);
    }

    if (p->movement_points <= 0) {
        printf("%s movement points are depleted and requires replenishment. Transporting to Bawana.\n", p->name);
        printf("%s is place on a Bawana cell and effects take place.\n", p->name);
        apply_bawana_effect(p);
        return;
    }
    printf("%s moved %d that cost %d MP and is left with %d and is moving in the %s.\n",
           p->name, effective_roll, effective_roll, p->movement_points, dir_to_string(p->dir));

    check_capture(p);
    p->turn_count++;
}

// Check if a cell is part of the starting area
bool is_starting_area(int f, int y, int x) {
    // already have a "standing area" function
    return (f == 0 && is_standing_area(x, y));
}

// Check if a cell is marked as invalid
bool is_invalid_cell(int f, int y, int x) {
    for (int i = 0; i < N_INVALID_CELLS; i++) {
        if (INVALID_CELLS[i].f == f &&
            INVALID_CELLS[i].x == x &&
            INVALID_CELLS[i].y == y) {
            return true;
        }
    }
    return false;
}


void init_maze_cells() {
    srand((unsigned)time(NULL));

    for (int f = 0; f < F; f++) {
        for (int y = 0; y < L; y++) {
            for (int x = 0; x < W; x++) {
                // Clear defaults
                MAZE[f][y][x].consumable = 0;
                MAZE[f][y][x].bonus = 0;
                MAZE[f][y][x].multiplier = 1;
                MAZE[f][y][x].is_wall = false;

                // Skip Bawana floor 
                if (f == BAWANA_FLOOR) continue;

                // Skip starting area or invalid cells
                if (is_starting_area(f, y, x) || is_invalid_cell(f, y, x)) continue;

                // Randomly assign cell consumable/bonus/multiplier
                int roll = rand() % 100;
                if (roll < 25) {
                    MAZE[f][y][x].consumable = 0;
                } else if (roll < 60) {
                    MAZE[f][y][x].consumable = 1 + rand() % 4;
                } else if (roll < 85) {
                    MAZE[f][y][x].bonus = 1 + rand() % 2;
                } else if (roll < 95) {
                    MAZE[f][y][x].bonus = 3 + rand() % 3;
                } else {
                    MAZE[f][y][x].multiplier = (rand() % 2) + 2;
                }
            }
        }
    }
}

/* END of maze.c */
