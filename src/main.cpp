#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::string echo(std::string str);

std::string type(std::string str);

const std::string PATH = std::getenv("PATH");

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string prompt;
  std::string command;

  size_t end_cmd_pos;

  std::cout << "PATH=" << PATH << std::endl; // Remove

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

    else{
      std::cout << command << ": command not found" << "\n";
    }
  } while(true);

  return 0;
}

// echo command
std::string echo(std::string str){
  return str;
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

  if(cmd == "exit" || cmd == "echo" || cmd == "type")
    return cmd + " is a shell builtin";
  
  size_t offset = 0, temp; 
  const size_t npos = PATH.npos;
  std::string path_dir;
  
  while(offset != npos){
    if(offset != 0) offset++;

    temp = PATH.find(':', offset);
    if(temp != npos){
      path_dir = PATH.substr(offset, temp - offset);
      offset = temp;
    }
    else{
      path_dir = PATH;
      offset = temp;
    }

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