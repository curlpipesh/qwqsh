#ifndef UUID_6F569A2D_D5A5_4397_A8E6_0EF78E089E89
#define UUID_6F569A2D_D5A5_4397_A8E6_0EF78E089E89

#include <unistd.h>

int shell_launch(std::vector<std::string> args);
int shell_launch_pipe(pid_t& currentChild, std::vector<std::vector<std::string> > pipedCommands);

#endif //UUID_6F569A2D_D5A5_4397_A8E6_0EF78E089E89
