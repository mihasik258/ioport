/*
I thought that the basic version of the termina
l would be enough to make it work, so I didn't
 implement the cursor initially.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include "io_access.h"
#include "command_processor.h"

#define MAX_INPUT_LENGTH 1024
#define HISTORY_SIZE 100
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

#define NULL_TERMINATOR '\0'
#define NEWLINE_CHAR '\n'

static char history[HISTORY_SIZE][MAX_INPUT_LENGTH];
static int history_count = 0;
static int history_current = 0;

static void set_terminal_mode(struct termios *original) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, original);
    new_termios = *original;
    new_termios.c_lflag &= ~(ICANON | ECHO); 
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
    struct termios original_termios;
    set_terminal_mode(&original_termios);
    printf("io> ");
    fflush(stdout);
    while (1) {
        c = getchar();
        if (c == NEWLINE_CHAR) {
            buffer[pos] = NULL_TERMINATOR;
            printf("\n");
            if (pos > 0 && history_count < HISTORY_SIZE) {
                strncpy(history[history_count], buffer, MAX_INPUT_LENGTH);
                history_count++;
            }
            history_current = history_count;
            reset_terminal_mode(&original_termios);
            return buffer;
        }
        else if (c == KEY_BACKSPACE || c == KEY_DELETE) {
            if (pos > 0 && cursor_pos > 0) {
                memmove(&buffer[cursor_pos - 1], &buffer[cursor_pos], pos - cursor_pos);
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
                    if (history_current > 0) {
                        history_current--;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }                       
                        strncpy(buffer, history[history_current], MAX_INPUT_LENGTH);
                        pos = strlen(buffer);
                        cursor_pos = pos;  
                        printf("%s", buffer);
                        fflush(stdout);
                    }
                }
                else if (c == KEY_DOWN_ARROW) {
                    if (history_current < history_count - 1) {
                        history_current++;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        strncpy(buffer, history[history_current], MAX_INPUT_LENGTH);
                        pos = strlen(buffer);
                        cursor_pos = pos;  
                        printf("%s", buffer);
                        fflush(stdout);
                    }
                    else if (history_current == history_count - 1) {
                        history_current++;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        buffer[0] = NULL_TERMINATOR;
                        pos = 0;
                        cursor_pos = 0;  
                    }
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
        else if (c >= 32 && c < 127 && pos < MAX_INPUT_LENGTH - 1) {
            if (cursor_pos < pos) {
                memmove(&buffer[cursor_pos + 1], &buffer[cursor_pos], pos - cursor_pos);
            }
            buffer[cursor_pos] = c;
            pos++;
            cursor_pos++;
            if (cursor_pos == pos) {
                printf("%c", c);
            } else {
                for (int i = cursor_pos - 1; i < pos; i++) {
                    printf("%c", buffer[i]);
                }
                for (int i = 0; i < pos - cursor_pos; i++) {
                    printf("\b");
                }
            }
            fflush(stdout);
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
    
    if (!io_init()) {
        fprintf(stderr, "Initialization failed. Some features may not work properly.\n");
        fprintf(stderr, "Try running with root privileges for full functionality.\n");
    }
    if (geteuid() != 0) {
        fprintf(stderr, "Warning: Running without root privileges. Many operations will fail.\n");
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
    printf("Exiting IO Access Tool. Goodbye!\n");
    
    return 0;
}
