#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "io_access.h"
#include "command_processor.h"

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
        line = readline("io> ");
        
        if (!line) {
            break; 
        }
        should_exit = process_command(line);
        if (*line != '\0') {
            add_history(line);
        }
        
        free(line);
    }
    
    io_cleanup();
    printf("Exiting IO Access Tool. Goodbye!\n");
    
    return 0;
}
