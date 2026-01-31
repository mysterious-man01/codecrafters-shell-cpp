#include "shell.hpp"

#include <iostream>
//#include <fstream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <fcntl.h>

namespace fs = std::filesystem;

void builtin_cmds(const std::vector<std::string>& cmd);

std::vector<std::string> parser(const std::string& str);

std::string echo(std::vector<std::string> str);

std::string type(std::vector<std::string> str);

std::string pwd();

void cd(std::vector<std::string> path);

int OSexec(std::vector<std::string> cmd);

std::string history(std::vector<std::string> n);

void write_file(std::string path, std::string msm, bool append);

void build_cmdline(const std::string& cmd_tokens,
                  std::vector<std::string>& cmdpipe);

const std::string PATH = std::getenv("PATH");

std::string previous_path;
std::vector<std::string> his;
bool stdout_redirect = false;
bool stderr_redirect = false;
bool append = false;

enum class State {
  Normal,
  SingleQuote,
  DoubleQuote
};

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string prompt;
  std::vector<std::string> command;

  // Shell REPL loop
  do{
    enable_raw_mode();

    shell(prompt);

    disable_raw_mode();

    his.push_back(prompt);

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

void builtin_cmds(const std::vector<std::string>& cmd){
  // Exits shell
  if(cmd[0] == "exit"){
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
    std::cout << history(cmd) << std::endl;
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

std::vector<std::string> parser(const std::string& str){
  State state = State::Normal;
  std::string current;
  std::vector<std::string> tokens;
  
  for(size_t i=0; i < str.size(); i++){
    char ch = str[i];

    switch(state){
      case State::Normal:
        if(ch == ' '){
          if(!current.empty()){
            tokens.push_back(current);
            current.clear();
          }
        }
        else if(ch == '\''){
          state = State::SingleQuote;
        }
        else if(ch == '"'){
          state = State::DoubleQuote;
        }
        else if(ch == '|' && i+1 < str.size() && str[i+1] != '|'){
          if(!current.empty()){
            tokens.push_back(current);
            current.clear();
          }
        }
        else if(ch == '\\' &&  i+1 < str.size()){
          current += str[++i];
        }
        else{
          if(i+1 <str.size() && i-1 > 0){
            if(ch == '2' && str[i+1] == '>')
              stderr_redirect = true;

            if(ch =='&' && str[i+1] == '>'){
              stdout_redirect = true;
              stderr_redirect = true;
            }
            
            if((ch == '>' && str[i-1] == ' ' && 
              (str[i+1] == ' ' || str[i+1] == '>')) || 
              (ch == '1' && str[i+1] == '>' && str[i-1]))
              stdout_redirect = true;

            if(ch == '>' && str[i+1] == '>')
              append = true;
          }
          
          current += ch;
        }
        break;
      
      case State::SingleQuote:
        if(ch == '\''){
          state = State::Normal;
        }
        else{
          current += ch;
        }
        break;

      case State::DoubleQuote:
        if(ch == '"'){
          state = State::Normal;
        }
        else if(ch == '\\' &&  i+1 < str.size()){
          if(str[i + 1] == '\"' || str[i + 1] == '\\')
            current += str[++i];
          else
            current += ch;
        }
        else{
          current += ch;
        }
    }
  }

  if(!current.empty())
    tokens.push_back(current);

  return tokens;
}

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

std::string pwd(){
  return fs::current_path().string();
}

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

// History builtin command
std::string history(std::vector<std::string> n){
  if(his.empty())
    return "";

  int i;
  try{
    if(n.size() == 1){
      i = 0;
    } else
      i = his.size() - std::atoi(n[1].c_str());
  } catch(const std::bad_cast& e){
    i = 0;
  }

  std::string ftxt = "";
  for(; i < his.size(); i++){
    if(!ftxt.empty())
      ftxt += "\n";

    ftxt += std::to_string(i+1) + "  " + his[i]; 
  }

  return ftxt;
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

  // std::ofstream outFile(path);

  // if(outFile.is_open()){
    
  //   if(msm == ""){
  //     outFile << msm;
  //   }
  //   else
  //     outFile << msm << std::endl;

  //   outFile.close();
  // } else{
  //   std::cerr << "Error: Unable to open the file: " << path << std::endl;
  // }
}

// Identify and split piped command
void build_cmdline(const std::string& cmd_tokens,
                  std::vector<std::string>& cmdpipe)
{
  size_t offset = 0;
  while(offset < cmd_tokens.size()){
    size_t pos = cmd_tokens.find('|', offset);
    if(pos == std::string::npos){
      cmdpipe.push_back(cmd_tokens.substr(offset));
      break;
    }

    cmdpipe.push_back(cmd_tokens.substr(offset, pos - offset));
    offset = pos + 1;
  }
}