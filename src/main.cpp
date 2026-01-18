#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <fcntl.h>

namespace fs = std::filesystem;

std::vector<std::string> parser(std::string str);

std::string echo(std::vector<std::string> str);

std::string type(std::vector<std::string> str);

std::string pwd();

void cd(std::vector<std::string> path);

int OSexec(std::vector<std::string> cmd);

void write_file(std::string path, std::string msm, bool append);

const std::string PATH = std::getenv("PATH");
std::string previous_path;
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
      if(stdout_redirect){
        write_file(command[command.size() - 1], echo(command), append);
      }
      else{
        std::cout << echo(command) << '\n';

        if(stderr_redirect){
          write_file(command[command.size() - 1], "", append);
        }
      }
    }

    // Call command type
    else if(command[0] == "type"){
      if(stdout_redirect){
        write_file(command[command.size() - 1], type(command), append);
      }
      else
        std::cout << type(command) << '\n';

        if(stderr_redirect){
          write_file(command[command.size() - 1], "", append);
        }
    }

    // Call command pwd
    else if(command[0] == "pwd"){
      if(stdout_redirect){
        write_file(command[command.size() - 1], pwd(), append);
      }
      else
        std::cout << pwd() << std::endl;

        if(stderr_redirect){
          write_file(command[command.size() - 1], "", append);
        }
    }

    else if(command[0] == "cd"){
      cd(command);
    }

    else{
      if(OSexec(command))
        std::cout << command[0] << ": command not found" << "\n";
    }

    stdout_redirect = false;
    stderr_redirect = false;
    append = false;
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
    return -1;
  }
  else{
    waitpid(pid, nullptr, 0);
  }

  return 0;
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