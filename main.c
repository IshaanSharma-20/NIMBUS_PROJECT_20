/* quiz.c - corrected & robust Question Bank functions + quiz delivery
   Minimal self-contained example implementing the functions from your snippet.
   Compile: gcc -std=c11 -O2 -Wall -Wextra quiz.c -o quiz
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* ---------- Types ---------- */

typedef struct {
    char *text;
    char **options;
    int option_count;
    int correct;           /* index (0..option_count-1) */
    char category[64];
    char difficulty[32];
    int marks;
} Question;

typedef struct {
    Question *questions;
    int count;
    int capacity;
} QuestionBank;

typedef struct {
    char name[64];
    int given_count;
    int *correct_flags;
    int score;
    int total_marks;
    double time_taken;
} StudentResult;

/* ---------- Color macros (no-op if terminal doesn't support) ---------- */
#define RESET  ""
#define RED    ""
#define GREEN  ""
#define YELLOW ""
#define CYAN   ""
#define MAGENTA ""

/* ---------- Helpers ---------- */

static char *strdup_or_die(const char *s) {
    if (!s) return NULL;
    char *d = strdup(s);
    if (!d) {
        fprintf(stderr, "Out of memory (strdup)\n");
        exit(EXIT_FAILURE);
    }
    return d;
}

/* Read a line from stdin, return malloc'd string (caller frees) */
char *read_line(void) {
    char *buf = NULL;
    size_t len = 0;
    ssize_t n = getline(&buf, &len, stdin);
    if (n <= 0) {
        if (buf) free(buf);
        /* return empty string to simplify callers */
        return strdup_or_die("");
    }
    /* remove trailing newline */
    if (n > 0 && buf[n-1] == '\n') buf[n-1] = '\0';
    return buf;
}

/* Fisher-Yates shuffle for int arrays */
void shuffle_int(int *arr, int n) {
    if (n <= 1) return;
    for (int i = n-1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

/* Simple progress indicator (prints fraction) */
void progress(int done, int total) {
    printf("\rProgress: %d/%d", done, total);
    if (done == total) printf("\n");
    fflush(stdout);
}

/* ---------- Question bank functions ---------- */

void qb_init(QuestionBank *qb) {
    if (!qb) return;
    qb->questions = NULL;
    qb->count = 0;
    qb->capacity = 0;
}

static void free_question(Question *q) {
    if (!q) return;
    free(q->text);
    if (q->options) {
        for (int i = 0; i < q->option_count; ++i)
            free(q->options[i]);
        free(q->options);
        q->options = NULL;
    }
    q->option_count = 0;
    q->correct = 0;
    q->marks = 0;
    q->category[0] = '\0';
    q->difficulty[0] = '\0';
}

void qb_free(QuestionBank *qb) {
    if (!qb) return;
    for (int i = 0; i < qb->count; ++i)
        free_question(&qb->questions[i]);
    free(qb->questions);
    qb->questions = NULL;
    qb->count = qb->capacity = 0;
}

void qb_add(QuestionBank *qb, const Question *src) {
    if (!qb || !src) return;

    if (qb->count == qb->capacity) {
        int newcap = qb->capacity ? qb->capacity * 2 : 8;
        Question *p = realloc(qb->questions, newcap * sizeof(Question));
        if (!p) {
            fprintf(stderr, "Failed to allocate memory for questions\n");
            return;
        }
        qb->questions = p;
        qb->capacity = newcap;
    }

    Question *q = &qb->questions[qb->count];
    /* Initialize destination Question to safe defaults */
    q->text = NULL;
    q->options = NULL;
    q->option_count = 0;
    q->correct = 0;
    q->marks = 0;
    q->category[0] = '\0';
    q->difficulty[0] = '\0';

    /* Deep copy fields */
    q->text = strdup_or_die(src->text);
    strncpy(q->category, src->category, sizeof(q->category)-1);
    q->category[sizeof(q->category)-1] = '\0';
    strncpy(q->difficulty, src->difficulty, sizeof(q->difficulty)-1);
    q->difficulty[sizeof(q->difficulty)-1] = '\0';
    q->option_count = src->option_count;
    q->correct = src->correct;
    q->marks = src->marks;

    if (q->option_count > 0) {
        q->options = malloc(sizeof(char*) * q->option_count);
        if (!q->options) {
            fprintf(stderr, "Out of memory (options)\n");
            free_question(q);
            return;
        }
        for (int i = 0; i < src->option_count; ++i) {
            q->options[i] = strdup_or_die(src->options[i]);
        }
    } else {
        q->options = NULL;
    }

    qb->count++;
}

/* ---------- Save result ---------- */

void save_result(const StudentResult *r) {
    if (!r) return;
    FILE *fp = fopen("results.txt", "a");
    if (!fp) {
        perror("fopen results.txt");
        return;
    }
    fprintf(fp, "Name: %s | Score: %d/%d | Time: %.0f sec\n",
            r->name, r->score, r->total_marks, r->time_taken);
    fclose(fp);
}

/* ---------- Deliver quiz ---------- */

StudentResult deliver_quiz(QuestionBank *qb, int count, int time_limit) {
    StudentResult r;
    memset(&r, 0, sizeof(r));

    if (!qb) return r;

    printf("Enter your name: ");
    char *nm = read_line();
    strncpy(r.name, nm, sizeof(r.name)-1);
    r.name[sizeof(r.name)-1] = '\0';
    free(nm);

    int qtotal = count > qb->count ? qb->count : count;
    if (qtotal <= 0) {
        printf("No questions available.\n");
        return r;
    }

    r.given_count = qtotal;
    r.correct_flags = calloc(qtotal, sizeof(int));
    if (!r.correct_flags) {
        fprintf(stderr, "Out of memory (correct_flags)\n");
        r.given_count = 0;
        return r;
    }
    r.score = 0;
    r.total_marks = 0;

    /* prepare shuffled question indices */
    int *idx = malloc(sizeof(int) * qb->count);
    if (!idx) {
        fprintf(stderr, "Out of memory (idx)\n");
        free(r.correct_flags);
        r.correct_flags = NULL;
        r.given_count = 0;
        return r;
    }
    for (int i = 0; i < qb->count; ++i) idx[i] = i;
    shuffle_int(idx, qb->count);

    printf("\nStarting Quiz...\n");

    time_t start = time(NULL);

    for (int i = 0; i < qtotal; ++i) {
        progress(i, qtotal);

        /* Time check */
        if (time_limit > 0) {
            int used = (int)difftime(time(NULL), start);
            if (used >= time_limit) {
                printf(RED "\nTIME OVER!" RESET "\n");
                break;
            }
            printf(YELLOW "(Time left: %d sec)\n" RESET, time_limit - used);
        }

        Question *q = &qb->questions[idx[i]];

        printf("\n\n%sQ%d:%s %s (%s | %s)\n", CYAN, i+1, RESET,
               q->text, q->category, q->difficulty);

        /* shuffle options */
        int *order = malloc(sizeof(int) * q->option_count);
        if (!order) {
            fprintf(stderr, "Out of memory (order)\n");
            break;
        }
        for (int j = 0; j < q->option_count; ++j) order[j] = j;
        shuffle_int(order, q->option_count);

        for (int j = 0; j < q->option_count; ++j)
            printf("  %c) %s\n", 'A' + j, q->options[order[j]]);

        printf("Marks: %d\n", q->marks);

        printf("Your answer: ");
        char *ans = read_line();
        int sel = -1;
        if (ans && strlen(ans) > 0) {
            char c = ans[0];
            if (islower((unsigned char)c)) c = toupper((unsigned char)c);
            if (c >= 'A' && c < (char)('A' + q->option_count))
                sel = c - 'A';
        }
        free(ans);

        r.total_marks += q->marks; /* counting this question's marks towards total */

        int mapped = -1;
        if (sel >= 0 && sel < q->option_count) {
            mapped = order[sel];
        } else {
            printf(RED "Invalid or no selection." RESET " Marked incorrect.\n");
        }

        if (mapped == q->correct) {
            printf(GREEN "Correct!" RESET "\n");
            r.score += q->marks;
            r.correct_flags[i] = 1;
        } else {
            /* show the correct option text (original index q->correct) */
            const char *correct_text = (q->correct >=0 && q->correct < q->option_count) ? q->options[q->correct] : "(unknown)";
            printf(RED "Incorrect." RESET " Correct: %s\n", correct_text);
        }

        free(order);
    }

    progress(qtotal, qtotal);
    r.time_taken = difftime(time(NULL), start);

    free(idx);
    /* Note: caller should call save_result(&r) and free r.correct_flags when done. */

    return r;
}

/* ---------- Quotes menu (simple) ---------- */
void quote_menu(void) {
    while (1) {
        printf("\n%s===== MOTIVATIONAL QUOTES =====%s\n", MAGENTA, RESET);
        printf("1) Random Quote\n");
        printf("2) Add Quote\n");
        printf("3) View All Quotes\n");
        printf("4) Back\n");
        printf("Choice: ");

        char *c = read_line();
        int ch = atoi(c);
        free(c);

        if (ch == 1) {
            FILE *fp = fopen("quotes.txt", "r");
            if (!fp) {
                printf("No quotes yet.\n");
                continue;
            }
            /* read lines into dynamic array */
            char *line = NULL;
            size_t cap = 0;
            ssize_t n;
            char **lines = NULL;
            int count = 0;
            while ((n = getline(&line, &cap, fp)) > 0) {
                if (n > 0 && line[n-1] == '\n') line[n-1] = '\0';
                char *copy = strdup_or_die(line);
                char **tmp = realloc(lines, sizeof(char*) * (count + 1));
                if (!tmp) { perror("realloc"); break; }
                lines = tmp;
                lines[count++] = copy;
            }
            free(line);
            fclose(fp);

            if (count == 0) {
                printf("No quotes found.\n");
            } else {
                int r = rand() % count;
                printf("Quote: %s\n", lines[r]);
            }
            for (int i = 0; lines && i < count; ++i) free(lines[i]);
            free(lines);

        } else if (ch == 2) {
            printf("Enter quote: ");
            char *q = read_line();
            FILE *fp = fopen("quotes.txt", "a");
            if (!fp) {
                perror("fopen quotes.txt");
                free(q);
                continue;
            }
            fprintf(fp, "%s\n", q);
            fclose(fp);
            free(q);
            printf("Saved.\n");

        } else if (ch == 3) {
            FILE *fp = fopen("quotes.txt", "r");
            if (!fp) { printf("No quotes.\n"); continue; }
            char *line = NULL;
            size_t cap = 0;
            while (getline(&line, &cap, fp) > 0)
                printf("%s", line); /* lines already contain newline from file */
            free(line);
            fclose(fp);

        } else if (ch == 4) {
            return;
        } else {
            printf("Invalid.\n");
        }
    }
}

/* ---------- Example main to demonstrate usage ---------- */
int main(void) {
    srand((unsigned)time(NULL));

    /* prepare a sample bank with 2 questions */
    QuestionBank qb;
    qb_init(&qb);

    Question q1;
    q1.text = "What is the capital of France?";
    q1.option_count = 4;
    q1.options = malloc(sizeof(char*) * 4);
    q1.options[0] = strdup_or_die("Berlin");
    q1.options[1] = strdup_or_die("London");
    q1.options[2] = strdup_or_die("Paris");
    q1.options[3] = strdup_or_die("Madrid");
    q1.correct = 2;
    strncpy(q1.category, "Geography", sizeof(q1.category)-1);
    strncpy(q1.difficulty, "Easy", sizeof(q1.difficulty)-1);
    q1.marks = 1;

    Question q2;
    q2.text = "2 + 2 = ?";
    q2.option_count = 3;
    q2.options = malloc(sizeof(char*) * 3);
    q2.options[0] = strdup_or_die("3");
    q2.options[1] = strdup_or_die("4");
    q2.options[2] = strdup_or_die("5");
    q2.correct = 1;
    strncpy(q2.category, "Math", sizeof(q2.category)-1);
    strncpy(q2.difficulty, "Easy", sizeof(q2.difficulty)-1);
    q2.marks = 1;

    qb_add(&qb, &q1);
    qb_add(&qb, &q2);

    /* free temp question options we created (qb_add made deep copies) */
    free_question(&q1);
    free_question(&q2);

    StudentResult r = deliver_quiz(&qb, 2, 0);
    printf("\nFinal score: %d/%d in %.0f sec\n", r.score, r.total_marks, r.time_taken);
    save_result(&r);

    free(r.correct_flags);
    qb_free(&qb);

    /* show quote menu demo (uncomment to test) */
    /* quote_menu(); */

    return 0;
}
