#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_QUESTION_LENGTH 256
#define MAX_OPTION_LENGTH 128
#define MAX_NAME_LENGTH 50
#define MAX_QUESTIONS 100
#define MAX_STUDENTS 50

// Structure definitions
typedef struct {
    char question[MAX_QUESTION_LENGTH];
    char options[4][MAX_OPTION_LENGTH];
    int correct_option;
    int marks;
    int question_id;
} Question;

typedef struct {
    int student_id;
    char name[MAX_NAME_LENGTH];
    int score;
    double time_taken;
    int *answers; // Dynamic array for student answers
    int total_questions;
    Question *quiz_questions; // Store the actual questions used in quiz
} StudentResult;

typedef struct {
    Question *questions; // Dynamic array for questions
    int question_count;
    int max_questions;
} QuestionBank;

typedef struct {
    StudentResult *results; // Dynamic array for student results
    int student_count;
    int max_students;
} ResultManager;

// Function prototypes
void initializeQuestionBank(QuestionBank *bank, int initial_size);
void initializeResultManager(ResultManager *manager, int initial_size);
void addQuestion(QuestionBank *bank);
void displayQuestions(const QuestionBank *bank);
void shuffleQuestions(Question *questions, int count);
void takeQuiz(const QuestionBank *bank, ResultManager *manager);
void evaluateQuiz(StudentResult *student, const QuestionBank *bank);
void generateStudentReport(const StudentResult *student, const QuestionBank *bank);
void generateClassReport(const ResultManager *manager);
void saveResultsToFile(const ResultManager *manager);
void freeMemory(QuestionBank *bank, ResultManager *manager);
void displayMenu();
int getTotalPossibleMarks(Question *questions, int count); // Added missing prototype

// Utility functions
int getIntegerInput(const char *prompt, int min, int max);
void clearInputBuffer();
void toLowerCase(char *str);

int main() {
    srand(time(NULL)); // Seed for randomization
    
    QuestionBank question_bank;
    ResultManager result_manager;
    
    initializeQuestionBank(&question_bank, 10);
    initializeResultManager(&result_manager, 10);
    
    int choice;
    
    printf("=== Dynamic Quiz Creator & Evaluator ===\n");
    
    do {
        displayMenu();
        choice = getIntegerInput("Enter your choice: ", 1, 7);
        
        switch(choice) {
            case 1:
                addQuestion(&question_bank);
                break;
            case 2:
                displayQuestions(&question_bank);
                break;
            case 3:
                if(question_bank.question_count > 0) {
                    takeQuiz(&question_bank, &result_manager);
                } else {
                    printf("No questions available! Please add questions first.\n");
                }
                break;
            case 4:
                if(result_manager.student_count > 0) {
                    int student_id;
                    printf("Enter student ID to view report: ");
                    scanf("%d", &student_id);
                    clearInputBuffer();
                    
                    int found = 0;
                    for(int i = 0; i < result_manager.student_count; i++) {
                        if(result_manager.results[i].student_id == student_id) {
                            generateStudentReport(&result_manager.results[i], &question_bank);
                            found = 1;
                            break;
                        }
                    }
                    if(!found) {
                        printf("Student ID not found!\n");
                    }
                } else {
                    printf("No student results available!\n");
                }
                break;
            case 5:
                if(result_manager.student_count > 0) {
                    generateClassReport(&result_manager);
                } else {
                    printf("No student results available!\n");
                }
                break;
            case 6:
                saveResultsToFile(&result_manager);
                break;
            case 7:
                printf("Exiting program. Goodbye!\n");
                break;
            default:
                printf("Invalid choice! Please try again.\n");
        }
        
        printf("\n");
    } while(choice != 7);
    
    freeMemory(&question_bank, &result_manager);
    return 0;
}

void initializeQuestionBank(QuestionBank *bank, int initial_size) {
    bank->questions = (Question*)malloc(initial_size * sizeof(Question));
    if(bank->questions == NULL) {
        printf("Memory allocation failed for question bank!\n");
        exit(1);
    }
    bank->question_count = 0;
    bank->max_questions = initial_size;
}

void initializeResultManager(ResultManager *manager, int initial_size) {
    manager->results = (StudentResult*)malloc(initial_size * sizeof(StudentResult));
    if(manager->results == NULL) {
        printf("Memory allocation failed for result manager!\n");
        exit(1);
    }
    manager->student_count = 0;
    manager->max_students = initial_size;
}

void addQuestion(QuestionBank *bank) {
    if(bank->question_count >= bank->max_questions) {
        // Resize the array
        bank->max_questions *= 2;
        Question temp = (Question)realloc(bank->questions, bank->max_questions * sizeof(Question));
        if(temp == NULL) {
            printf("Memory reallocation failed for question bank!\n");
            return;
        }
        bank->questions = temp;
        printf("Question bank resized to %d questions\n", bank->max_questions);
    }
    
    Question *q = &bank->questions[bank->question_count];
    q->question_id = bank->question_count + 1;
    
    clearInputBuffer();
    
    printf("Enter question: ");
    fgets(q->question, MAX_QUESTION_LENGTH, stdin);
    q->question[strcspn(q->question, "\n")] = 0; // Remove newline
    
    printf("Enter option A: ");
    fgets(q->options[0], MAX_OPTION_LENGTH, stdin);
    q->options[0][strcspn(q->options[0], "\n")] = 0;
    
    printf("Enter option B: ");
    fgets(q->options[1], MAX_OPTION_LENGTH, stdin);
    q->options[1][strcspn(q->options[1], "\n")] = 0;
    
    printf("Enter option C: ");
    fgets(q->options[2], MAX_OPTION_LENGTH, stdin);
    q->options[2][strcspn(q->options[2], "\n")] = 0;
    
    printf("Enter option D: ");
    fgets(q->options[3], MAX_OPTION_LENGTH, stdin);
    q->options[3][strcspn(q->options[3], "\n")] = 0;
    
    q->correct_option = getIntegerInput("Enter correct option (1-4): ", 1, 4);
    q->marks = getIntegerInput("Enter marks for this question: ", 1, 10);
    
    bank->question_count++;
    printf("Question added successfully! (ID: %d)\n", q->question_id);
}

void displayQuestions(const QuestionBank *bank) {
    if(bank->question_count == 0) {
        printf("No questions available!\n");
        return;
    }
    
    printf("\n=== QUESTION BANK (%d questions) ===\n", bank->question_count);
    for(int i = 0; i < bank->question_count; i++) {
        const Question *q = &bank->questions[i];
        printf("\nQuestion %d (ID: %d, Marks: %d)\n", i+1, q->question_id, q->marks);
        printf("Q: %s\n", q->question);
        printf("A) %s\n", q->options[0]);
        printf("B) %s\n", q->options[1]);
        printf("C) %s\n", q->options[2]);
        printf("D) %s\n", q->options[3]);
        printf("Correct: %c\n", 'A' + q->correct_option - 1);
    }
}

void shuffleQuestions(Question *questions, int count) {
    // Fisher-Yates shuffle algorithm
    for(int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Question temp = questions[i];
        questions[i] = questions[j];
        questions[j] = temp;
    }
}

void takeQuiz(const QuestionBank *bank, ResultManager *manager) {
    if(manager->student_count >= manager->max_students) {
        manager->max_students *= 2;
        StudentResult temp = (StudentResult)realloc(manager->results, manager->max_students * sizeof(StudentResult));
        if(temp == NULL) {
            printf("Memory reallocation failed for result manager!\n");
            return;
        }
        manager->results = temp;
        printf("Result manager resized to %d students\n", manager->max_students);
    }
    
    StudentResult *student = &manager->results[manager->student_count];
    
    // Get student information
    student->student_id = manager->student_count + 1;
    clearInputBuffer();
    printf("Enter your name: ");
    fgets(student->name, MAX_NAME_LENGTH, stdin);
    student->name[strcspn(student->name, "\n")] = 0;
    
    int quiz_questions;
    do {
        quiz_questions = getIntegerInput("How many questions would you like in the quiz? ", 1, bank->question_count);
        if(quiz_questions > bank->question_count) {
            printf("Not enough questions in bank! Available: %d\n", bank->question_count);
        }
    } while(quiz_questions > bank->question_count);
    
    // Create a copy of questions for shuffling
    Question quiz_set = (Question)malloc(quiz_questions * sizeof(Question));
    student->quiz_questions = (Question*)malloc(quiz_questions * sizeof(Question)); // Store for evaluation
    int used_indices = (int)calloc(bank->question_count, sizeof(int));
    
    // Select random questions without replacement
    for(int i = 0; i < quiz_questions; i++) {
        int index;
        do {
            index = rand() % bank->question_count;
        } while(used_indices[index]);
        
        quiz_set[i] = bank->questions[index];
        student->quiz_questions[i] = bank->questions[index]; // Store for evaluation
        used_indices[index] = 1;
    }
    
    free(used_indices);
    
    // Shuffle the selected questions
    shuffleQuestions(quiz_set, quiz_questions);
    
    // Allocate memory for student answers
    student->answers = (int*)malloc(quiz_questions * sizeof(int));
    student->total_questions = quiz_questions;
    student->score = 0;
    
    printf("\n=== QUIZ STARTED ===\n");
    printf("You have %d questions. Good luck!\n\n", quiz_questions);
    
    time_t start_time = time(NULL);
    
    // Present questions
    for(int i = 0; i < quiz_questions; i++) {
        printf("Question %d/%d (Marks: %d)\n", i+1, quiz_questions, quiz_set[i].marks);
        printf("Q: %s\n", quiz_set[i].question);
        printf("A) %s\n", quiz_set[i].options[0]);
        printf("B) %s\n", quiz_set[i].options[1]);
        printf("C) %s\n", quiz_set[i].options[2]);
        printf("D) %s\n", quiz_set[i].options[3]);
        
        int answer = getIntegerInput("Your answer (1-4): ", 1, 4);
        student->answers[i] = answer;
        
        printf("\n");
    }
    
    time_t end_time = time(NULL);
    student->time_taken = difftime(end_time, start_time);
    
    // Evaluate the quiz
    evaluateQuiz(student, bank);
    
    int total_possible_marks = getTotalPossibleMarks(quiz_set, quiz_questions);
    printf("Quiz completed! Time taken: %.2f seconds\n", student->time_taken);
    printf("Your score: %d/%d\n", student->score, total_possible_marks);
    
    manager->student_count++;
    
    free(quiz_set);
}

// Fixed: Added the missing function
int getTotalPossibleMarks(Question *questions, int count) {
    int total = 0;
    for(int i = 0; i < count; i++) {
        total += questions[i].marks;
    }
    return total;
}

void evaluateQuiz(StudentResult *student, const QuestionBank *bank) {
    student->score = 0;
    for(int i = 0; i < student->total_questions; i++) {
        if(student->answers[i] == student->quiz_questions[i].correct_option) {
            student->score += student->quiz_questions[i].marks;
        }
    }
}

void generateStudentReport(const StudentResult *student, const QuestionBank *bank) {
    printf("\n=== STUDENT REPORT ===\n");
    printf("Student ID: %d\n", student->student_id);
    printf("Name: %s\n", student->name);
    printf("Score: %d/%d\n", student->score, getTotalPossibleMarks(student->quiz_questions, student->total_questions));
    printf("Time Taken: %.2f seconds\n", student->time_taken);
    printf("Questions Attempted: %d\n", student->total_questions);
    
    // Calculate percentage
    int total_marks = getTotalPossibleMarks(student->quiz_questions, student->total_questions);
    double percentage = (total_marks > 0) ? (student->score * 100.0 / total_marks) : 0;
    printf("Percentage: %.2f%%\n", percentage);
    
    // Show question-wise performance
    printf("\nQuestion-wise Performance:\n");
    for(int i = 0; i < student->total_questions; i++) {
        printf("Q%d: Your answer: %c, Correct answer: %c - %s\n", 
               i+1,
               'A' + student->answers[i] - 1,
               'A' + student->quiz_questions[i].correct_option - 1,
               (student->answers[i] == student->quiz_questions[i].correct_option) ? "Correct" : "Incorrect");
    }
}

void generateClassReport(const ResultManager *manager) {
    if(manager->student_count == 0) {
        printf("No student data available!\n");
        return;
    }
    
    printf("\n=== CLASS REPORT ===\n");
    printf("Total Students: %d\n\n", manager->student_count);
    
    // Calculate statistics
    int total_score = 0;
    double total_time = 0;
    int top_score = 0;
    char top_student[MAX_NAME_LENGTH] = "";
    
    printf("Individual Results:\n");
    printf("ID\tName\t\tScore\tTime(s)\n");
    printf("----------------------------------------\n");
    
    for(int i = 0; i < manager->student_count; i++) {
        const StudentResult *s = &manager->results[i];
        int total_possible = getTotalPossibleMarks(s->quiz_questions, s->total_questions);
        printf("%d\t%-15s\t%d/%d\t%.2f\n", s->student_id, s->name, s->score, total_possible, s->time_taken);
        
        total_score += s->score;
        total_time += s->time_taken;
        
        if(s->score > top_score) {
            top_score = s->score;
            strcpy(top_student, s->name);
        }
    }
    
    printf("\nClass Statistics:\n");
    printf("Average Score: %.2f\n", (double)total_score / manager->student_count);
    printf("Average Time: %.2f seconds\n", total_time / manager->student_count);
    printf("Top Performer: %s (Score: %d)\n", top_student, top_score);
}

void saveResultsToFile(const ResultManager *manager) {
    if(manager->student_count == 0) {
        printf("No results to save!\n");
        return;
    }
    
    FILE *file = fopen("quiz_results.txt", "w");
    if(file == NULL) {
        printf("Error opening file for writing!\n");
        return;
    }
    
    fprintf(file, "Quiz Results Report\n");
    fprintf(file, "===================\n\n");
    fprintf(file, "Total Students: %d\n\n", manager->student_count);
    
    for(int i = 0; i < manager->student_count; i++) {
        const StudentResult *s = &manager->results[i];
        int total_possible = getTotalPossibleMarks(s->quiz_questions, s->total_questions);
        fprintf(file, "Student ID: %d\n", s->student_id);
        fprintf(file, "Name: %s\n", s->name);
        fprintf(file, "Score: %d/%d\n", s->score, total_possible);
        fprintf(file, "Time Taken: %.2f seconds\n", s->time_taken);
        fprintf(file, "Questions Attempted: %d\n", s->total_questions);
        
        // Calculate percentage
        double percentage = (total_possible > 0) ? (s->score * 100.0 / total_possible) : 0;
        fprintf(file, "Percentage: %.2f%%\n", percentage);
        fprintf(file, "------------------------\n");
    }
    
    fclose(file);
    printf("Results saved to 'quiz_results.txt'\n");
}

void freeMemory(QuestionBank *bank, ResultManager *manager) {
    // Free question bank
    free(bank->questions);
    
    // Free student results and their answer arrays
    for(int i = 0; i < manager->student_count; i++) {
        free(manager->results[i].answers);
        free(manager->results[i].quiz_questions);
    }
    free(manager->results);
}

void displayMenu() {
    printf("Main Menu:\n");
    printf("1. Add Question\n");
    printf("2. Display All Questions\n");
    printf("3. Take Quiz\n");
    printf("4. View Student Report\n");
    printf("5. Generate Class Report\n");
    printf("6. Save Results to File\n");
    printf("7. Exit\n");
}

int getIntegerInput(const char *prompt, int min, int max) {
    int value;
    while(1) {
        printf("%s", prompt);
        if(scanf("%d", &value) == 1) {
            if(value >= min && value <= max) {
                return value;
            }
        }
        printf("Invalid input! Please enter a number between %d and %d.\n", min, max);
        clearInputBuffer();
    }
}

void clearInputBuffer() {
    int c;
    while((c = getchar()) != '\n' && c != EOF);
}

void toLowerCase(char *str) {
    for(int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}
