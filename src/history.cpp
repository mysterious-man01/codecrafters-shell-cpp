#include "history.hpp"
#include "common.hpp"

#include <filesystem>

namespace fs = std::filesystem;

std::string HISTFILE;
std::vector<std::string> history;
size_t history_saved = 0;

// Initialize HISTFILE internal variable
void inithist(){
    if(std::getenv("HISTFILE") != nullptr){
        HISTFILE = std::getenv("HISTFILE");
    } else
        HISTFILE = std::string(std::getenv("HOME")) + "/.shellrc";
}

// Get hitory from HISTFILE and saves on memory
void gethist(){
  if(!fs::exists(HISTFILE)){
    return;
  }

  std::string file = read_file(HISTFILE);

  size_t offset = 0;
  while(offset < file.size()){
    size_t pos = file.find('\n', offset);
    if(pos == std::string::npos){
      history.push_back(file.substr(offset));
      break;
    }

    history.push_back(file.substr(offset, pos - offset));
    offset = pos + 1;
  }

  history_saved = history.size();
}

// Save history on in memory to a file in HISTFILE
void savehist(){
  std::string hist;
  for(int i=history_saved; i < history.size(); i++){
    if(!hist.empty())
      hist += "\n";

    hist += history[i];
  }

  if(!hist.empty())
    write_file(HISTFILE, hist, true);
}

// History builtin command
std::string history_command(const std::vector<std::string>& n){
  if(history.empty())
    return "";

  std::string ftxt = "";
  int i;

  if(n.size() > 2){
    if(n[1] == "-r"){
      auto txt = read_file(n.back());
      if(txt.empty())
        return "";

      size_t offset = 0;
      while(offset < txt.size()){
        size_t pos = txt.find('\n', offset);
        if(pos == std::string::npos){
          if(offset < txt.size())
            history.push_back(txt.substr(offset));

          break;
        }

        history.push_back(txt.substr(offset, pos - offset));
        offset = pos + 1;
      }

      return "";
    }

    if(n[1] == "-w"){
      for(i=0; i < history.size(); i++){
        if(!ftxt.empty())
          ftxt += "\n";
        
        ftxt += history[i];
      }

      write_file(n.back(), ftxt, false);
      history_saved = history.size();

      return "";
    }

    if(n[1] == "-a"){
      ftxt = "";
      for(i=history_saved; i < history.size(); i++){
        if(!ftxt.empty())
          ftxt += "\n";
        
        ftxt += history[i];
        history_saved++;
      }

      if(!ftxt.empty())
        write_file(n.back(), ftxt, true);

      return "";
    }
  }
  
  try{
    if(n.size() == 1){
      i = 0;
    } else
      i = history.size() - std::atoi(n[1].c_str());
  } catch(const std::bad_cast& e){
    i = 0;
  }

  for(; i < history.size(); i++)
    ftxt += std::to_string(i+1) + "  " + history[i] + "\n";

  return ftxt;
}