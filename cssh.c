#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <features.h> 
#include <libgen.h>

#define MAX_ARGS 32

char **get_next_command(size_t *num_args)
{
    printf("cssh$ ");

    char *line = NULL;
    size_t len = 0;
    getline(&line, &len, stdin);
    if (ferror(stdin))
    {
        perror("getline");
        exit(1);
    }
    if (feof(stdin))
    {
        return NULL;
    }

    char **words = (char **)malloc(MAX_ARGS * sizeof(char *));
    int i = 0;

    char *parse = line;
    while (parse != NULL)
    {
        char *word = strsep(&parse, " \t\r\f\n");
        if (strlen(word) != 0)
        {
            words[i++] = strdup(word);
        }
    }
    *num_args = i;
    for (; i < MAX_ARGS; ++i)
    {
        words[i] = NULL;
    }

    free(line);

    return words;
}

void free_command(char **words)
{
    for (int i = 0; i < MAX_ARGS; ++i)
    {
        if (words[i] == NULL)
        {
            break;
        }
        free(words[i]);
    }
    free(words);
}

int main()
{
    size_t num_args;

    char **command_line_words = get_next_command(&num_args);
    while (command_line_words != NULL)
    {
        if (num_args == 0)
        {
            free_command(command_line_words);
            command_line_words = get_next_command(&num_args);
            continue;
        }

        if (strcmp(command_line_words[0], "exit") == 0)
        {
            free_command(command_line_words);
            break;
        }

        int input_fd = -1;
        int output_fd = -1;
	int append_mode = 0;
        int syntax_error = 0;

        for (int i = 1; i < num_args; i++)
        {
            if (strcmp(command_line_words[i], "<") == 0)
            {
                if (i + 1 < num_args)
                {
                    input_fd = open(command_line_words[i + 1], O_RDONLY);
                    if (input_fd < 0)
                    {
                        perror("open");
                        syntax_error = 1;
                        break;
                    }
                    free(command_line_words[i]);
                    free(command_line_words[i + 1]);
                    for (int j = i + 2; j < num_args; j++)
                    {
                        command_line_words[j - 2] = command_line_words[j];
                    }
                    num_args -= 2;
                    i -= 2;
                }
                else
                {
                    printf("Syntax error: no input file specified\n");
                    syntax_error = 1;
                    break;
                }
            }
            else if (strcmp(command_line_words[i], ">") == 0)
            {
                if (i + 1 < num_args)
                {
                    output_fd = open(command_line_words[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (output_fd < 0)
                    {
                        perror("open");
                        syntax_error = 1;
                        break;
                    }
                    free(command_line_words[i]);
                    free(command_line_words[i + 1]);
                    for (int j = i + 2; j < num_args; j++)
                    {
                                                command_line_words[j - 2] = command_line_words[j];
                    }
                    num_args -= 2;
                    i -= 2;
                }
                else
                {
                    printf("Syntax error: no output file specified\n");
                    syntax_error = 1;
                    break;
                }
            }
            else if (strcmp(command_line_words[i], ">>") == 0)
            {
                if (i + 1 < num_args)
                {
                    output_fd = open(command_line_words[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (output_fd < 0)
                    {
                        perror("open");
                        syntax_error = 1;
                        break;
                    }
                    free(command_line_words[i]);
                    free(command_line_words[i + 1]);
                    for (int j = i + 2; j < num_args; j++)
                    {
                        command_line_words[j - 2] = command_line_words[j];
                    }
                    num_args -= 2;
                    i -= 2;
                    append_mode = 1;
                }
                else
                {
                    printf("Syntax error: no output file specified\n");
                    syntax_error = 1;
                    break;
                }
            }
        }

        if (syntax_error)
        {
            free_command(command_line_words);
            command_line_words = get_next_command(&num_args);
            continue;
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }
        else if (pid == 0)
        {
            if (input_fd >= 0)
            {
                if (dup2(input_fd, STDIN_FILENO) == -1)
                {
                    perror("dup2");
                    exit(1);
                }
                close(input_fd);
            }

            if (output_fd >= 0)
            {
                if (dup2(output_fd, STDOUT_FILENO) == -1)
                {
                    perror("dup2");
                    exit(1);
                }
                close(output_fd);
            }

            execvp(command_line_words[0], command_line_words);
            perror("execvp");
            exit(1);
        }
        else
        {
            int status;
            waitpid(pid, &status, 0);
        }

        free_command(command_line_words);
        command_line_words = get_next_command(&num_args);
    }

     free_command(command_line_words);

    return 0;
}
