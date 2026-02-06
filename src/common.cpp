#include "common.hpp"

#include <iostream>
#include <fcntl.h>
#include <unistd.h>

bool stdout_redirect = false;
bool stderr_redirect = false;
bool append = false;

// Read a file content as string
std::string read_file(const std::string& path){
  int file = open(path.c_str(), O_RDONLY);
  if(file < 0)
    return "";

  std::string txt;
  char buf[4096];
  ssize_t n;

  while((n = read(file, buf, sizeof(buf))) > 0){
    txt.append(buf, n);
  }

  close(file);

  return txt;
}

// Write file function
void write_file(std::string path, std::string msm, bool append){
  int fd;
  if(append){
    fd = open(path.c_str(),
              O_WRONLY | O_CREAT | O_APPEND,
              0644);
  }
  else{
    fd = open(path.c_str(),
              O_WRONLY | O_CREAT | O_TRUNC,
              0644);
  }

  if(fd < 0)
    std::cerr << "Error: Unable to open the file: " << path << std::endl;

  const std::string txt = msm + (msm.empty() ? "" : "\n");

  if(write(fd, txt.data(), txt.size()) < 0)
    std::cerr << "Error: Unable to write on file" << std::endl;
  
  close(fd);
}