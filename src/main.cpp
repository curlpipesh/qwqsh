#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Required for sigsetjmp and siglongjmp
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>

#include "colors.hpp"
#include "conf.hpp"
#include "runner.hpp"
#include "tokenizer.hpp"

void signalHandler(int signal);

pid_t currentChild;
sigjmp_buf ctrlcBuf;

int main(int argc, char** argv) {
    // Disable the default readline signal handlers
    // TODO Better tab completion
    rl_clear_signals();
    // Set up SIGINT handler
    if(signal(SIGINT, &signalHandler) == SIG_ERR) {
        std::cout << Color::FG_RED << "Failed to register SIGINT handler with kernel!" << Color::FG_DEFAULT << std::endl;
    }

    // Required for better ^C handling
    while(sigsetjmp(ctrlcBuf, 1) != 0);

    while(1) {
        // As defined in conf.hpp
        char* line = readline("> ");
        // Exiting on ^D and stuff
        if(!line || line == NULL) {
            break;
        }
        if(*line) {
            add_history(line);
            std::string lineString = line;
            if(lineString.find("|") != std::string::npos) {
                PDEBUG("Will be piping!");
                std::vector<std::string> pipeTokens = split(line, '|');
                std::vector<std::vector<std::string> > splitPipeTokens;
                PDEBUG("Pipe tokens:");
                for(int i = 0; i < pipeTokens.size(); i++) {
                    PDEBUG("\"" << pipeTokens[i] << "\"");
                }
                PDEBUG("Doing split");
                for(std::vector<std::string>::iterator it = pipeTokens.begin(); it != pipeTokens.end(); it++) {
                    splitPipeTokens.push_back(split(trim((*it).c_str())));
                }
#if DEBUG
                PDEBUG("Pipe(d) commands:");
                std::vector<std::vector<std::string> >::iterator iter = splitPipeTokens.begin();
                for(; iter != splitPipeTokens.end(); iter++) {
                    std::vector<std::string>::iterator it = (*iter).begin();
                    std::string debugStr = "Command: \"";
                    std::string toks = "";
                    for(; it != (*iter).end(); it++) {
                        toks.append(*it);
                        toks.append(" ");
                    }
                    debugStr.append(trim(toks.c_str()));
                    debugStr.append("\"");
                    PDEBUG(debugStr);
                }
#endif
                PDEBUG("Running piped commands!");
                shell_launch_pipe(currentChild, splitPipeTokens);
            } else {
                PDEBUG("No pipe found");
                std::vector<std::string> tokens = split(line);
                std::vector<std::string>::iterator it = tokens.begin();
#if DEBUG
                std::string debugStr = "Result: \"";
                std::string toks = "";
                for(; it != tokens.end(); it++) {
                    toks.append(*it);
                    toks.append(" ");
                }
#endif
                debugStr.append(trim(toks.c_str()));
                debugStr.append("\"");
                PDEBUG(debugStr);
                PDEBUG("Will attempt to run!");
                shell_launch(currentChild, tokens);
            }
        }
        free(line);
    }
    return 0;
}

// Basically just for catching ^C.
void signalHandler(int signal) {
    if(currentChild > 1) { // We are NOT going to try to kill init or anything else that important
        int status;
        pid_t result = waitpid(currentChild, &status, WNOHANG);
        if(result == 0) {
            kill(currentChild, SIGINT);
            PDEBUG(Color::FG_LIGHT_RED << "Killed" << Color::FG_DEFAULT << " child: "
                   << Color::FG_LIGHT_BLUE << currentChild << Color::FG_DEFAULT);
            std::cout << std::endl;
        } else if(result == -1) {
            // Some error
            std::cout << Color::FG_RED << "Unable to get status of child!?" << Color::FG_DEFAULT << std::endl;
        } else {
            std::cout << "Child " << Color::FG_BLUE << currentChild << Color::FG_DEFAULT
                      << " already exited with code " << Color::FG_RED << status << Color::FG_DEFAULT
                      << std::endl;
            currentChild = 0;
        }
    }
    std::cout << std::endl;
    // Required for better ^C handling
    siglongjmp(ctrlcBuf, 1);
}
