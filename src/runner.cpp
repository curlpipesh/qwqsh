/*
 * TODO Doesn't verify syntax, command validity, etc.
 *      For validity, could just read $PATH with getenv(3), but probably can't just cache
 *      the results from that because it could change after running a command.
 *      Syntax verification will require writing a (minimal?) parser, mainly to check for
 *      the things like "||" instead of "|" and etc.
 */

// For std::transform
#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

// For perror
#include <stdio.h>
// For execvp and piping
#include <unistd.h>
// For waitpid, WIFEXITED, WIFSIGNALED, ...
#include <sys/wait.h>

#include "colors.hpp"
#include "conf.hpp"
#include "tokenizer.hpp"

#define PIPE_READ 0
#define PIPE_WRITE 1

std::vector<char*> convert(std::vector<std::string> args);
char* cconvert(const std::string &s);
int shell_launch(std::vector<std::string> args);
long open_max(void);

// TODO currentChild isn't actually used
// std::vector<std::vector<std::string> > was easier than trying to make a char***
int shell_launch_pipe(pid_t& currentChild, std::vector<std::vector<std::string> > pipedCommands) {
    PWARN("Multiple pipes are not yet implemented - you have been warned!");
    if(pipedCommands.size() == 0) {
        // No commands!
        PWARN("No commands given to shell_launch_pipe()?!");
        return 1;
    } else if(pipedCommands.size() == 1) {
        // Only one command, so why bother piping? We just need to treat it like a normal command
        return shell_launch(pipedCommands[0]);
    }

    // We have multiple commands that need piping, so...
    // This will be have to set up file descriptors and pipe i/o around and all that

    // The number of pipes needed is always (commandCount - 1).
    int commandCount = pipedCommands.size();
    int totalFdCount = commandCount - 1;

    int fds[totalFdCount][2];
    pid_t pids[commandCount];
    pid_t wpids[commandCount];
    int statuses[commandCount];

    for(int i = 0; i < commandCount; i++) {
        PDEBUG("i=" << i);
        int pstat = pipe(fds[i]);
        if(pstat == 0) {
            PDEBUG("pipe()'d");
            // pipe() worked
            pids[i] = fork();
            if(pids[i] == 0) {
                PDEBUG(Color::FG_MAGENTA << "{" << Color::FG_DEFAULT);
                PDEBUG("fork()'d");
                // fork() worked
                if(i == 0) {
                    PDEBUG("** Making initial WRITE pipe **");
                    // The initial process only needs to hook up to the write end
                    dup2(fds[i][PIPE_WRITE], PIPE_WRITE);
                    close(fds[i][PIPE_WRITE]);
                    close(fds[i][PIPE_READ]);
                    PDEBUG("## WRITE: i=" << i << " ##");
                } else if(i == totalFdCount) {
                    PDEBUG("** Making final READ pipe **");
                    // The final process only needs to hook up to the read end
                    dup2(fds[i - 1][PIPE_READ], PIPE_READ);
                    close(fds[i - 1][PIPE_READ]);
                    close(fds[i - 1][PIPE_WRITE]);
                    PDEBUG("## READ: (i-1)=" << (i - 1) << " ##");
                } else {
                    PDEBUG("** Making in-between pipe **");
                    // Everything in-between needs to hook up to both ends
                    dup2(fds[i - 1][PIPE_READ], PIPE_READ);
                    close(fds[i - 1][PIPE_READ]);
                    close(fds[i - 1][PIPE_WRITE]);
                    PDEBUG("## READ: (i-1)=" << (i - 1) << " ##");
                    dup2(fds[i][PIPE_WRITE], PIPE_WRITE);
                    close(fds[i][PIPE_WRITE]);
                    close(fds[i][PIPE_READ]);
                    PDEBUG("## WRITE: i=" << i << " ##");
                }
                // This was easier than making a char**
                std::vector<char*> converted = convert(pipedCommands[i]);
                PDEBUG("*# Preparing to execvp(" << converted[0] << ")! #*");
                PDEBUG(Color::FG_MAGENTA << "}" << Color::FG_DEFAULT);
                if(i == totalFdCount) {
                    PDEBUG("Output:");
                }
                if(execvp(converted[0], &converted[0]) == -1) {
                    perror("shell: execvp() failed");
                }
                exit(EXIT_FAILURE);
            } else if(pids[i] < 0) {
                perror("shell: fork() failed");
                break;
            } else if(i < commandCount - 1) {
                // waitpid
                PDEBUG("Waiting on child...");
                pid_t watch = i == 0 ? pids[i] : pids[i - 1];
                do {
                    wpids[i] = waitpid(watch, &(statuses[i]), WUNTRACED);
                } while(!WIFEXITED(statuses[i]) && !WIFSIGNALED(statuses[i]));
                PDEBUG("Done waiting! (" << pipedCommands[i][0] << ")");
#if DEBUG
            } else {
                std::string s = "";
                for(int j = 0; j < pipedCommands[i].size(); j++) {
                    s = s.append(pipedCommands[i][j]).append(" ");
                }
                PDEBUG("Relevant information:");
                PDEBUG("i=" << i);
                PDEBUG("pids[i]=" << pids[i]);
                PDEBUG("Command string=" << trim(s.c_str()));
#endif
            }
        } else {
            perror("shell: pipe() failed");
        }
        close(fds[i][PIPE_WRITE]);
        close(fds[i][PIPE_READ]);
        wait(NULL);
    }
    wait(NULL);
    PDEBUG("Done!");
    return 0;
}

int shell_launch(std::vector<std::string> args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0) {
        // Child process
        std::vector<char*> converted = convert(args);
        PDEBUG("About to execvp()");
        PDEBUG("Output: " << std::endl);
        if(execvp(args[0].c_str(), &converted[0]) == -1) {
            // execvp failed
            perror("shell: execvp() failed");
        }
        exit(EXIT_FAILURE);
    } else if(pid < 0) {
        // Error forking
        perror("shell: fork() failed");
    } else {
        // Parent process
        PDEBUG("Waiting on child...");
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
        PDEBUG("Done waiting!");
    }
    PDEBUG("Done!");
    return 1;
}

std::vector<char*> convert(std::vector<std::string> args) {
    // Holds the char*s from the std::strings
    std::vector<char*> converted;
    // Does the transform, from beginning to end, inserts into converted, using convert as the convert function
    std::transform(args.begin(), args.end(), std::back_inserter(converted), cconvert);
    // Last argument to execvp() MUST be NULL
    converted.push_back(NULL);
    return converted;
}

// This is honestly probably just a lot stupid...
char* cconvert(const std::string &s) {
    return (char*) s.c_str();
}
