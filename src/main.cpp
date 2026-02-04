#include "shell.hpp"
#include "parser.hpp"
#include "history.hpp"
#include "builtin.hpp"

#include <iostream>
#include <sys/wait.h>

int main(){
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string prompt;
  std::vector<std::string> command;

  inithist();
  gethist();

  // Shell REPL loop
  do{
    enable_raw_mode();

    shell(prompt);

    disable_raw_mode();
    
    build_cmdline(prompt, command);
    if(command.empty()) continue;

    if(command.size() > 1){
      int prev_fd = -1;
      for(size_t i=0; i < command.size(); i++){
        bool is_last = (i == command.size() - 1);

        auto cmd = parser(command[i]);
      
        int fd[2];
        if(!is_last){
          if(pipe(fd) == -1){
            perror("pipe");
            break;
          }
        }
      
        pid_t pid = fork();
        
        if(pid == 0){
          if(prev_fd != -1){
            dup2(prev_fd, STDIN_FILENO);
            close(prev_fd);
          }

          if(!is_last){
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
          }

          builtin_cmds(cmd);
          _exit(0);
        }

        if(prev_fd != -1)
          close(prev_fd);

        if(!is_last){
          close(fd[1]);
          prev_fd = fd[0];
        }
      }

      while(wait(nullptr) > 0);
    } else{
      auto cmd = parser(command[0]);
      builtin_cmds(cmd);
    }

    prompt = "";
    command.clear();
  } while(true);

  return 0;
}