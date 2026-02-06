#pragma once

#include <string>
#include <vector>

extern std::vector<std::string> history;

// Initialize HISTFILE internal variable
void inithist();

// Get hitory from HISTFILE and saves on memory
void gethist();

// Save history on in memory to a file in HISTFILE
void savehist();

// History builtin command
std::string history_command(const std::vector<std::string>& n);