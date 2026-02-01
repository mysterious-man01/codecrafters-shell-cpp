#pragma once

#include <string>
#include <vector>

extern std::vector<std::string> history;

// Disable shell mode
void disable_raw_mode();

// Enable shell mode
void enable_raw_mode();

// Main function for shell
void shell(std::string& buffer);