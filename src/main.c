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
    int c;
    struct termios original_termios;
    set_terminal_mode(&original_termios);
    printf("io> ");
    fflush(stdout);
    while (1) {
        c = getchar();
        if (c == '\n' || c == '\r') {
            buffer[pos] = '\0';
            printf("\n");
            if (pos > 0 && history_count < HISTORY_SIZE) {
                strncpy(history[history_count], buffer, MAX_INPUT_LENGTH);
                history_count++;
            }
            history_current = history_count;
            
            reset_terminal_mode(&original_termios);
            return buffer;
        }
        else if (c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
        }
        else if (c == 27) {
            c = getchar();
            if (c == 91) { 
                c = getchar();
                if (c == 65) {
                    if (history_current > 0) {
                        history_current--;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }                       
                        strncpy(buffer, history[history_current], MAX_INPUT_LENGTH);
                        pos = strlen(buffer);
                        printf("%s", buffer);
                        fflush(stdout);
                    }
                }
                else if (c == 66) {
                    if (history_current < history_count - 1) {
                        history_current++;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        strncpy(buffer, history[history_current], MAX_INPUT_LENGTH);
                        pos = strlen(buffer);
                        printf("%s", buffer);
                        fflush(stdout);
                    }
                    else if (history_current == history_count - 1) {
                        history_current++;
                        for (int i = 0; i < pos; i++) {
                            printf("\b \b");
                        }
                        buffer[0] = '\0';
                        pos = 0;
                    }
                }
            }
        }
        else if (c >= 32 && c < 127 && pos < MAX_INPUT_LENGTH - 1) {
            buffer[pos] = c;
            pos++;
            printf("%c", c);
            fflush(stdout);
        }
        else if (c == 3) {
            printf("\n");
            reset_terminal_mode(&original_termios);
            return NULL;
        }
        else if (c == 4) {
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
