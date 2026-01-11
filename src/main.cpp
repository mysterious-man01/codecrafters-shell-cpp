#include <iostream>
#include <string>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

namespace fs = std::filesystem;

std::string echo(std::string str);

std::string type(std::string str);

std::string pwd();

void cd(std::string path);

int OSexec(std::string cmd, std::string args);

const std::string PATH = std::getenv("PATH");
std::string previous_path;

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
    
    end_cmd_pos = prompt.find(' ', 0);
    command = prompt.substr(0, end_cmd_pos);

    if(command.empty()) continue;

    // Exits shell
    if(command == "exit"){
      return 0;
    }
    
    // Call command echo
    else if(command == "echo"){
      std::cout << echo(prompt.substr(end_cmd_pos + 1, (prompt.find('\n', 0) - end_cmd_pos))) << '\n';
    }

    // Call command type
    else if(command == "type"){
      std::cout << type(prompt.substr(end_cmd_pos + 1, (prompt.find('\n', 0) - end_cmd_pos))) << '\n';
    }

    // Call command pwd
    else if(command == "pwd"){
      std::cout << pwd() << std::endl;
    }

    else if(command == "cd"){
      cd(prompt.substr(end_cmd_pos + 1, (prompt.find('\n') - end_cmd_pos)));
    }

    else{
      std::string args = prompt.substr(end_cmd_pos + 1, (prompt.find('\n', 0) - end_cmd_pos));

      if(OSexec(command, args))
        std::cout << command << ": command not found" << "\n";
    }
  } while(true);

  return 0;
}

// echo command
std::string echo(std::string str){
  size_t offset = 0;
  std::string txt;
  while(offset < str.size()){
    if(str[offset] == ' '){
      txt += ' ';
      while(offset < str.size() && str[offset] == ' ')
        offset++;
      continue;
    }

    if(offset >= str.size())
      break;

    if(str[offset] == '\''){
      size_t end = str.find('\'', offset + 1);
      if (end == std::string::npos){
        txt += str.substr(offset + 1, str.find('\n', offset) - offset);
        break;
      }

      txt += str.substr(offset + 1, end - offset - 1);
      offset = end + 1;
    }
    else{
      size_t end = str.find(' ', offset);
      size_t quote = str.find('\'', offset);

      if(end < quote){
        txt += str.substr(offset, end - offset);
        offset = end;
      }
      else if(quote < end){
        txt += str.substr(offset, quote - offset);
        offset = quote + 1;
      }
      else{
        end = str.size();
        txt += str.substr(offset, end - offset);
        offset = end;
      }
    }
  }

  return txt;
}

std::string pwd(){
  return fs::current_path().string();
}

void cd(std::string path){
  if(path == "-"){
    path = previous_path.empty() ? pwd() : previous_path;
    previous_path = pwd();
  }

  if(path == "~"){
    path = std::getenv("HOME");
  }
  
  previous_path = pwd();
  
  if(chdir(path.c_str()) != 0){
    std::cout << "cd: " << path << ": No such file or directory" << std::endl;
  }
}

// type command
std::string type(std::string str){
  std::string cmd;
  size_t end_cmd_pos;
  
  end_cmd_pos = str.find(' ', 0);
  if(end_cmd_pos != str.npos){
    cmd = str.substr(0, end_cmd_pos);
  }
  else cmd = str;

  if(cmd.empty()) return "";

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
int OSexec(std::string cmd, std::string args){
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
          if(cmd == entry.path().filename() && fs::is_regular_file(entry)){
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

  offset = 0;
  std::vector<std::string> c_args;
  if(args.empty())
    c_args.push_back("");

  while (offset < args.size()) {
    while (offset < args.size() && args[offset] == ' ')
      offset++;

    if (offset >= args.size())
      break;

    if (args[offset] == '"') {
      size_t end = args.find('"', offset + 1);
      if (end == std::string::npos){
        c_args.push_back(args.substr(offset, args.find('\n', offset) - offset));
        break;
      }

      c_args.push_back(args.substr(offset, end - offset + 2));
      offset = end + 1;
    }
    else if (args[offset] == '\'') {
      size_t end = args.find('\'', offset + 1);
      if (end == std::string::npos){
        c_args.push_back(args.substr(offset, args.find('\n', offset) - offset));
        break;
      }

      c_args.push_back(args.substr(offset + 1, end - offset - 1));
      offset = end + 1;
    }
    else {
      size_t end = args.find(' ', offset);
      if (end == std::string::npos)
          end = args.size();

      c_args.push_back(args.substr(offset, end - offset));
      offset = end;
    }
  }

  std::vector<char *> c_argv;
  c_argv.push_back(const_cast<char*>(cmd.c_str()));

  for(auto& i : c_args){
    if(i != "")
      c_argv.push_back(i.data());
  }

  c_argv.push_back(nullptr);

  pid_t pid = fork();

  if(pid == 0){
    execvp(cmd.c_str(), c_argv.data());
    perror("execvp");
    return -1;
  }
  else{
    waitpid(pid, nullptr, 0);
  }

  return 0;
}