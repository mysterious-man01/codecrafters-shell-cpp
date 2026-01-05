#include <iostream>
#include <string>

std::string echo(std::string str);

std::string type(std::string str);

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

    // Exits shell
    if(command == "exit"){
      return 0;
    }
    
    // Call command echo
    else if(command == "echo"){
      std::cout << echo(prompt.substr(end_cmd_pos + 1, prompt.find('\n', 0))) << '\n';
    }

    // Call command type
    else if(command == "type"){
      std::cout << type(prompt.substr(end_cmd_pos + 1, prompt.find('\n', 0))) << '\n';
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

  if(cmd == "exit" || cmd == "echo" || cmd == "type"){
    return cmd + " is a shell builtin";
  }
  
  return cmd + ": not found";
}