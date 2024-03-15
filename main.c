#define _POSIX_C_SOURCE 199309L
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <time.h>
#include <unistd.h>

#define N_TOWERS 3
#define MAX_DISCS 10

#define CSI "\x1b["

struct tower {
    short discs[MAX_DISCS];
    short n_discs;
    bool selected;
};

char *disc_str[11] = {
    "                   ",
    "         -         ",
    "        ---        ",
    "       -----       ",
    "      -------      ",
    "     ---------     ",
    "    -----------    ",
    "   -------------   ",
    "  ---------------  ",
    " ----------------- ",
    "-------------------"
};

struct termios term_old;
short n_discs;
int disc_str_max_len;

void save_cursor_pos(void) {
    fputs(CSI "s", stdout);
}

void restore_cursor_pos(void) {
    fputs(CSI "u", stdout);
}

void term_setup(void) {
    struct termios new;

    tcgetattr(STDIN_FILENO, &term_old);

    new = term_old;
    new.c_lflag &= ~(ICANON); /* make input available without typing <CR> */
    new.c_lflag &= ~(ECHO);   /* disable input echo */

    tcsetattr(STDIN_FILENO, TCSANOW, &new);
}

void term_reset(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &term_old);
}

void delay(long ms) {
  struct timespec ts;

  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;

  nanosleep(&ts, &ts);
}

bool tower_push_disc(struct tower *tower, short disc) {
    if (
        (tower->n_discs == 0) ||
        ((tower->n_discs > 0) &&
         (tower->n_discs < n_discs) &&
         (disc < tower->discs[tower->n_discs - 1]))
    ) {
        tower->discs[tower->n_discs] = disc;
        tower->n_discs++;

        return true;
    }

    return false;
}

bool tower_pop_disc(struct tower *tower, short *disc) {
    if (tower->n_discs > 0) {
        tower->n_discs--;
        *disc = tower->discs[tower->n_discs];
        tower->discs[tower->n_discs] = 0;

        return true;
    }

    return false;
}

void move_disc(struct tower *t1, struct tower *t2) {
    short disc;

    if (tower_pop_disc(t1, &disc)) {
        if (!tower_push_disc(t2, disc)) {
            tower_push_disc(t1, disc);
        }
    }
}

void display_state(struct tower *towers) {
    short d, t;

    for (d = n_discs - 1; d >= 0; d--) {
        for (t = 0; t < N_TOWERS; t++) {
            if (
                towers[t].selected &&
                d == towers[t].n_discs - 1
            ) {
                printf(
                    CSI "7m" "%*.*s" CSI "0m" " ",
                    disc_str_max_len,
                    disc_str_max_len,
                    (MAX_DISCS - n_discs) + disc_str[towers[t].discs[d]]
                );
            } else {
                printf(
                    "%*.*s ",
                    disc_str_max_len,
                    disc_str_max_len,
                    (MAX_DISCS - n_discs) + disc_str[towers[t].discs[d]]
                );
            }
        }

        puts("\b");
    }
}

void hanoi(int n, int t1, int t2, int t3, struct tower *towers) {
    if (n > 0) {
        hanoi(n - 1, t1, t3, t2, towers);

        restore_cursor_pos();
        display_state(towers);
        delay(50);

        towers[t1].selected = true;

        restore_cursor_pos();
        display_state(towers);

        move_disc(&towers[t1], &towers[t3]);
        towers[t1].selected = false;
        delay(50);

        hanoi(n - 1, t2, t1, t3, towers);
    }
}

int main(int argc, char *argv[]) {
    struct tower towers[N_TOWERS];
    short i, from, to, tmp_len;
    bool finished, quit, valid, auto_run;
    char c, tmp[8];

    if (argc > 1) {
        auto_run = (strcmp(argv[1], "--auto") == 0);
    }

    do {
        fputs("Nombre de disques : ", stdout);
        fgets(tmp, sizeof tmp, stdin);

        tmp[strcspn(tmp, "\n")] = '\0';
        tmp_len = (short)strlen(tmp);

        valid = true;

        for (i = 0; i < tmp_len && valid; i++) {
            valid = (valid && isdigit(tmp[i]));
        }

        n_discs = (short)valid * atoi(tmp);

        if (tmp_len > (short)(sizeof tmp - 2)) {
            while (((c = getchar()) != '\n') && (c != EOF)) {}
        }
    } while (n_discs < 3 || n_discs > MAX_DISCS);
    /* TODO: erase lines when input valid */

    disc_str_max_len = 1 + 2 * (n_discs - 1);

    memset(towers, 0, N_TOWERS * sizeof *towers);

    for (i = n_discs; i >= 0; i--) {
        tower_push_disc(&towers[0], i);
    }

    term_setup();

    fprintf(stdout, CSI "%dS", n_discs + 1);
    fprintf(stdout, CSI "%dA", n_discs + 1);
    save_cursor_pos();

    /* TODO: implement replay whether by adding a disc or by asking */
    finished = false;
    quit = false;

    if (auto_run) {
        hanoi(n_discs, 0, 1, 2, towers);
        restore_cursor_pos();
        display_state(towers);
    } else {
        while (!finished && !quit) {
            restore_cursor_pos();
            display_state(towers);

            do {
                do {
                    c = getchar();

                    switch (c) {
                    case 'a':
                    case 'A':
                        from = 0;
                        break;
                    case 'z':
                    case 'Z':
                        from = 1;
                        break;
                    case 'e':
                    case 'E':
                        from = 2;
                        break;
                    case 'q':
                    case 'Q':
                    case '\x1b':
                        quit = true;
                        break;
                    default:
                        from = -1;
                    }
                } while (
                    !quit &&
                    (from < 0 || from > N_TOWERS)
                );
            } while (towers[from].n_discs == 0 && !quit);
            /* TODO: fix this logic mess */

            if (!quit) {
                towers[from].selected = true;

                restore_cursor_pos();
                display_state(towers);

                do {
                    do {
                        c = getchar();

                        switch (c) {
                        case 'a':
                        case 'A':
                            to = 0;
                            break;
                        case 'z':
                        case 'Z':
                            to = 1;
                            break;
                        case 'e':
                        case 'E':
                            to = 2;
                            break;
                        case 'q':
                        case 'Q':
                        case '\x1b':
                            quit = true;
                            break;
                        default:
                            to = -1;
                        }
                    } while (
                        !quit &&
                        (to < 0 || to > N_TOWERS)
                    );
                } while (to == from && !quit);
                /* TODO: fix this logic mess */

                if (!quit) {
                    move_disc(&towers[from], &towers[to]);
                    towers[from].selected = false;
                    finished = towers[N_TOWERS - 1].n_discs == n_discs;
                }
            }
        }

        if (finished) {
            restore_cursor_pos();
            display_state(towers);
            puts("fin X"); /* crap */
            getchar();
        }
    }

    term_reset();

    return 0;
}
