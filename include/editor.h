#pragma once
#include <string>
#include <vector>

namespace retro {

enum class Mode { NORMAL, INSERT, COMMAND };

class Editor {
public:
    Editor();
    void open(const std::string& filename);
    void run();

private:
    std::vector<std::string> lines_;
    std::string filename_;
    int cursorRow_ = 0;
    int cursorCol_ = 0;
    int scrollRow_ = 0;
    int scrollCol_ = 0;
    int screenRows_ = 0;
    int screenCols_ = 0;
    Mode mode_ = Mode::NORMAL;
    std::string statusMsg_;
    std::string commandBuf_;
    bool dirty_ = false;
    bool running_ = true;

    void refreshScreen();
    void drawRows();
    void drawStatusBar();
    void drawCommandBar();
    void drawWelcome();

    void processNormal(int key);
    void processInsert(int key);
    void processCommand(int key);
    void executeCommand(const std::string& cmd);

    void save();
    void setStatus(const std::string& msg);

    int clampCol();
    std::string modeString();
};

}
