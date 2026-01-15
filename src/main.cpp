#include <cstddef>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef _WIN32
const char os_pathsep = ';';
#else
const char os_pathsep = ':';
#endif

enum class CommandType { exit, echo, type, pwd, cd, unknown };

CommandType get_command_type(const std::string &cmd) {
  if (cmd == "exit") {
    return CommandType::exit;
  } else if (cmd == "echo") {
    return CommandType::echo;
  } else if (cmd == "type") {
    return CommandType::type;
  } else if (cmd == "pwd") {
    return CommandType::pwd;
  } else if (cmd == "cd") {
    return CommandType::cd;
  } else {
    return CommandType::unknown;
  }
}

std::string trim(const std::string &line) {
  const char *WHITESPACE = " \t\v\r\n";
  std::size_t start = line.find_first_not_of(WHITESPACE);
  std::size_t end = line.find_last_not_of(WHITESPACE);
  return start == end ? std::string() : line.substr(start, end - start + 1);
}

std::vector<std::string> split_by_words(const std::string &text) {
  std::vector<std::string> words;
  std::string word;

  int i = 0;
  while (i < text.size()) {
    if (text[i] == '\"') {
      i++;
      while (text[i] != '\"') {
        word += text[i];
        i++;
      }
    } else if (text[i] == '\'') {
      i++;
      while (text[i] != '\'') {
        word += text[i];
        i++;
      }
    } else if (text[i] == ' ' && word.size() > 0) {
      words.push_back(word);
      word = "";
    } else if (text[i] != ' ') {
      i += text[i] == '\\';
      word += text[i];
    }

    i++;
  }

  if (word.size() > 0) {
    words.push_back(word);
  }

  return words;
}

std::filesystem::path find_executable(std::string &cmd) {
  std::filesystem::path found;

  const char *ENV_PATH = std::getenv("PATH");
  std::stringstream ss(ENV_PATH);

  for (std::string line; std::getline(ss, line, os_pathsep);) {
    std::filesystem::path cur_path(line);
    cur_path /= cmd;

    if (access(cur_path.c_str(), X_OK) == 0) {
      found = cur_path;
      break;
    }
  }

  return found;
}

void execute_external(std::vector<std::string> args,
                      std::filesystem::path &path) {
  if (args.empty()) {
    return;
  }

  std::vector<char *> c_args;
  for (std::string &arg : args) {
    c_args.push_back(arg.data());
  }
  c_args.push_back(nullptr);

  if (!fork()) {
    execv(path.c_str(), c_args.data());
    exit(0);
  } else {
    wait(NULL);
  }
}

void type(std::vector<std::string> args) {
  for (std::string cmd : args) {
    CommandType cmd_type = get_command_type(cmd);

    if (cmd_type != CommandType::unknown) {
      std::cout << cmd << " is a shell builtin\n";
      continue;
    }

    std::filesystem::path path = find_executable(cmd);

    if (path.empty()) {
      std::cout << cmd << ": not found\n";
    } else {
      std::cout << cmd << " is " << path.string() << "\n";
    }
  }
}

void cd(std::vector<std::string> args) {
  std::filesystem::path path;

  if (args.size() <= 1 || args[1] == "~") {
    const char *HOME = std::getenv("HOME");
    path = HOME;
  } else {
    path = args[1];
  }

  if (std::filesystem::exists(path)) {
    std::filesystem::current_path(path);
  } else {
    std::cout << "cd: " << path.string() << ": No such file or directory\n";
  }
}

void execute(std::vector<std::string> args) {
  // C++ doesn't allow switch statements on strings, so use an Enum
  std::string cmd = args[0];

  switch (get_command_type(cmd)) {
  case CommandType::exit: {
    // TODO: account for params in exit; return too many params if this occurs
    std::exit(0);
    break;
  }

  case CommandType::echo: {
    std::string output;

    for (int i = 1; i < args.size(); i++) {
      output += args[i] + " ";
    }

    std::cout << output << "\n";
    break;
  }

  case CommandType::type: {
    args.erase(args.begin());
    type(args);
    break;
  }

  case CommandType::pwd: {
    std::cout << std::filesystem::current_path().string() << "\n";
    break;
  }

  case CommandType::cd: {
    cd(args);
    break;
  }

  case CommandType::unknown: {
    std::filesystem::path path = find_executable(cmd);

    if (path.empty()) {
      std::cout << cmd << ": command not found" << "\n";
    } else {
      execute_external(args, path);
    }
  }
  }
}

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    std::cout << "$ ";

    std::string input;
    std::getline(std::cin, input);

    std::vector<std::string> args = split_by_words(input);
    if (!args.empty()) {
      execute(args);
    }
  }
}
