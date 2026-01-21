#pragma once

#include <string>

// Disable shell mode
void disable_raw_mode();

// Enable shell mode
void enable_raw_mode();

// Main function for shell
void shell(std::string& buffer);