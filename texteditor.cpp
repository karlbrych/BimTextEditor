#include <iostream>
#include <vector>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include<cstdlib>

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

class Editor {
public:
    void run();
    void openFile(const std::string& filename);
    void processKeypress();
    void refreshScreen();
    void moveCursor(char key);
    void insertChar(char c);
    void deleteChar();
    void forwardDeleteChar();

private:
    std::vector<std::string> lines;
    size_t cursorX = 0, cursorY = 0;
};

void Editor::openFile(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (!file) return;
    char* line = nullptr;
    size_t len = 0;
    while (getline(&line, &len, file) != -1) {
        lines.push_back(line);
    }
    free(line);
    fclose(file);
    if (lines.empty()) {
        lines.push_back("");
    }
}

void Editor::refreshScreen() {
    std::cout << "\x1b[2J"; // Clear screen
    std::cout << "\x1b[H";  // Move cursor to top-left
    for (const auto& line : lines) {
        std::cout << line;
    }
    std::cout << "\x1b[" << cursorY + 1 << ";" << cursorX + 1 << "H"; // Move cursor
    std::cout.flush();
}

void Editor::moveCursor(char key) {
    switch (key) {
        case 'w': if (cursorY > 0) --cursorY; break;
        case 's': if (cursorY < lines.size() - 1) ++cursorY; break;
        case 'a': if (cursorX > 0) --cursorX; break;
        case 'd': 
            if (cursorY < lines.size() && cursorX < lines[cursorY].size()) ++cursorX; 
            break;
    }
    // Ensure cursorX is within the length of the current line
    if (cursorY < lines.size() && cursorX >= lines[cursorY].size()) {
        cursorX = lines[cursorY].size();
    }
}

void Editor::insertChar(char c) {
    if (cursorY >= lines.size()) {
        lines.push_back("");
    }
    lines[cursorY].insert(cursorX, 1, c);
    cursorX++;
}

void Editor::deleteChar() {
    if (cursorY >= lines.size() || cursorX == 0) return;
    lines[cursorY].erase(cursorX - 1, 1);
    cursorX--;
}

void Editor::forwardDeleteChar() {
    if (cursorY >= lines.size() || cursorX >= lines[cursorY].size()) return;
    lines[cursorY].erase(cursorX, 1);
}

void Editor::processKeypress() {
    char c;
    read(STDIN_FILENO, &c, 1);
    if (c == '\x1b') {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 0) return;
        if (read(STDIN_FILENO, &seq[1], 1) == 0) return;
        if (seq[0] == '[') {
            switch (seq[1]) {
                case '3':
                    if (read(STDIN_FILENO, &seq[0], 1) == 0) return;
                    if (seq[0] == '~') {
                        forwardDeleteChar();
                    }
                    break;
            }
        }
    } else {
        switch (c) {
            case 17: // Ctrl + Q
                const char*command="clear";
                disableRawMode();
                exit(0);
                int result=system(command);
                break;
            case 23: // Ctrl + W
                moveCursor('w');
                break;
            case 19: // Ctrl + S
                moveCursor('s');
                break;
            case 1:  // Ctrl + A
                moveCursor('a');
                break;
            case 4:  // Ctrl + D
                moveCursor('d');
                break;
            case 127: // Backspace
                deleteChar();
                break;
            default:
                insertChar(c);
                break;
        }
    }
}

void Editor::run() {
    enableRawMode();
    while (true) {
        refreshScreen();
        processKeypress();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }
    Editor editor;
    editor.openFile(argv[1]);
    editor.run();
    return 0;
}
