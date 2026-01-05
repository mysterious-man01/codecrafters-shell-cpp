#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string prompt;

  // TODO: Uncomment the code below to pass the first stage
  do{
    std::cout << "$ ";

    std::getline(std::cin, prompt);
    if(prompt == "exit"){
      return 0;
    }
    
    std::cout << prompt << ": command not found" << "\n";
  } while(true);

  return 0;
}
