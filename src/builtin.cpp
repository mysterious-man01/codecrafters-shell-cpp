#include "builtin.hpp"
#include "common.hpp"
#include "history.hpp"

#include <iostream>
#include <filesystem>
#include <fcntl.h>
#include <sys/wait.h>

namespace fs = std::filesystem;

const std::string PATH = std::getenv("PATH");

std::string previous_path;

// echo command
std::string echo(std::vector<std::string> str){
  std::string txt;

  if(str.size() > 1){
    bool stop = false;
    for(int i=1; i < str.size(); i++){
      std::string& token = str[i];
      for(int j=0; j < token.size(); j++){
        if(j+1 < token.size()){
          if(token[j] == '>' || token[j] == '1' || 
            token[j] == '2' || token[j] == '&' && token[j+1] == '>'){
            stop = true;
            break;
          }
        }
        else{
          if(token[j] == '>'){
            stop = true;
            break;
          }
        }
      }

      if(stop)
        break;
      
      txt += str[i] + " ";
    }
  }

  if(!txt.empty())
    txt.pop_back();

  return txt;
}

// pwd command
std::string pwd(){
  return fs::current_path().string();
}

// cd command to change shell workig directory
void cd(std::vector<std::string> path){
  if(path.size() == 1)
    path.push_back("");

  if(path[1] == "-"){
    path[1] = previous_path.empty() ? pwd() : previous_path;
    previous_path = pwd();
  }

  if(path[1] == "~"){
    path[1] = std::getenv("HOME");
  }
  
  previous_path = pwd();
  
  if(chdir(path[1].c_str()) != 0){
    std::cout << "cd: " << path[1] << ": No such file or directory" << std::endl;
  }
}

// type command
std::string type(std::vector<std::string> str){
  if(str.size() == 1)
    str.push_back("");
  
  std::string cmd = str[1];

  if(cmd == "") return "";

  if(cmd == "exit" || cmd == "echo" || cmd == "history"
    || cmd == "type" || cmd == "pwd" || cmd == "cd")
    return cmd + " is a shell builtin";
  
  size_t offset = 0, temp; 
  const size_t npos = PATH.npos;
  std::string path_dir;
  
  while(offset < PATH.size()){
    temp = PATH.find(':', offset);
    if(temp == std::string::npos){
      path_dir = PATH.substr(offset, PATH.find('\n', offset) - offset);
      break;
    }

    path_dir = PATH.substr(offset, temp - offset);
    offset = temp + 1;

    if(path_dir.empty()) continue;

    if(fs::exists(path_dir)){
      try{
        for(const auto& entry : fs::directory_iterator(path_dir)){
          if(cmd == entry.path().filename() && fs::is_regular_file(entry)){
            fs::perms perms = fs::status(entry).permissions();

            if((perms & fs::perms::owner_exec) != fs::perms::none || 
               (perms & fs::perms::group_exec) != fs::perms::none || 
               (perms & fs::perms::others_exec) != fs::perms::none)
              return cmd + " is " + entry.path().string();
          }
        }
      } catch(const fs::filesystem_error& e){
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
      }
    }
  }
  
  return cmd + ": not found";
}

// External command executor
int OSexec(std::vector<std::string> cmd){
  if(cmd.empty()) return -1;

  size_t offset = 0, temp;
  std::string path_dir;
  std::string cmd_path;

  while(offset < PATH.size()){
    temp = PATH.find(':', offset);
    if(temp == std::string::npos){
      path_dir = PATH.substr(offset, PATH.find('\n', offset) - offset);
      break;
    }

    path_dir = PATH.substr(offset, temp - offset);
    offset = temp + 1;

    if(path_dir.empty()) continue;

    if(fs::exists(path_dir)){
      try{
        for(const auto& entry : fs::directory_iterator(path_dir)){
          if(cmd[0] == entry.path().filename() && fs::is_regular_file(entry)){
            fs::perms perms = fs::status(entry).permissions();

            if((perms & fs::perms::owner_exec) != fs::perms::none || 
               (perms & fs::perms::group_exec) != fs::perms::none || 
               (perms & fs::perms::others_exec) != fs::perms::none)
            {
              cmd_path = entry.path();
              break;
            }
          }
        }
      } catch(fs::filesystem_error& e){
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
      }
    }
  }

  if(cmd_path.empty()) return -1;

  bool stop = false;
  std::vector<char *> c_argv;

  for(std::string& token : cmd){
    for(int i=0; i < token.size(); i++){
      if(token[i] == '>' || (i+1 < token.size() && token[i] == '1' && token[i+1] == '>')){
        stop = true;
        break;
      }
    }

    if(stop)
      break;

    c_argv.push_back(token.data());
  }
  c_argv.push_back(nullptr);

  pid_t pid = fork();
  if(pid == 0){
    if(stdout_redirect || stderr_redirect){
      int fd;
      if(append){
        fd = open(cmd[cmd.size()-1].c_str(),
                  O_WRONLY | O_CREAT | O_APPEND,
                  0644);
      }
      else{
        fd = open(cmd[cmd.size()-1].c_str(),
                  O_WRONLY | O_CREAT | O_TRUNC,
                  0644);
      }

      if(fd < 0) {
        perror("open");
        _exit(1);
      }

      if(stdout_redirect && !stderr_redirect){
        dup2(fd, STDOUT_FILENO);
      }
      else if(!stdout_redirect && stderr_redirect){
        dup2(fd, STDERR_FILENO);
      }
      else{
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
      }
      
      close(fd);
    }
    
    execvp(cmd[0].c_str(), c_argv.data());
    perror("execvp");
    _exit(1);
  }
  else{
    waitpid(pid, nullptr, 0);
  }

  return 0;
}

// Buitin commands
void builtin_cmds(const std::vector<std::string>& cmd){
  // Exits shell
  if(cmd[0] == "exit"){
    savehist();
    exit(0);
  }
  
  // Call command echo
  else if(cmd[0] == "echo"){
    if(stdout_redirect){
      
      write_file(cmd[cmd.size() - 1], echo(cmd), append);
    }
    else{
      std::cout << echo(cmd) << '\n';

      if(stderr_redirect){
        write_file(cmd[cmd.size() - 1], "", append);
      }
    }
  }

  // Call command type
  else if(cmd[0] == "type"){
    if(stdout_redirect){
      write_file(cmd[cmd.size() - 1], type(cmd), append);
    }
    else
      std::cout << type(cmd) << '\n';

      if(stderr_redirect){
        write_file(cmd[cmd.size() - 1], "", append);
      }
  }

  // Call command pwd
  else if(cmd[0] == "pwd"){
    if(stdout_redirect){
      write_file(cmd[cmd.size() - 1], pwd(), append);
    }
    else
      std::cout << pwd() << std::endl;

      if(stderr_redirect){
        write_file(cmd[cmd.size() - 1], "", append);
      }
  }

  // Change working directory
  else if(cmd[0] == "cd"){
    cd(cmd);
  }

  // Shows all past commands typed on shell
  else if(cmd[0] == "history"){
    if(stdout_redirect){
      write_file(cmd[cmd.size() - 1], history_command(cmd), append);
    }
    else{
      std::cout << history_command(cmd);
      
      if(stderr_redirect){
        write_file(cmd[cmd.size() - 1], "", append);
      }
    }
  }

  // Calls external programs
  else{
    if(OSexec(cmd))
      std::cout << cmd[0] << ": command not found" << "\n";
  }

  /////////////////////////
  // Redirection to file bug after redirection to file using pipe,
  // after the piped command with redirect the posterior command
  // will retirect to a file, even when user has not typed redirection command
  ////////////////////////

  stdout_redirect = false;
  stderr_redirect = false;
  append = false;
}