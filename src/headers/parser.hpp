#pragma once

#include <string>
#include <vector>

// States of the parser function
enum class State {
  Normal,
  SingleQuote,
  DoubleQuote
};

// Identify and split piped command
void build_cmdline(const std::string& cmd_tokens,
                  std::vector<std::string>& cmdpipe);

// Parser function to identify commands in a string
std::vector<std::string> parser(const std::string& str);