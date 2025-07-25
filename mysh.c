#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h>

#define MAX_LINE_LENGTH 1024
#define MAX_WILDCARD_COUNT 128

int interactive_mode = 0;

typedef struct {
    char **tokens;
    int wildCardLocations[MAX_WILDCARD_COUNT];
    int num_tokens;
    int pipeLocation; 
    int wildcardCount;
} Token;

typedef struct {
    char** args;
    int num_args;
    char* input_redirection;
    char* output_redirection;
    int has_input_redirection;
    int has_output_redirection;
} Command;


Token* tokenize(char* line);
Command* process_token(Token *cmd);
int wildcard_match(const char* pattern, const char* str);
void handle_wildcard(Command* command, const char* pattern);
int process_pipe(Token* cmd_info);
int process_command(Command *cmd);
void free_token(Token *token);
void free_command(Command *cmd);
void exitShell(Command* cmd);

int main(int argc, char *argv[]) {
    int input_fd = STDIN_FILENO;
    interactive_mode = isatty(STDIN_FILENO);  
    if (argc > 1) {
        // Open the file for reading if an argument is provided
        input_fd = open(argv[1], O_RDONLY);
        if (!input_fd) {
            perror("Error opening file");
            return 1;
        }
        // Not interactive if reading from a file
        interactive_mode = 0; 
    }

    if (interactive_mode) {
        printf("Welcome to my shell!\n");
    }

    char line[MAX_LINE_LENGTH];
    int chars_read;
    int last_exit_status = 0;

    while (1) {
        if (interactive_mode) {
            write(STDOUT_FILENO, "mysh> ", 6);
        }
        chars_read = 0;
        while (chars_read < MAX_LINE_LENGTH - 1) {
            int result = read(input_fd, &line[chars_read], 1); 
            if (result < 0) {
                perror("Error reading input");
                close(input_fd);
                exit(1);
            } else if (result == 0) { 
                // End of file or no more input
                break;
            } else if (line[chars_read] == '\n') {
                break;
            } else {
                chars_read += result;
            }
        }

        // Break the loop if no more input unless interactive (which needs exit)
        if (chars_read == 0) { 
            if(interactive_mode){
                continue;
            }else{
                break;
            }
        }
        line[chars_read] = '\0'; 

        Token* cmd_info = tokenize(line);

        // Handle conditionals
        if (strcmp(cmd_info->tokens[0], "else") == 0 && last_exit_status == 0) {
                continue;  
            }
        if (strcmp(cmd_info->tokens[0], "then") == 0 && last_exit_status != 0) {
                continue;  
            }
       
        // Process and execute command 
        if (cmd_info-> pipeLocation != -1) {
            last_exit_status = process_pipe(cmd_info);
        }
        else {
            Command* command = process_token(cmd_info);
            free_token(cmd_info);
            last_exit_status = process_command(command);
            free_command(command);
        }
    }
    if (interactive_mode) {
        printf("mysh: exiting\n");
    }
    if (input_fd != STDIN_FILENO) {
        close(input_fd);
    }
    return 0;
}


Token* tokenize(char* line) {
    Token *cmd_info = malloc(sizeof(Token));
    if (!cmd_info) {
        perror("Error allocating CommandInfo");
        exit(EXIT_FAILURE);
    }
    cmd_info->tokens = malloc(MAX_LINE_LENGTH * sizeof(char*));
    if (!cmd_info->tokens) {
        free(cmd_info);
        perror("Error allocating tokens array");
        exit(EXIT_FAILURE);
    }
    cmd_info->num_tokens = 0;
    cmd_info->pipeLocation = -1;
    cmd_info->wildcardCount = 0;

    for (size_t i = 0; i < strlen(line);) {
        if (line[i] == ' ' || line[i] == '\n') {
            // Skip whitespace
            i++;  
        } else if (line[i] == '|' || line[i] == '>' || line[i] == '<') {
            // Store special characters as separate tokens
            if (line[i] == '|'){
                // remember locartion of pipe
                cmd_info->pipeLocation = cmd_info->num_tokens;
            }
            cmd_info->tokens[cmd_info->num_tokens++] = strndup(&line[i], 1); 
            i++;
        } else { 
            // store sequence of nonwhite space characters as one token given that it does not include any special characters
            int start= i;
            while (line[i] != ' ' && line[i] != '\n' && line[i] != '\0' 
                   && line[i] != '|' && line[i] != '>' && line[i] != '<') { 
                if (line[i] == '*'){
                    // remember locartion of wildcard
                    cmd_info->wildCardLocations[cmd_info->wildcardCount++] = cmd_info->num_tokens;
                }
                i++;
            } 
            cmd_info->tokens[cmd_info->num_tokens++] = strndup(&line[start], i - start); 
        }
    }
    // printf("Tokens:\n"); 
    // for (int i = 0; i < cmd_info->num_tokens; i++) {
    //     printf("  Token %d: %s\n", i, cmd_info->tokens[i]);
    // }

    return cmd_info;
}


Command* process_token(Token *cmd) {
    Command *command = malloc(sizeof(Command));
    if (!command) {
        perror("malloc failed for Command");
        exit(EXIT_FAILURE);
    }
    command->args = malloc(MAX_LINE_LENGTH * sizeof(char*));
    if (!command->args) {
        perror("malloc failed for Command args");
        free(command);
        exit(EXIT_FAILURE);
    }
    command->num_args = 0;
    command->input_redirection = NULL;
    command->output_redirection = NULL;
    command->has_input_redirection = 0;
    command->has_output_redirection = 0;

    int wildcardCount = 0;

    for (int i = 0; i < cmd->num_tokens; i++) {
        if (i == 0 && (strcmp(cmd->tokens[i], "then") == 0 || strcmp(cmd->tokens[i], "else") == 0)) {
            // Skip 'then' or 'else' as the first token
            continue; 
        } else if (strcmp(cmd->tokens[i], "<") == 0 && i + 1 < cmd->num_tokens) {
            // check if token is input redirection
            command->input_redirection = strdup(cmd->tokens[++i]);
            command->has_input_redirection = 1;
        } else if (strcmp(cmd->tokens[i], ">") == 0 && i + 1 < cmd->num_tokens) {
            // check if token is output redirection
            command->output_redirection = strdup(cmd->tokens[++i]);
            command->has_output_redirection = 1;
        } else if (wildcardCount < cmd->wildcardCount && i == cmd->wildCardLocations[wildcardCount]) {
            // handle any wildcards
            handle_wildcard(command, cmd->tokens[i]);
            wildcardCount++;
        } else {
            command->args[command->num_args] = strdup(cmd->tokens[i]);
            if (command->args[command->num_args]) { 
                command->num_args++;
            }
        }
    }

    return command;
}


int wildcard_match(const char* pattern, const char* str) {
    if (*pattern == '\0'){
        return *str == '\0';
    }
    if (*pattern == '*') {
        return wildcard_match(pattern + 1, str) || (*str && wildcard_match(pattern, str + 1));
    }
    return (*pattern == *str) && wildcard_match(pattern + 1, str + 1);
}


void handle_wildcard(Command* command, const char* pattern) {
    char* last_slash = strrchr(pattern, '/');
    char* dir_path;
    char* file_pattern;

    if (last_slash) {
        *last_slash = '\0';
        dir_path = strdup(pattern);
        file_pattern = strdup(last_slash + 1);
    } else {
        dir_path = strdup(".");
        file_pattern = strdup(pattern);
    }

    DIR* dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir failed");
        free(dir_path);
        free(file_pattern);
        return;
    }

    struct dirent* entry;
    int match_found = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (file_pattern[0] != '.' && entry->d_name[0] == '.') continue;
        if (wildcard_match(file_pattern, entry->d_name)) {
            match_found = 1;
            char* matched_name;
            if (strcmp(dir_path, ".") == 0) {
                matched_name = strdup(entry->d_name);
            } else {
                matched_name = malloc(strlen(dir_path) + strlen(entry->d_name) + 2);
                snprintf(matched_name, strlen(dir_path) + strlen(entry->d_name) + 2, "%s/%s", dir_path, entry->d_name);
            }
            command->args[command->num_args++] = matched_name;
        }
    }
    if (!match_found) {
        command->args[command->num_args++] = strdup(pattern);
    }
    closedir(dir);
    free(dir_path);
    free(file_pattern);
}




int process_pipe(Token* cmd_info) {
    int pipefd[2];
    int status = 0;
    pid_t pid2 = 0;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }
    if (cmd_info->pipeLocation != -1) {
        // Extract left part of the pipe command
        Token left_cmd_info;
        left_cmd_info.tokens = cmd_info->tokens;
        left_cmd_info.num_tokens = cmd_info->pipeLocation;
        left_cmd_info.wildcardCount = 0;
        for (int i = 0; i < cmd_info->wildcardCount; i++) {
            if (cmd_info->wildCardLocations[i] < cmd_info->pipeLocation) {
                left_cmd_info.wildCardLocations[left_cmd_info.wildcardCount++] = cmd_info->wildCardLocations[i];
            }
        }
        Command* left_command = process_token(&left_cmd_info);

        // Extract right part of the pipe command
        Token right_cmd_info;
        right_cmd_info.tokens = cmd_info->tokens + cmd_info->pipeLocation + 1;
        right_cmd_info.num_tokens = cmd_info->num_tokens - cmd_info->pipeLocation - 1;
        right_cmd_info.wildcardCount = 0;
        for (int i = 0; i < cmd_info->wildcardCount; i++) {
            if (cmd_info->wildCardLocations[i] > cmd_info->pipeLocation) {
                right_cmd_info.wildCardLocations[right_cmd_info.wildcardCount++] = cmd_info->wildCardLocations[i] - cmd_info->pipeLocation - 1;
            }
        }
        Command* right_command = process_token(&right_cmd_info);

        

        // Exit immediately if left command is 'exit'
        if (strcmp(left_command->args[0], "exit") == 0) {
            free_command(right_command);
            status = 1;
            exitShell(left_command);
        }


        // Child process for left side of pipe
        pid_t pid1 = fork();
        if (pid1 == 0) { 
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            process_command(left_command);
            exit(EXIT_SUCCESS);
        }

        int status = 0;
        if (strcmp(right_command->args[0], "exit") == 0) {
            // Exit after left command completes if right command is 'exit'
            waitpid(pid1, NULL, 0);
            free_command(left_command);
            status = 1;
            exitShell(right_command);
        }
        // Child process for right side of pipe
        else{
            pid_t pid2 = fork();
            if (pid2 == 0) { 
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                status = process_command(right_command);
                exit(EXIT_SUCCESS);
            }
        }

        // Parent process
        close(pipefd[0]);
        close(pipefd[1]);

        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);

        free_command(left_command);
        free_command(right_command);
        free_token(cmd_info);

        return status;
    } 
    status = 1;
    return status;
}



int process_command(Command *cmd) {
    int inputFd = -1, outputFd = -1;
    int status = 0;
    int original_stdout = -1;
    int original_stdin = -1;

    // Input Redirection
    if (cmd->has_input_redirection) {
        original_stdin = dup(STDIN_FILENO);  
        inputFd = open(cmd->input_redirection, O_RDONLY);
        if (inputFd == -1) {
            perror("open input file");
            return 1;
        }
        dup2(inputFd, STDIN_FILENO);
        close(inputFd);
    }

    // Output Redirection
    if (cmd->has_output_redirection) {
        original_stdout = dup(STDOUT_FILENO); 
        outputFd = open(cmd->output_redirection, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (outputFd == -1) {
            perror("open output file");
            if (original_stdout != -1) close(original_stdout);
            return 1;
        }
        dup2(outputFd, STDOUT_FILENO);
        close(outputFd);
    }

    // Handle built-in commands
    if (strcmp(cmd->args[0], "cd") == 0) {
        if (cmd->num_args < 2) {
            fprintf(stderr, "cd: missing argument\n");
            status = 1;
        } else {
            if (chdir(cmd->args[1]) != 0) {
                perror("cd");
                status = 1;
            }
        }
    } else if (strcmp(cmd->args[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("pwd");
            status = 1;
        } else {
            printf("%s\n", cwd);
        }
    } else if (strcmp(cmd->args[0], "which") == 0) {
        if (cmd->num_args < 2) {
            fprintf(stderr, "which: missing argument\n");
            status = 1;
        } else {
            const char *directories[] = { "/usr/local/bin", "/usr/bin", "/bin" };
            int found = 0;
            for (int i = 0; i < 3 && !found; i++) {
                char full_path[PATH_MAX];
                snprintf(full_path, PATH_MAX, "%s/%s", directories[i], cmd->args[1]);
                if (access(full_path, X_OK) == 0) {
                    printf("%s\n", full_path);
                    found = 1;
                }
            }
            if (!found) {
                fprintf(stderr, "%s not found\n", cmd->args[1]);
                status = 1;
            }
        }
    } else if (strcmp(cmd->args[0], "exit") == 0) {
        exitShell(cmd);
    } else {
        // Handle external commands
        char *executable_path = cmd->args[0][0] == '/' ? strdup(cmd->args[0] + 1) : NULL;
        if (!executable_path) {
            const char *directories[] = { "/usr/local/bin", "/usr/bin", "/bin" };
            for (int i = 0; i < 3 && executable_path == NULL; i++) {
                char full_path[PATH_MAX];
                snprintf(full_path, PATH_MAX, "%s/%s", directories[i], cmd->args[0]);
                if (access(full_path, X_OK) == 0) {
                    executable_path = strdup(full_path);
                }
            }
        }
        if (executable_path) {
            cmd->args[cmd->num_args] = NULL;
            pid_t pid = fork();
            if (pid == 0) { 
                execv(executable_path, cmd->args);
                perror("execv");
                status =  1;
            } else if (pid > 0) { 
                int wait_status;
                waitpid(pid, &wait_status, 0);
                if (WIFEXITED(wait_status)) {
                    status = WEXITSTATUS(wait_status);
                }
            } else {
                perror("fork");
                status = 1;
            }
            free(executable_path);
        } else {
            fprintf(stderr, "Command not found: %s\n", cmd->args[0]);
            status = 1;
        }
    }

    // Restore original stdin and stdout
    if (original_stdin != -1) {
        dup2(original_stdin, STDIN_FILENO);
        close(original_stdin);
    }

    if (original_stdout != -1) {
        dup2(original_stdout, STDOUT_FILENO);
        close(original_stdout);
    }

    return status;
}



void free_token(Token *token) {
    if (token == NULL) {
        return; 
    }
    if (token->tokens != NULL) {
        for (int i = 0; i < token->num_tokens; i++) {
            if (token->tokens[i] != NULL) {
                free(token->tokens[i]);
                token->tokens[i] = NULL; 
            }
        }
        free(token->tokens);
        token->tokens = NULL;
    }
    free(token);
}


void free_command(Command *cmd) {
    if (cmd == NULL) {
        return;
    }
    if (cmd->args != NULL) {
        for (int i = 0; i < cmd->num_args; i++) {
            if (cmd->args[i] != NULL) {
                free(cmd->args[i]);
            }
        }
        free(cmd->args);
    }
    if (cmd->has_input_redirection && cmd->input_redirection != NULL) {
        free(cmd->input_redirection);
    }
    if (cmd->has_output_redirection && cmd->output_redirection != NULL) {
        free(cmd->output_redirection);
    }
    free(cmd);
}

void exitShell(Command* cmd){
// Print any arguments passed to exit, separated by spaces
    for (int i = 1; i < cmd->num_args; i++) {
        write(STDOUT_FILENO, cmd->args[i], strlen(cmd->args[i]));
        if (i < cmd->num_args - 1) {
            write(STDOUT_FILENO, " ", 1);
        }
    }
    if (cmd->num_args > 1) write(STDOUT_FILENO, "\n", 1);
    free_command(cmd);
    if (interactive_mode) {
        write(STDOUT_FILENO, "mysh: exiting\n", 14);
    }
    exit(0);
}
