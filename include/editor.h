#pragma once
#include <string>
#include <vector>
#include <deque>

namespace retro {

enum class Mode { NORMAL, INSERT, COMMAND, VISUAL };

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

    std::string searchPattern_;
    bool searchActive_ = false;

    struct Snapshot {
        std::vector<std::string> lines;
        int row, col;
    };
    std::deque<Snapshot> undoStack_;
    std::deque<Snapshot> redoStack_;
    void pushUndo();
    void undo();
    void redo();

    int visualStartRow_ = 0;
    int visualStartCol_ = 0;
    std::vector<std::string> yankBuffer_;
    bool yankIsLine_ = false;

    void processVisual(int key);
    void yankSelection();
    void deleteSelection();

    struct Buffer {
        std::vector<std::string> lines;
        std::string filename;
        int cursorRow = 0, cursorCol = 0;
        int scrollRow = 0, scrollCol = 0;
        bool dirty = false;
    };
    std::vector<Buffer> buffers_;
    int currentBuffer_ = 0;
    void switchBuffer(int idx);
    void saveCurrentBuffer();
    void loadCurrentBuffer();

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
    void searchNext();
    void searchPrev();

    int clampCol();
    std::string modeString();
};

}
