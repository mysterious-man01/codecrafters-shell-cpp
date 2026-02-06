#pragma once

#include <string>

extern bool stdout_redirect;
extern bool stderr_redirect;
extern bool append;

// Read a file content as string
std::string read_file(const std::string& path);

// Write file function
void write_file(std::string path, std::string msm, bool append);