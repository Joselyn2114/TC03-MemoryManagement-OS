//

#ifndef PARSER_H
#define PARSER_H

#include "command.h"
#include "strategy.h"

int parse_strategy(const char* arg, StrategyType* strategy);

int parse_command(char* arg, Command* command);

int parse_command_type(const char* arg, CommandType* type);

#endif  // PARSER_H
