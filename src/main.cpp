#include <iostream>
#include <string>

std::string echo(std::string str);

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

    if(command == "exit"){
      return 0;
    }
    
    else if(command == "echo"){
      std::cout << echo(prompt.substr(end_cmd_pos + 1, prompt.find('\n', 0))) << '\n';
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