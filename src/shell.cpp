#include "shell.hpp"

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <vector>
#include <filesystem>
#include <unordered_set>
#include <algorithm>

namespace fs = std::filesystem;

const std::string SIMBOL = "$ ";

termios orig_termios;
bool wasTABed = false;

// Disable shell mode
void disable_raw_mode(){
    write(STDOUT_FILENO, "\r\x1b[2K\r", 6);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// Enable shell mode
void enable_raw_mode(){
    std::cout.flush();
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    termios raw = orig_termios;

    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to refresh terminal's line
void refresh_line(const std::string& buffer, size_t cursor_pos){
    write(STDOUT_FILENO, "\r\x1b[2K", 5);
    write(STDOUT_FILENO, SIMBOL.c_str(), SIMBOL.size());
    write(STDOUT_FILENO, buffer.c_str(), buffer.size());

    char seq[32];
    // Must G (horizontal absolute) instead of C to correct cursor
    snprintf(seq, sizeof(seq), "\x1b[%zuG", SIMBOL.size() + cursor_pos + 1);
    write(STDOUT_FILENO, seq, strlen(seq));
}

// Function to handle arrow butons
void handle_arrows(const char& ch, size_t& cursor_pos, const std::string& buffer){
    switch(ch){
        case 'A': // arrow up (command history)
            //
            break;
        case 'C': // right
            if(cursor_pos < buffer.size())
                cursor_pos++;
            break;
        case 'D': // left
            if(cursor_pos > 0)
                cursor_pos--;
            break;
    }
}

// Function to handle characters
void handle_chars(const char ch, std::string& buffer, size_t& cursor_pos){
    if(ch == '\b' || ch == '\x7f'){
        if(cursor_pos > 0){
            buffer.erase(cursor_pos - 1, 1);
            cursor_pos--;
        }
    }
    else if(cursor_pos <= buffer.size()){
        buffer.insert(cursor_pos, 1, ch);
        cursor_pos++;
    }
}

std::vector<std::string> exec_finder(const std::string& prefix, 
                                     std::vector<std::string>& matches)
{
    const char* env = std::getenv("PATH");
    if(!env) return {};

    std::string PATH(env);
    std::unordered_set<std::string> seen;
    size_t offset = 0;
    
    if(!matches.empty()){
        for(auto& entry : matches)
            seen.insert(entry);
    }

    while(offset < PATH.size()){
        size_t pos = PATH.find(':', offset);
        std::string path_dir;

        if (pos == std::string::npos){
            path_dir = PATH.substr(offset);
            offset = PATH.size();
        } else{
            path_dir = PATH.substr(offset, pos - offset);
            offset = pos + 1;
        }

        if(path_dir.empty())
            continue;

        if(!fs::exists(path_dir))
            continue;

        try{
            for(const auto& entry : fs::directory_iterator(path_dir)){
                if(!fs::is_regular_file(entry) && !fs::is_symlink(entry))
                    continue;

                fs::perms perms = fs::status(entry).permissions();
                if((perms & fs::perms::owner_exec) == fs::perms::none &&
                    (perms & fs::perms::group_exec) == fs::perms::none &&
                    (perms & fs::perms::others_exec) == fs::perms::none)
                    continue;

                std::string name = entry.path().filename().string();

                if(name.compare(0, prefix.size(), prefix) == 0){
                    if(seen.insert(name).second)
                        matches.push_back(name);
                }
            }
        } catch(fs::filesystem_error& e){
            continue;
        }
    }

    return matches;
}

void longuest_common_prefix(size_t& cursor_pos,
                            std::string& buffer,
                            const std::string& prefix,
                            const std::vector<std::string>& matches)
{
    if(matches.empty()) return;

    std::string pre = prefix;
    std::string ref = matches[0];
    
    for(int j=1; j < matches.size(); j++){
        for(int k=0; k < ref.size(); k++){
            if(ref[k] != matches[j][k])
                break;    
            
            if(k > pre.size() - 1){
                pre += ref[k];
                continue;
            }

            if(k == pre.size() - 1){
                if(pre[k] != ref[k])
                    pre[k] = ref[k];
            }
        }

    }

    if(pre.size() > prefix.size()){
        cursor_pos = pre.size();
        buffer = pre;
    }
}

// Shell <TAB> function
void TABcomplete(std::string& buffer, size_t& cursor_pos){
    const std::string prefix = buffer.substr(0, cursor_pos);
    const std::vector<std::string> builtin_cmd = {"exit", "echo", "cd", "type", "pwd"};
    std::vector<std::string> matches;
    
    for(int i=0; i < builtin_cmd.size(); i++){
        if(builtin_cmd[i].compare(0, prefix.size(), prefix) == 0)
            matches.push_back(builtin_cmd[i]);
    }

    exec_finder(prefix, matches);

    if(matches.size() == 1){
        buffer = matches[0] + " ";
        cursor_pos = matches[0].size() + 1;
    }
    else if(matches.size() > 1){
        std::sort(matches.begin(), matches.end());

        longuest_common_prefix(cursor_pos, buffer, prefix, matches);
        
        if(!wasTABed){
            wasTABed = true;
            write(STDOUT_FILENO, "\x07", 1);
            return;
        }
        wasTABed = false;

        write(STDOUT_FILENO, "\r\n", 2);
        for(int i=0; i < matches.size(); i++)
            write(STDOUT_FILENO, (matches[i]+"  ").c_str(), matches[i].size() + 2);

        write(STDOUT_FILENO, "\n", 1);
    }
    else
        write(STDOUT_FILENO, "\x07", 1);
}

// Main function for shell
void shell(std::string& buffer){
    char ch;
    size_t cursor_pos = 0;

    while(true){
        refresh_line(buffer, cursor_pos);

        read(STDIN_FILENO, &ch, 1);
        
        if(ch == '\n' || ch == '\r'){
            write(STDOUT_FILENO, "\r\n", 2);
            break;
        }
        
        if(ch == '\t' && !buffer.empty()){
            TABcomplete(buffer, cursor_pos);
            write(STDOUT_FILENO, "\r", 1);
            continue;
        }

        else if(ch == '\x1b'){
            char seq[3];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);
            
            if(seq[0] == '['){
                if(seq[1] >= '0' && seq[1] <= '9'){
                    read(STDIN_FILENO, &seq[2], 1);
                    
                    // Delete function
                    if(seq[1] == '3' && seq[2] == '~'){
                        if(cursor_pos < buffer.size()){
                            buffer.erase(cursor_pos, 1);
                        }
                    }
                }
                else
                    handle_arrows(seq[1], cursor_pos, buffer);
            }
        }
        else{
            handle_chars(ch, buffer, cursor_pos);
        }
    }
}

// Test function
// int main(){
//     enable_raw_mode();

//     std::string buf;

//     shell(buf);

//     disable_raw_mode();

//     std::cout << "1> " << buf << std::endl;
    
//     return 0;
// }