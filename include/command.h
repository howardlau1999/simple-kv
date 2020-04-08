#ifndef _COMMAND_H_
#define _COMMAND_H_
#include <string>
#include <vector>

enum Operation {
    GET,
    PUT,
    DELETE,
    SCAN,
    KV,
    NO_MORE,
    PING,
    PONG,
    OK,
    ERROR,
};

class Command {
    Operation op;
    std::vector<std::vector<char>> args;
};

#endif