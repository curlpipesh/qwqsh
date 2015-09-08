#ifndef UUID_608455EC_8589_47EB_97D5_3B735E08394C
#define UUID_608455EC_8589_47EB_97D5_3B735E08394C

#include <string>

#define TOKENIZER_NO_ERROR 0
#define TOKENIZER_ERROR 1

std::vector<std::string> split(const char* str, char c = ' ');
struct TokenizerError* verify(char* line);
std::string stripExtraWhitespace(char* line);
const char* trim(const char* str);

struct TokenizerError {
    int status;
    char* message;
    char* line;
    char* linePointer;
};

#endif //UUID_608455EC_8589_47EB_97D5_3B735E08394C
