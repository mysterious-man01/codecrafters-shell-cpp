#pragma once

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <vector>

#include "../shell.cpp"

// Disable shell mode
void disable_raw_mode();

// Enable shell mode
void enable_raw_mode();

// Main function for shell
void shell(std::string& buffer);