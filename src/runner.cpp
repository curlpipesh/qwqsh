/*
 * TODO Doesn't verify syntax, command validity, etc.
 *      For validity, could just read $PATH with getenv(3), but probably can't just cache
 *      the results from that because it could change after running a command.
 *      Syntax verification will require writing a (minimal?) parser, mainly to check for
 *      the things like "||" instead of "|" and etc. 
 *
 * TODO Use mkfifo(3)? UNIX programming book seems to recommend doing so (see page 514-8)
 *      "FIFOs are used by shell commands to pass data from one shell pipeline to another..."
 *      We can't just use popen(3) and pclose(3) because those just invoke /bin/sh, and we
 *      don't want to cheat by doing that. Writing our own popen(3) and pclose(3) would be
 *      possible (because we're already doing essentially that), but would require some 
 *      extra work (See book, pages 503-10). 
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
    // Could probably just store all the file descriptors in an array (like an int[?][2])
    // and just keep them in the parent and just adjust them as we fork() new 
    // processes. 
    
    // File descriptors
    int fd[2];
    int pipeStatus;
    pid_t pidOne, pidTwo, wpidOne;
    int statusOne;

    pipeStatus = pipe(fd); // TODO Error-check the pipe() call
    if(pipeStatus == 0) {
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
	    PDEBUG("Final output: " << std::endl);
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
        // If we waitpid() here, then it hangs until we ^C it.
        close(fd[PIPE_READ]);
        close(fd[PIPE_WRITE]);
        wait(NULL);
    } else {
        perror("shell: pipe() failed");
    }

    PDEBUG("Done!");
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

char* cconvert(const std::string &s) {
    // !!!
    // THIS IS PROBABLY REALLY DANGEROUS BECAUSE IT'S SUPPOSED TO BE const!!!
    // !!!
    return (char*) s.c_str();
}
