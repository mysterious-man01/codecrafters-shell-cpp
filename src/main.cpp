#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

namespace fs = std::filesystem;

std::vector<std::string> parser(std::string str);

std::string echo(std::vector<std::string> str);

std::string type(std::vector<std::string> str);

std::string pwd();

void cd(std::vector<std::string> path);

int OSexec(std::vector<std::string> cmd);

const std::string PATH = std::getenv("PATH");
std::string previous_path;

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
  std::string command;

  size_t end_cmd_pos;

  // Shell REPL loop
  do{
    std::cout << "$ ";
    std::getline(std::cin, prompt);

    std::vector<std::string> command = parser(prompt);

    if(command.empty()) continue;

    // Exits shell
    if(command[0] == "exit"){
      return 0;
    }
    
    // Call command echo
    else if(command[0] == "echo"){
      std::cout << echo(command) << '\n';
    }

    // Call command type
    else if(command[0] == "type"){
      std::cout << type(command) << '\n';
    }

    // Call command pwd
    else if(command[0] == "pwd"){
      std::cout << pwd() << std::endl;
    }

    else if(command[0] == "cd"){
      cd(command);
    }

    else{
      if(OSexec(command))
        std::cout << command[0] << ": command not found" << "\n";
    }
  } while(true);

  return 0;
}

std::vector<std::string> parser(std::string str){
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
        else if(ch == '\\' &&  i+1 < str.size()){
          current += str[++i];
        }
        else{
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
    for(int i=1; i<str.size(); i++){
      txt += str[i] + " ";
    }
  }

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

  if(cmd == "exit" || cmd == "echo" 
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

  std::vector<char *> c_argv;
  for(auto& i : cmd){
    c_argv.push_back(i.data());
  }
  c_argv.push_back(nullptr);

  pid_t pid = fork();
  if(pid == 0){
    execvp(cmd[0].c_str(), c_argv.data());
    perror("execvp");
    return -1;
  }
  else{
    waitpid(pid, nullptr, 0);
  }

  return 0;
}