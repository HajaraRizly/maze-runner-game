#ifndef MAZE_H
#define MAZE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>  

/*Constants */

#define W 10   // width
#define L 25   // length
#define F 3    // floors

#define IN_RANGE(v,a,b) ((v) >= (a) && (v) <= (b))
#define STAIRS_CHANGE_INTERVAL 5  // change every 5 rounds

/* Bawana area constants */
#define BAWANA_FLOOR 0
#define BAWANA_X1 6
#define BAWANA_X2 9
#define BAWANA_Y1 20
#define BAWANA_Y2 24
#define BAWANA_ENTRANCE_X 9
#define BAWANA_ENTRANCE_Y 19

#define CLAMP_MP(mp)  do { if ((mp) > 250) (mp) = 250; } while (0)


/* Types */
typedef enum { NORTH=1, EAST=2, SOUTH=3, WEST=4 } Dir;
typedef enum { NORMAL, POISONED, DISORIENTED, TRIGGERED, HAPPY } Status;

typedef struct {
    const char *name;
    int f;
    int x, y;
    int spawn_x, spawn_y;
    int first_x, first_y;
    Dir dir;
    bool in_maze;
    int turn_count;
    int movement_points;

    /* Bawana */
    Status status;
    int skip_turns;           /* for poisoning */
    int disoriented_turns;    /* for disoriented effect */
} Player;

typedef struct {
    int consumable;   /* 0–4 movement cost */
    int bonus;        /* +1 to +5 movement bonus */
    int multiplier;   /* 2 or 3 multiplier */
    bool is_wall;     /* true if blocked */
} Cell;

typedef struct {
    int f;
    int x;
    int y;
} CellCoord;

typedef struct {
    int f1, x1, y1;  /* start cell */
    int f2, x2, y2;  /* end cell */
} Stair;

typedef struct {
    int f1, f2;   /* start floor (higher), end floor (lower) */
    int x, y;     /* same cell position across floors */
} Pole;

typedef struct {
    int f;
    int x1, y1;
    int x2, y2;
} Wall;

typedef struct { int f,x,y; } Flag;

/*Globals*/
extern Cell MAZE[F][L][W];

extern Stair *STAIRS;
extern int N_STAIRS;
extern int *STAIRS_DIR;

extern Pole *POLES;
extern int N_POLES;

extern Wall *WALLS;
extern int N_WALLS;

extern CellCoord *INVALID_CELLS;
extern int N_INVALID_CELLS;

extern Flag FLAG;

extern Player PLAYERS[];
extern const int N_PLAYERS;

/* = Prototypes = */
/* initialization */
void init_maze_cells(void);
void count_blocks(void);

/* dice */
int roll_movement_dice(void);
int roll_direction_dice(void);

// transport
bool stairs_overlap(Stair *a, Stair *b);
bool cell_occupied(int f, int x, int y);

/* file loaders */
bool load_seed(const char *filename);
bool load_stairs(const char *filename);
bool load_poles(const char *filename);
bool load_walls(const char *filename);
bool load_flag(const char *filename);

/* walls/maze helpers */
void mark_wall_cells(void);
bool is_blocked_by_wall(int f, int x1, int y1, int x2, int y2);

/* flag/bawana */
bool is_flag_cell(int f, int x, int y);
bool is_bawana_cell(int f, int x, int y);
bool is_bawana_entrance(int f, int x, int y);
void apply_bawana_effect(Player *p);

/* cell presence */
bool cell_present(int f, int x, int y);

/* movement / transport */
bool apply_stairs(Player *p);
bool apply_poles(Player *p);
bool apply_transport_if_present(Player *p);
void try_enter_maze(Player *p, int move_roll);
void move_player(Player *p, int move_roll, int dir_roll);
void apply_dir_roll(Player *p, int dir_roll);

/* utils */
const char* dir_to_string(Dir d);
void check_capture(Player *p);

#endif 
