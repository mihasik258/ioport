/*
I thought that the basic version of the terminal
would be enough to make it work, so I didn't
implement the cursor initially.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "io_access.h"
#include "command_processor.h"

#define MAX_INPUT_LENGTH 1024
#define HISTORY_BUFFER_SIZE 4096
#define MAX_CMD_LENGTH 16
#define MAX_ARG_LENGTH 32

#define KEY_BACKSPACE 127
#define KEY_DELETE 8
#define KEY_ESCAPE 27
#define KEY_ENTER 10
#define KEY_CTRL_C 3
#define KEY_CTRL_D 4
#define KEY_LEFT_BRACKET 91
#define KEY_UP_ARROW 65
#define KEY_DOWN_ARROW 66
#define KEY_LEFT_ARROW 68
#define KEY_RIGHT_ARROW 67

#define ASCII_SPACE 32
#define ASCII_DEL 127

#define NULL_TERMINATOR '\0'
#define NEWLINE_CHAR '\n'

static struct termios original_termios_global;
static int termios_initialized = 0;

typedef struct history_entry {
    char *command;
    struct history_entry *prev;
    struct history_entry *next;
} history_entry_t;

static history_entry_t *history_head = NULL;
static history_entry_t *history_tail = NULL;
static history_entry_t *history_current = NULL;
static size_t history_memory_used = 0;
static size_t history_count = 0;
static char unfinished_input[MAX_INPUT_LENGTH] = {0};
static bool has_unfinished_input = false;

static void add_to_history(const char *command) {
    size_t command_len = strlen(command) + 1;
    while (history_memory_used + command_len > HISTORY_BUFFER_SIZE && history_head != NULL) {
        history_entry_t *oldest = history_head;
        size_t oldest_len = strlen(oldest->command) + 1;
        history_memory_used -= oldest_len;
        history_count--;
        history_head = oldest->next;
        if (history_head != NULL) {
            history_head->prev = NULL;
        } else {
            history_tail = NULL;
        }        
        free(oldest->command);
        free(oldest);
    }
    history_entry_t *new_entry = malloc(sizeof(history_entry_t));
    if (new_entry == NULL) {
        return; 
    }
    new_entry->command = strdup(command);
    if (new_entry->command == NULL) {
        free(new_entry);
        return; 
    }
    
    new_entry->prev = history_tail;
    new_entry->next = NULL;
    if (history_tail != NULL) {
        history_tail->next = new_entry;
    } else {
        history_head = new_entry;
    }
    history_tail = new_entry;
    
    history_memory_used += command_len;
    history_count++;
    history_current = NULL;
}

static void free_history(void) {
    history_entry_t *current = history_head;
    while (current != NULL) {
        history_entry_t *next = current->next;
        free(current->command);
        free(current);
        current = next;
    }
    history_head = NULL;
    history_tail = NULL;
    history_current = NULL;
    history_memory_used = 0;
    history_count = 0;
}

// Restores terminal state on SIGINT / SIGTERM
static void sigint_handler(int sig) {
    if (termios_initialized) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios_global);
    }
    if (sig == SIGINT) {
        printf("\nReceived interrupt signal\n");
    } else if (sig == SIGTERM) {
        printf("\nReceived termination signal\n");
    }
    exit(1);
}

static void set_terminal_mode(struct termios *original) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, original);
    new_termios = *original;
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

static void reset_terminal_mode(struct termios *original) {
    tcsetattr(STDIN_FILENO, TCSANOW, original);
}
static char* read_line(void) {
    static char buffer[MAX_INPUT_LENGTH];
    int pos = 0;
    int cursor_pos = 0;
    int c;
    buffer[0] = NULL_TERMINATOR;
    if (has_unfinished_input) {
        strncpy(buffer, unfinished_input, MAX_INPUT_LENGTH);
        pos = strlen(buffer);
        cursor_pos = pos;
        has_unfinished_input = false;
    }
    struct termios original_termios;
    set_terminal_mode(&original_termios);
    printf("io> ");
    if (pos > 0) {
        printf("%s", buffer);
    }
    fflush(stdout);
    while (1) {
        c = getchar();
        if (c == NEWLINE_CHAR) {
            buffer[pos] = NULL_TERMINATOR;
            printf("\n");
            if (pos > 0) {
                add_to_history(buffer);
            }
            history_current = NULL;
            unfinished_input[0] = NULL_TERMINATOR;
            has_unfinished_input = false;
            reset_terminal_mode(&original_termios);
            return buffer;
        }
        else if (c == KEY_BACKSPACE || c == KEY_DELETE) {
            if (pos > 0 && cursor_pos > 0) {
                memmove(&buffer[cursor_pos - 1], &buffer[cursor_pos], pos - cursor_pos + 1);
                pos--;
                cursor_pos--;
                printf("\b");
                for (int i = cursor_pos; i < pos; i++) {
                    printf("%c", buffer[i]);
                }
                printf(" ");
                for (int i = 0; i <= pos - cursor_pos; i++) {
                    printf("\b");
                }
                fflush(stdout);
            }
        }
        else if (c == KEY_ESCAPE) {
            c = getchar();
            if (c == KEY_LEFT_BRACKET) {
                c = getchar();
                if (c == KEY_UP_ARROW) {
                    if (history_current == NULL && pos > 0) {
                        strncpy(unfinished_input, buffer, MAX_INPUT_LENGTH);
                        has_unfinished_input = true;
                    }
                    if (history_tail != NULL) {
                        if (history_current == NULL) {
                            history_current = history_tail;
                        }
                        else if (history_current->prev != NULL) {
                            history_current = history_current->prev;
                        }
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        strncpy(buffer, history_current->command, MAX_INPUT_LENGTH);
                        pos = strlen(buffer);
                        cursor_pos = pos;
                        printf("%s", buffer);
                    }
                    fflush(stdout);
                }
                else if (c == KEY_DOWN_ARROW) {
                    if (history_current != NULL) {
                        if (history_current->next != NULL) {
                            history_current = history_current->next;
                        }
                        else {
                            history_current = NULL;
                        }
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        if (history_current != NULL) {
                            strncpy(buffer, history_current->command, MAX_INPUT_LENGTH);
                        }
                        else if (has_unfinished_input) {
                            strncpy(buffer, unfinished_input, MAX_INPUT_LENGTH);
                        }
                        else {
                            buffer[0] = NULL_TERMINATOR;
                        }
                        pos = strlen(buffer);
                        cursor_pos = pos;
                        printf("%s", buffer);
                    }
                    fflush(stdout);
                }
                else if (c == KEY_LEFT_ARROW) {
                    if (cursor_pos > 0) {
                        cursor_pos--;
                        printf("\033[D");
                        fflush(stdout);
                    }
                }
                else if (c == KEY_RIGHT_ARROW) {
                    if (cursor_pos < pos) {
                        cursor_pos++;
                        printf("\033[C");
                        fflush(stdout);
                    }
                }
            }
        }
        else if (c >= ASCII_SPACE && c < ASCII_DEL && pos < MAX_INPUT_LENGTH - 1) {
            if (cursor_pos < pos) {
                memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], pos - cursor_pos);
            }
            buffer[cursor_pos] = c;
            pos++;
            cursor_pos++;
            buffer[pos] = NULL_TERMINATOR;
            if (cursor_pos == pos) {
                printf("%c", c);
            }
            else {
                for (int i = cursor_pos - 1; i < pos; i++) {
                    printf("%c", buffer[i]);
                }
                for (int i = 0; i < pos - cursor_pos; i++) {
                    printf("\b");
                }
            }
            fflush(stdout);
            if (has_unfinished_input) {
                has_unfinished_input = false;
                unfinished_input[0] = NULL_TERMINATOR;
            }
        }
        else if (c == KEY_CTRL_C) {
            printf("\n");
            reset_terminal_mode(&original_termios);
            return NULL;
        }
        else if (c == KEY_CTRL_D) {
            if (pos == 0) {
                printf("\n");
                reset_terminal_mode(&original_termios);
                return NULL;
            }
        }
    }
}

int main(void) {
    printf("IO Access Tool - Low-level hardware register access\n");
    if (geteuid() != 0) {
        fprintf(stderr, "Warning: Running without root privileges. Many operations will fail.\n");
    }
    else if (!io_init()) {
        fprintf(stderr, "Initialization failed. Some features may not work properly.\n");
    }
    printf("Type 'help' for available commands.\n");
    
    char* line;
    int should_exit = 0;
    
    while (!should_exit) {
        line = read_line();
        
        if (!line) {
            break;
        }
        should_exit = process_command(line);
    }
    
    io_cleanup();
    free_history();
    printf("Exiting IO Access Tool. Goodbye!\n");
    
    return 0;
}
