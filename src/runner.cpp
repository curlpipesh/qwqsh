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

#include "conf.hpp"

#define PIPE_READ 0
#define PIPE_WRITE 1

std::vector<char*> convert(std::vector<std::string> args);
char* cconvert(const std::string &s);
int shell_launch(pid_t& currentChild, std::vector<std::string> args);

// '>>' should be '> >' within a nested template argument list
int shell_launch_pipe(pid_t& currentChild, std::vector<std::vector<std::string> > pipedCommands) {
	PWARN("Multiple pipes are not yet implemented - you have been warned!");
    if(pipedCommands.size() == 0) {
        // No commands!
        return 1;
    } else if(pipedCommands.size() == 1) {
        // Only one command, so why bother piping? We just need to treat it like a normal command
        return shell_launch(currentChild, pipedCommands[0]);
    }
    // We have multiple commands that need piping, so...
    // This will be have to set up file descriptors and pipe i/o around and all that
    // TODO Actually handle more than one command at a time
    // File descriptors
    int fd[2];
    pid_t pidOne, pidTwo, wpidOne;
    int statusOne;

    pipe(fd);
    pidOne = fork();
    if(pidOne == 0) {
        // fork() worked
        currentChild = getpid();
        dup2(fd[PIPE_WRITE], PIPE_WRITE);
        close(fd[PIPE_READ]);
        close(fd[PIPE_WRITE]);
        std::vector<char*> converted = convert(pipedCommands[0]);
        if(execvp(converted[0], &converted[0]) == -1) {
            perror("shell: execvp() failed");
        }
        exit(EXIT_FAILURE);
    } else if(pidOne < 0) {
        perror("shell: fork() failed");
    } else {
        PDEBUG("Waiting on child...");
        do {
            wpidOne = waitpid(pidOne, &statusOne, WUNTRACED);
        } while(!WIFEXITED(statusOne) && !WIFSIGNALED(statusOne));
        PDEBUG("Done waiting!");
    }
    pidTwo = fork();
    if(pidTwo == 0) {
        currentChild = getpid();
        dup2(fd[PIPE_READ], PIPE_READ);
        close(fd[PIPE_READ]);
        close(fd[PIPE_WRITE]);
        std::vector<char*> converted = convert(pipedCommands[1]);
        if(execvp(converted[0], &converted[0]) == -1) {
            perror("shell: execvp() failed");
        }
        exit(EXIT_FAILURE);
    } else if(pidTwo < 0) {
        perror("shell: fork() failed");
    }
    close(fd[PIPE_READ]);
    close(fd[PIPE_WRITE]);
    wait(NULL);

    return 0;
}

int shell_launch(pid_t& currentChild, std::vector<std::string> args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if(pid == 0) {
        currentChild = getpid();
        // Child process
        std::vector<char*> converted = convert(args);
        PDEBUG("About to execvp()");
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

char* cconvert(const std::string &s) {
    // !!!
    // THIS IS PROBABLY REALLY DANGEROUS BECAUSE IT'S SUPPOSED TO BE const!!!
    // !!!
    return (char*) s.c_str();
}
