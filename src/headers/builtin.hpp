#pragma once

#include <string>
#include <vector>

extern const std::string PATH;

// echo command
std::string echo(std::vector<std::string> str);

// cd command to change shell workig directory
void cd(std::vector<std::string> path);

// type command
std::string type(std::vector<std::string> str);

// External command executor
int OSexec(std::vector<std::string> cmd);

// Buitin commands
void builtin_cmds(const std::vector<std::string>& cmd);