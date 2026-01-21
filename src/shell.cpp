#include "shell.hpp"

#include <iostream>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <vector>

const std::string SIMBOL = "$ ";

termios orig_termios;

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

// Shell <TAB> function
void TABcomplete(std::string& buffer, size_t& cursor_pos){
    std::string temp = buffer.substr(0, cursor_pos);
    std::vector<std::string> builtin_cmd = {"exit", "echo", "cd", "type", "pwd"};
    std::vector<std::string> matches;
    
    for(int i=0; i < builtin_cmd.size(); i++){
        if(builtin_cmd[i].compare(0, temp.size(), temp) == 0)
            matches.push_back(builtin_cmd[i]);
    }

    if(matches.size() == 1){
        buffer = matches[0] + " ";
        cursor_pos = matches[0].size() + 1;
    }

    if(matches.size() > 1){
        write(STDOUT_FILENO, "\r\n", 2);
        for(int i=0; i < matches.size(); i++)
            write(STDOUT_FILENO, (matches[i]+" ").c_str(), matches[i].size() + 1);

        write(STDOUT_FILENO, "\n", 1);
    }

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