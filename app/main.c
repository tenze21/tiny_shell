#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "base.h"
#include "commands.h"
#include "utils.h"
#include "redirects.h"

/**
 * @dev parse string in argv from start to end into the `command_t` structure.
 * @param argv input string
 * @param start argv index to start parsing from
 * @param end argv index to end parsing at
 * @param cmd pointer to command_t type
 */
static int parse_cmd(char *argv[], int start, int end, command_t *cmd)
{
    cmd->argc = 0;
    cmd->out_append = false;
    cmd->err_append = false;
    cmd->redirect_out = NULL;
    cmd->redirect_err = NULL;

    for (int i = start; i < end; i++)
    {
        bool is_redirect_out = strcmp(argv[i], ">") == 0 || strcmp(argv[i], "1>") == 0;
        bool is_redirect_err = strcmp(argv[i], "2>") == 0;
        bool is_out_append = strcmp(argv[i], ">>") == 0 || strcmp(argv[i], "1>>") == 0;
        bool is_err_append = strcmp(argv[i], "2>>") == 0;
        bool is_piped = strcmp(argv[i], "|") == 0;

        if (is_redirect_out || is_redirect_err || is_err_append || is_out_append)
        {
            if (i + 1 >= end)
            { /*No output file*/
                fprintf(stderr, "tShell: Syntax error near unexpected token 'newline'\n");
                return -1;
            }

            if (is_redirect_op(argv[i + 1]))
            { /*consecutive redirects*/
                fprintf(stderr, "tShell: Syntax error near unexpected token '%s'\n", argv[i + 1]);
                return -1;
            }

            cmd->out_append = is_out_append;
            cmd->err_append = is_err_append;

            if (is_redirect_out || is_out_append)
                cmd->redirect_out = argv[i + 1];
            if (is_redirect_err || is_err_append)
                cmd->redirect_err = argv[i + 1];
            i++;
        }
        else
        {
            cmd->argv[cmd->argc++] = argv[i];
        }
    }
    cmd->argv[cmd->argc] = NULL;

    if (cmd->argc == 0)
    {
        fprintf(stderr, "tShell: error at or near unexpected token '|'\n");
        return -1;
    }
    return 0;
}

/**
 * @brief parse user input into pipes as a `pipeline_t` type
 * @param input cmd input
 * @param pipeline pointer to pipeline_t type
 */
static int parse_pipeline(const char *input, pipeline_t *pipeline)
{
    char *argv[MAXARGS];
    int argc = tokenize_cmd(input, argv);

    pipeline->ncmds = 0;
    int cmd_start = 0;
    for (int i = 0; i <= argc; ++i)
    {
        bool at_pipe = (i < argc) && strcmp(argv[i], "|") == 0;
        bool at_end = (i == argc);

        if (at_pipe || at_end)
        {
            if (pipeline->ncmds >= MAXPIPES)
            {
                fprintf(stderr, "tShell: Too many pipeline stages\n");
                return -1;
            }

            if (parse_cmd(argv, cmd_start, i, &pipeline->cmds[pipeline->ncmds]) < 0)
                return -1;
            pipeline->ncmds++;
            cmd_start = i + 1;
        }
    }
    return 0;
}

static void execute(command_t cmd)
{
    if (strcmp(cmd.argv[0], "echo") == 0)
    {
        echo(&cmd.argv[1]);
    }
    else if (strcmp(cmd.argv[0], "type") == 0)
    {
        type(cmd.argv[1]);
    }
    else if (strcmp(cmd.argv[0], "pwd") == 0)
    {
        pwd();
    }
    else
    {
        char *path_to_cmd = find_in_path(cmd.argv[0]);
        if (path_to_cmd != NULL)
        {
            exec_external(path_to_cmd, cmd.argv);
        }
        else
        {
            fprintf(stderr, "%s: command not found\n", cmd.argv[0]);
            _exit(EXIT_FAILURE);
        }
        free(path_to_cmd);
    }
}

/**
 * @brief Execute user input; single command or piped commands
 * @param pipeline pointer to the parsed pipeline
 */
static int exec_input(pipeline_t *pipeline)
{
    int nbrcmds = pipeline->ncmds;

    if (nbrcmds == 1)
    { /*if there is only one command directly execute it and return*/
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            return -1;
        }

        if (pid == 0)
        {
            apply_redirects(&pipeline->cmds[0]);
            execute(pipeline->cmds[0]);
            exit(EXIT_SUCCESS);
        }

        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }

    int npipes = nbrcmds - 1;
    int pipefds[2 * MAXPIPES]; // pipe file descriptors

    for (int i = 0; i < npipes; ++i)
    { // create pipes
        if (pipe(pipefds + i * 2) < 0)
        {
            perror("pipe error");
            return -1;
        }
    }

    pid_t pids[MAXPIPES]; // child process ids
    for (int i = 0; i < nbrcmds; ++i)
    { /*execute piped commands*/
        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork error");
            return -1;
        }

        if (pid == 0) // child process
        {
            if (i > 0) // stdin comes from the previous pipe's read end (unless first cmd)
                dup2(pipefds[(i - 1) * 2], STDIN_FILENO);

            if (i < nbrcmds - 1) // stdout goes to this pipe's write end (unless last cmd)
                dup2(pipefds[i * 2 + 1], STDOUT_FILENO);

            for (int pipeidx = 0; pipeidx < npipes * 2; ++pipeidx) // close all pipe file descriptor held by child
                close(pipefds[pipeidx]);

            apply_redirects(&pipeline->cmds[i]);
            execute(pipeline->cmds[i]);
            exit(EXIT_SUCCESS);
        }
        pids[i] = pid;
    }

    for (int pipeidx = 0; pipeidx < npipes * 2; ++pipeidx) // close all pipe file descriptor held by parent
        close(pipefds[pipeidx]);

    int status;
    for (int j = 0; j < nbrcmds; ++j)
        waitpid(pids[j], &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static void free_args(char *argv[], const size_t argc)
{
    for (size_t i = 0; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            free(argv[i]);
        }
    }
}

/*
 * READLINE UTILITIES
 */

/**
 * @dev Readline calls this repeatedly until it returns NULL, when user presses TAB.
 * @param text - The partial word the user has typed so far.
 * @param state - 0 on first call, increments each subsequent call. used by readline to determine if it's a fresh call.
 */
char *command_generator(const char *text, int state)
{
    static int builtin_idx;
    static int len;
    static char **dir_paths; // array of directory paths split from PATH
    static int path_count;   // number of directories in PATH
    static int dir_idx;      // points to the directory being scanned in `dir_paths`
    static DIR *cur_dir;     // open handle to the current directory.

    // state=0 implies it's a fresh completion call, reset all the state variables
    if (state == 0)
    {
        builtin_idx = 0;
        len = strlen(text);

        // cleanup any leftover directory handle from previous TAB
        if (cur_dir)
        {
            closedir(cur_dir);
            cur_dir = NULL;
        }

        if (dir_paths)
        {
            for (int i = 0; i < path_count; ++i)
                free(dir_paths[i]);
            free(dir_paths);
            dir_paths = NULL;
        }

        path_count = 0;
        dir_idx = 0;

        const char *path_env = getenv("PATH");
        if (path_env)
        {
            char *path_copy = strdup(path_env);

            // count directories in PATH
            char *tmp = strdup(path_env);
            char *tok = strtok(tmp, ":");
            while (tok)
            {
                path_count++;
                tok = strtok(NULL, ":");
            }
            free(tmp);

            dir_paths = malloc(path_count * sizeof(char *));
            if (!dir_paths)
                return NULL;
            int i = 0;
            tok = strtok(path_copy, ":");
            while (tok)
            {
                dir_paths[i++] = strdup(tok);
                tok = strtok(NULL, ":");
            }

            free(path_copy);
        }
    }

    // Walk through `builtin_cmds` and return the next match
    if (builtin_cmds[builtin_idx] != NULL)
    {
        const char *name;
        while ((name = builtin_cmds[builtin_idx++]) != NULL)
        {
            // Check if name starts with whatever the user typed
            if (strncmp(name, text, len) == 0)
                return strdup(name);
        }
    }

    // Look for matches in PATH
    while (dir_idx < path_count)
    {
        // open the next directory if we don't already have an open directory
        if (!cur_dir)
        {
            cur_dir = opendir(dir_paths[dir_idx]);

            if (!cur_dir)
            {
                dir_idx++;
                continue; // skip unreadable directories
            }
        }

        struct dirent *entry;
        while ((entry = readdir(cur_dir)) != NULL)
        {
            // Skip . and ..
            if (entry->d_name[0] == '.')
                continue;

            // check if the filename matches what the user typed
            if (strncmp(entry->d_name, text, len) != 0)
                continue;

            // build the full path to executable
            char full_path[MAXPATHLEN];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_paths[dir_idx], entry->d_name);

            struct stat st;
            if (stat(full_path, &st) != 0)
                continue;

            // only suggest regular files that are executable
            if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR))
                return strdup(entry->d_name);
        }

        closedir(cur_dir);
        cur_dir = NULL;
        dir_idx++;
    }

    return NULL;
}

/*
 * @dev readline calls this function first, this allows you to do context aware completion by passing different `command_generator` functions depending on the first word.
 */
char **command_completion(const char *text, int start, int end)
{
    // start = 0, means we are completing the first word, the command itself.
    // start > 0, means we are completing an argument, this can be handles differently depending on the command that was passed.
    if (start == 0)
    {
        // Prevent readline from also doing its own filename completion.
        rl_attempted_completion_over = 1;
        return rl_completion_matches(text, command_generator);
    }

    return NULL;
}

int main(void)
{
    rl_readline_name = "tinyShell";

    // Wire up completion function
    rl_attempted_completion_function = command_completion;
    char *line;
    while ((line = readline("$ ")) != NULL)
    {
        if (*line != '\0')
            add_history(line);

        pipeline_t pipeline = {0};

        if (parse_pipeline(line, &pipeline) < 0)
            continue;

        if (strcmp(pipeline.cmds[0].argv[0], "exit") == 0)
            break;
        else if (strcmp(pipeline.cmds[0].argv[0], "cd") == 0)
        {
            cd(pipeline.cmds[0].argv[1]);
            continue;
        }
        else if (strcmp(pipeline.cmds[0].argv[0], "history") == 0)
        {
            history(pipeline.cmds[0].argv[1]);
            continue;
        }

        exec_input(&pipeline);

        for (int i = 0; i < pipeline.ncmds; ++i)
            free_args(pipeline.cmds[i].argv, pipeline.cmds[i].argc);
    }
    return EXIT_SUCCESS;
}
