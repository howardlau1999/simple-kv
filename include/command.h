#ifndef _COMMAND_H_
#define _COMMAND_H_
#include <string>
#include <vector>

enum Operation : char {
    GET, // C->S, GET the corresponding value of KEY, followed by a 8-byte KEY
    PUT, // C->S, PUT a KV pair, followed by a 8-byte KEY and 256-byte VALUE
    DELETE, // C->S, DELETE a key, followed by a 8-byte KEY
    SCAN, // C->S, SCAN all kv pairs between two KEYs, followed by 2 8-byte KEYs indicating MIN_KEY and MAX_KEY
    KV, // S->C, returns a KV pair, followed by a 8-byte KEY and 256-byte VALUE
    VALUE, // S->C, simply a VALUE, followed by a 256-byte VALUE
    NO_MORE, // S->C, means there will be no more KV pairs, no data will follow
    NOT_FOUND,
    OK, // S->C, Operation went well, no data will follow
    ERROR, // S->C, Something went wrong, a 256-byte string of error message
};

struct Command {
public:
    Operation op;
};

#endif