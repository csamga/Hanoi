#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termio.h>
#include <unistd.h>

#define N_TOWERS 3
#define MAX_DISCS 10

#define CSI "\x1b["

static char *disc_str[11] = {
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

static struct termios term_old;
static short n_discs;

struct tower {
    short discs[MAX_DISCS];
    short n_discs;
    bool selected;
};

void save_cursor_pos(void) { fputs(CSI "s", stdout); }
void restore_cursor_pos(void) { fputs(CSI "u", stdout); }

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

bool tower_push_disc(struct tower *tower, short disc) {
    if (
        (tower->n_discs == 0) ||
        ((tower->n_discs > 0) &&
         (tower->n_discs < n_discs) &&
         (disc < tower->discs[tower->n_discs - 1])))
    {
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
            /* TODO: print appropriate number of spaces if no disc */
            if (
                towers[t].selected &&
                d == towers[t].n_discs - 1)
            {
                printf(
                    CSI "7m" "%*.*s" CSI "0m" " ",
                    (1 + 2 * (n_discs - 1)),
                    (1 + 2 * (n_discs - 1)),
                    (MAX_DISCS - n_discs) + disc_str[towers[t].discs[d]]
                );
            } else {
                printf(
                    "%*.*s ",
                    (1 + 2 * (n_discs - 1)),
                    (1 + 2 * (n_discs - 1)),
                    (MAX_DISCS - n_discs) + disc_str[towers[t].discs[d]]
                );
            }
        }

        puts("\b");
    }
}

int main(void) {
    struct tower towers[N_TOWERS];
    short i, from, to, tmp_len;
    bool finished, quit, valid;
    char c, tmp[8];

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
            while ((c = getchar()) != '\n' && c != EOF) {}
        }
    } while (n_discs < 3 || n_discs > MAX_DISCS);

    /* TODO: erase lines when input valid */

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

    while (!finished && !quit) {
        restore_cursor_pos();
        display_state(towers);

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
            }
        } while (
            !quit &&
            (from < 0 || from > N_TOWERS) &&
            towers[from].n_discs == 0
        );

        if (!quit) {
            towers[from].selected = true;

            restore_cursor_pos();
            display_state(towers);

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
                }
            } while (
                !quit &&
                (to < 0 || to > N_TOWERS) &&
                to == from
            );

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

    term_reset();

    return 0;
}
