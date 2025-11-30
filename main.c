/* ================================================================
   QUESTION BANK MANAGEMENT BY ISHAAN SHARMA 
   ================================================================ */
void qb_init(QuestionBank *qb) {
    qb->questions = NULL;
    qb->count = 0;
    qb->capacity = 0;
}


void free_question(Question *q) {
    free(q->text);
    for (int i=0; i<q->option_count; i++)
        free(q->options[i]);
    free(q->options);
}

void qb_free(QuestionBank *qb) {
    for (int i=0; i<qb->count; i++)
        free_question(&qb->questions[i]);
    free(qb->questions);
}

void qb_add(QuestionBank *qb, Question *src) {
    if (qb->count == qb->capacity) {
        qb->capacity = qb->capacity ? qb->capacity * 2 : 8;
        qb->questions = realloc(qb->questions, qb->capacity * sizeof(Question));
    }

    Question *q = &qb->questions[qb->count++];

    q->text = strdup_s(src->text);
    strcpy(q->category, src->category);
    strcpy(q->difficulty, src->difficulty);
    q->option_count = src->option_count;
    q->correct = src->correct;
    q->marks = src->marks;

    q->options = malloc(sizeof(char*) * q->option_count);
    for (int i=0; i<src->option_count; i++)
        q->options[i] = strdup_s(src->options[i]);
}

/* ================================================================
   SAVE QUIZ RESULT
   ================================================================ */
void save_result(StudentResult *r) {
    FILE *fp = fopen("results.txt", "a");
    if (!fp) return;
    fprintf(fp, "Name: %s | Score: %d/%d | Time: %.0f sec\n",
            r->name, r->score, r->total_marks, r->time_taken);
    fclose(fp);
}

/* ================================================================
   DELIVER QUIZ
   ================================================================ */
StudentResult deliver_quiz(QuestionBank *qb, int count, int time_limit) {
    StudentResult r;

    printf("Enter your name: ");
    char *nm = read_line();
    strncpy(r.name, nm, 63);
    free(nm);

    int qtotal = count > qb->count ? qb->count : count;

    r.given_count = qtotal;
    r.correct_flags = calloc(qtotal, sizeof(int));
    r.score = 0;
    r.total_marks = 0;

    int idx[qb->count];
    for (int i=0; i<qb->count; i++) idx[i] = i;
    shuffle(idx, qb->count);

    printf("\nStarting Quiz...\n");

    time_t start = time(NULL);

    for (int i=0; i<qtotal; i++) {
        progress(i, qtotal);
        Question *q = &qb->questions[idx[i]];

        printf("\n\n%sQ%d:%s %s (%s | %s)\n", CYAN, i+1, RESET,
               q->text, q->category, q->difficulty);

        int order[q->option_count];
        for (int j=0; j<q->option_count; j++) order[j] = j;
        shuffle(order, q->option_count);

        for (int j=0; j<q->option_count; j++)
            printf("  %c) %s\n", 'A'+j, q->options[order[j]]);

        printf("Marks: %d\n", q->marks);

        if (time_limit > 0) {
            int used = (int)difftime(time(NULL), start);
            if (used >= time_limit) {
                printf(RED "TIME OVER!" RESET "\n");
                break;
            }
            printf(YELLOW "(Time left: %d sec)\n" RESET, time_limit - used);
        }

        printf("Your answer: ");
        char *ans = read_line();
        int sel = -1;
        if (strlen(ans)>0) {
            char c = ans[0];
            if (c>='a' && c<='z') c -= 32;
            if (c>='A' && c<'A'+q->option_count)
                sel = c - 'A';
        }
        free(ans);

        int mapped = order[sel];
        r.total_marks += q->marks;

        if (mapped == q->correct) {
            printf(GREEN "Correct!" RESET "\n");
            r.score += q->marks;
            r.correct_flags[i] = 1;
        } else {
            printf(RED "Incorrect." RESET " Correct: %s\n",
                   q->options[q->correct]);
        }
    }

    progress(qtotal, qtotal);
    r.time_taken = difftime(time(NULL), start);

    return r;
}
void quote_menu() {
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

            char lines[300][256];
            int n = 0;
            while (fgets(lines[n], sizeof(lines[n]), fp)) n++;
            fclose(fp);

            if (n == 0) { printf("No quotes found.\n"); continue; }

            int r = rand() % n;
            printf("Quote: %s\n", lines[r]);

        } else if (ch == 2) {
            printf("Enter quote: ");
            char *q = read_line();
            FILE *fp = fopen("quotes.txt", "a");
            fprintf(fp, "%s\n", q);
            fclose(fp);
            free(q);
            printf("Saved.\n");

        } else if (ch == 3) {
            FILE *fp = fopen("quotes.txt", "r");
            if (!fp) { printf("No quotes.\n"); continue; }
            char line[256];
            while (fgets(line, sizeof(line), fp))
                printf("%s", line);
            fclose(fp);

        } else if (ch == 4) {
            return;
        } else {
            printf("Invalid.\n");
        }
    }
}

