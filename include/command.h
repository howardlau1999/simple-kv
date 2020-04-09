#ifndef _COMMAND_H_
#define _COMMAND_H_
#include <string>
#include <vector>

enum Operation : char {
    GET,
    PUT,
    DELETE,
    SCAN,
    KV,
    VALUE,
    NO_MORE,
    PING,
    PONG,
    OK,
    ERROR,
};

class Command {
public:
    Operation op;
    std::vector<std::vector<char>> args;
};

#endif