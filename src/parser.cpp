#include "parser.hpp"
#include "common.hpp"

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

// Parser function to identify commands in a string
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