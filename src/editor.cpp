#include "editor.h"
#include "terminal.h"
#include "vcs.h"
#include "syntax.h"
#include <fstream>
#include <algorithm>

namespace retro {

Editor::Editor() {
    lines_.push_back("");
}

void Editor::open(const std::string& filename) {
    filename_ = filename;
    lines_.clear();
    std::ifstream f(filename);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            lines_.push_back(line);
        }
    }
    if (lines_.empty()) lines_.push_back("");
    dirty_ = false;
    Syntax::detectLanguage(filename_);
}

void Editor::run() {
    Terminal::enableRawMode();
    Terminal::getSize(screenRows_, screenCols_);
    screenRows_ -= 2;

    while (running_) {
        refreshScreen();
        int key = Terminal::readKey();
        if (key == 0) continue;

        switch (mode_) {
            case Mode::NORMAL:  processNormal(key); break;
            case Mode::INSERT:  processInsert(key); break;
            case Mode::COMMAND: processCommand(key); break;
            case Mode::VISUAL:  processVisual(key); break;
        }
    }

    Terminal::clear();
    Terminal::disableRawMode();
}

void Editor::refreshScreen() {
    Terminal::write("\x1b[?25l");
    Terminal::moveCursor(0, 0);
    drawRows();
    drawStatusBar();
    drawCommandBar();

    int displayRow = cursorRow_ - scrollRow_;
    int displayCol = cursorCol_ - scrollCol_;
    Terminal::moveCursor(displayRow, displayCol);
    Terminal::write("\x1b[?25h");
    Terminal::flush();
}

void Editor::drawRows() {
    for (int y = 0; y < screenRows_; y++) {
        int fileRow = y + scrollRow_;
        if (fileRow < static_cast<int>(lines_.size())) {
            Terminal::setColor(BRIGHT_BLACK);
            std::string num = std::to_string(fileRow + 1);
            while (num.size() < 4) num = " " + num;
            Terminal::write(num + " ");

            std::string& line = lines_[fileRow];
            if (Syntax::hasLanguage()) {
                auto tokens = Syntax::highlight(line);
                for (auto& tok : tokens) {
                    if (tok.start >= scrollCol_ + screenCols_ - 5) break;
                    int tStart = std::max(tok.start, scrollCol_);
                    int tEnd = std::min(tok.start + tok.length, scrollCol_ + screenCols_ - 5);
                    if (tStart >= tEnd) continue;
                    switch (tok.type) {
                        case TokenType::KEYWORD: Terminal::setColor(MAGENTA); break;
                        case TokenType::TYPE: Terminal::setColor(CYAN); break;
                        case TokenType::STRING: Terminal::setColor(YELLOW); break;
                        case TokenType::NUMBER: Terminal::setColor(RED); break;
                        case TokenType::COMMENT: Terminal::setColor(BRIGHT_BLACK); break;
                        default: Terminal::setColor(GREEN); break;
                    }
                    Terminal::write(line.substr(tStart, tEnd - tStart));
                }
            } else {
                Terminal::setColor(GREEN);
                int len = static_cast<int>(line.size()) - scrollCol_;
                if (len < 0) len = 0;
                if (len > screenCols_ - 5) len = screenCols_ - 5;
                if (scrollCol_ < static_cast<int>(line.size()))
                    Terminal::write(line.substr(scrollCol_, len));
            }
        } else {
            Terminal::setColor(BRIGHT_BLACK);
            Terminal::write("~");
        }
        Terminal::resetColor();
        Terminal::write("\x1b[K\r\n");
    }
}

void Editor::drawStatusBar() {
    Terminal::setColor(BLACK, GREEN);
    std::string left = " " + modeString() + " | " +
                       (filename_.empty() ? "[Sin nombre]" : filename_) +
                       (dirty_ ? " [+]" : "");
    std::string right = "L" + std::to_string(cursorRow_ + 1) + ":" +
                        "C" + std::to_string(cursorCol_ + 1) + " ";

    int pad = screenCols_ - static_cast<int>(left.size()) - static_cast<int>(right.size());
    if (pad < 0) pad = 0;
    Terminal::write(left + std::string(pad, ' ') + right);
    Terminal::resetColor();
    Terminal::write("\r\n");
}

void Editor::drawCommandBar() {
    Terminal::write("\x1b[K");
    if (mode_ == Mode::COMMAND) {
        Terminal::setColor(CYAN);
        Terminal::write(":" + commandBuf_);
        Terminal::resetColor();
    } else if (!statusMsg_.empty()) {
        Terminal::setColor(YELLOW);
        Terminal::write(statusMsg_);
        Terminal::resetColor();
    }
}

void Editor::processNormal(int key) {
    statusMsg_.clear();
    switch (key) {
        case 'i':
            pushUndo();
            mode_ = Mode::INSERT;
            break;
        case 'a':
            pushUndo();
            if (cursorCol_ < static_cast<int>(lines_[cursorRow_].size()))
                cursorCol_++;
            mode_ = Mode::INSERT;
            break;
        case 'o':
            pushUndo();
            lines_.insert(lines_.begin() + cursorRow_ + 1, "");
            cursorRow_++;
            cursorCol_ = 0;
            mode_ = Mode::INSERT;
            dirty_ = true;
            break;
        case 'O':
            pushUndo();
            lines_.insert(lines_.begin() + cursorRow_, "");
            cursorCol_ = 0;
            mode_ = Mode::INSERT;
            dirty_ = true;
            break;
        case ':':
            mode_ = Mode::COMMAND;
            commandBuf_.clear();
            break;
        case 'h': case KEY_ARROW_LEFT:
            if (cursorCol_ > 0) cursorCol_--;
            break;
        case 'l': case KEY_ARROW_RIGHT:
            if (cursorCol_ < static_cast<int>(lines_[cursorRow_].size()) - 1)
                cursorCol_++;
            break;
        case 'k': case KEY_ARROW_UP:
            if (cursorRow_ > 0) cursorRow_--;
            break;
        case 'j': case KEY_ARROW_DOWN:
            if (cursorRow_ < static_cast<int>(lines_.size()) - 1)
                cursorRow_++;
            break;
        case '0': case KEY_HOME:
            cursorCol_ = 0;
            break;
        case '$': case KEY_END:
            cursorCol_ = std::max(0, static_cast<int>(lines_[cursorRow_].size()) - 1);
            break;
        case 'g':
            cursorRow_ = 0;
            cursorCol_ = 0;
            break;
        case 'G':
            cursorRow_ = static_cast<int>(lines_.size()) - 1;
            cursorCol_ = 0;
            break;
        case 'x':
            if (!lines_[cursorRow_].empty() && cursorCol_ < static_cast<int>(lines_[cursorRow_].size())) {
                pushUndo();
                lines_[cursorRow_].erase(cursorCol_, 1);
                dirty_ = true;
            }
            break;
        case 'd':
            pushUndo();
            if (lines_.size() > 1) {
                lines_.erase(lines_.begin() + cursorRow_);
                if (cursorRow_ >= static_cast<int>(lines_.size()))
                    cursorRow_ = static_cast<int>(lines_.size()) - 1;
                dirty_ = true;
            } else {
                lines_[0].clear();
                cursorCol_ = 0;
                dirty_ = true;
            }
            break;
        case 'u':
            undo();
            break;
        case 18:
            redo();
            break;
        case '/':
            mode_ = Mode::COMMAND;
            commandBuf_ = "/";
            break;
        case 'n':
            searchNext();
            break;
        case 'N':
            searchPrev();
            break;
        case 'v':
            mode_ = Mode::VISUAL;
            visualStartRow_ = cursorRow_;
            visualStartCol_ = cursorCol_;
            break;
        case 'y':
            yankBuffer_ = {lines_[cursorRow_]};
            yankIsLine_ = true;
            setStatus("1 line yanked");
            break;
        case 'p':
            if (!yankBuffer_.empty()) {
                pushUndo();
                if (yankIsLine_) {
                    for (int i = static_cast<int>(yankBuffer_.size()) - 1; i >= 0; i--)
                        lines_.insert(lines_.begin() + cursorRow_ + 1, yankBuffer_[i]);
                    cursorRow_++;
                    cursorCol_ = 0;
                } else {
                    lines_[cursorRow_].insert(cursorCol_ + 1, yankBuffer_[0]);
                    cursorCol_ += static_cast<int>(yankBuffer_[0].size());
                }
                dirty_ = true;
            }
            break;
    }
    clampCol();

    if (cursorRow_ < scrollRow_) scrollRow_ = cursorRow_;
    if (cursorRow_ >= scrollRow_ + screenRows_) scrollRow_ = cursorRow_ - screenRows_ + 1;
    if (cursorCol_ < scrollCol_) scrollCol_ = cursorCol_;
    if (cursorCol_ >= scrollCol_ + screenCols_ - 5) scrollCol_ = cursorCol_ - screenCols_ + 6;
}

void Editor::processInsert(int key) {
    statusMsg_.clear();
    if (key == KEY_ESCAPE) {
        mode_ = Mode::NORMAL;
        if (cursorCol_ > 0) cursorCol_--;
        return;
    }

    switch (key) {
        case KEY_ENTER: {
            std::string& line = lines_[cursorRow_];
            std::string newLine = line.substr(cursorCol_);
            line = line.substr(0, cursorCol_);
            lines_.insert(lines_.begin() + cursorRow_ + 1, newLine);
            cursorRow_++;
            cursorCol_ = 0;
            dirty_ = true;
            break;
        }
        case KEY_BACKSPACE: {
            if (cursorCol_ > 0) {
                lines_[cursorRow_].erase(cursorCol_ - 1, 1);
                cursorCol_--;
                dirty_ = true;
            } else if (cursorRow_ > 0) {
                cursorCol_ = static_cast<int>(lines_[cursorRow_ - 1].size());
                lines_[cursorRow_ - 1] += lines_[cursorRow_];
                lines_.erase(lines_.begin() + cursorRow_);
                cursorRow_--;
                dirty_ = true;
            }
            break;
        }
        case KEY_ARROW_UP: if (cursorRow_ > 0) cursorRow_--; break;
        case KEY_ARROW_DOWN: if (cursorRow_ < static_cast<int>(lines_.size()) - 1) cursorRow_++; break;
        case KEY_ARROW_LEFT: if (cursorCol_ > 0) cursorCol_--; break;
        case KEY_ARROW_RIGHT:
            if (cursorCol_ < static_cast<int>(lines_[cursorRow_].size())) cursorCol_++;
            break;
        case KEY_TAB:
            lines_[cursorRow_].insert(cursorCol_, "    ");
            cursorCol_ += 4;
            dirty_ = true;
            break;
        default:
            if (key >= 32 && key < 127) {
                lines_[cursorRow_].insert(cursorCol_, 1, static_cast<char>(key));
                cursorCol_++;
                dirty_ = true;
            }
            break;
    }
    clampCol();

    if (cursorRow_ < scrollRow_) scrollRow_ = cursorRow_;
    if (cursorRow_ >= scrollRow_ + screenRows_) scrollRow_ = cursorRow_ - screenRows_ + 1;
}

void Editor::processCommand(int key) {
    if (key == KEY_ESCAPE) {
        mode_ = Mode::NORMAL;
        commandBuf_.clear();
        return;
    }
    if (key == KEY_ENTER) {
        executeCommand(commandBuf_);
        commandBuf_.clear();
        mode_ = Mode::NORMAL;
        return;
    }
    if (key == KEY_BACKSPACE) {
        if (!commandBuf_.empty()) commandBuf_.pop_back();
        else mode_ = Mode::NORMAL;
        return;
    }
    if (key >= 32 && key < 127) {
        commandBuf_ += static_cast<char>(key);
    }
}

void Editor::executeCommand(const std::string& cmd) {
    if (!cmd.empty() && cmd[0] == '/') {
        searchPattern_ = cmd.substr(1);
        searchActive_ = !searchPattern_.empty();
        if (searchActive_) searchNext();
        return;
    }
    if (cmd == "q") {
        if (dirty_) {
            setStatus("Cambios sin guardar! Usa :q! o :w primero");
            return;
        }
        running_ = false;
    } else if (cmd == "q!") {
        running_ = false;
    } else if (cmd == "w") {
        save();
    } else if (cmd == "wq" || cmd == "x") {
        save();
        running_ = false;
    } else if (cmd.substr(0, 2) == "w ") {
        filename_ = cmd.substr(2);
        save();
    } else if (cmd == "vinit") {
        VCS::init(filename_);
        setStatus("VCS inicializado para " + filename_);
    } else if (cmd.substr(0, 8) == "vcommit ") {
        if (dirty_) save();
        VCS::commit(filename_, cmd.substr(8));
        setStatus("Commit creado: " + cmd.substr(8));
    } else if (cmd == "vlog") {
        Terminal::clear();
        Terminal::moveCursor(0, 0);
        Terminal::setColor(CYAN);
        Terminal::write("=== HISTORIAL DE COMMITS ===\r\n\r\n");
        auto commits = VCS::log(filename_);
        for (auto& c : commits) {
            Terminal::setColor(YELLOW);
            Terminal::write("[" + c.id + "] ");
            Terminal::setColor(WHITE);
            Terminal::write(c.message);
            Terminal::setColor(BRIGHT_BLACK);
            Terminal::write(" (" + c.timestamp + ")\r\n");
        }
        if (commits.empty()) {
            Terminal::setColor(RED);
            Terminal::write("No hay commits.\r\n");
        }
        Terminal::resetColor();
        Terminal::write("\r\nPulsa cualquier tecla...");
        Terminal::flush();
        Terminal::readKey();
    } else if (cmd == "vdiff") {
        Terminal::clear();
        Terminal::moveCursor(0, 0);
        Terminal::setColor(CYAN);
        Terminal::write("=== DIFF vs ULTIMO COMMIT ===\r\n\r\n");
        std::string d = VCS::diff(filename_);
        for (char ch : d) {
            if (ch == '\n') Terminal::write("\r\n");
            else Terminal::write(std::string(1, ch));
        }
        Terminal::resetColor();
        Terminal::write("\r\nPulsa cualquier tecla...");
        Terminal::flush();
        Terminal::readKey();
    } else if (cmd == "vstatus") {
        setStatus(VCS::status(filename_));
    } else {
        setStatus("Comando desconocido: " + cmd);
    }
}

void Editor::save() {
    if (filename_.empty()) {
        setStatus("No hay nombre de archivo. Usa :w <nombre>");
        return;
    }
    std::ofstream f(filename_);
    for (size_t i = 0; i < lines_.size(); i++) {
        f << lines_[i];
        if (i < lines_.size() - 1) f << '\n';
    }
    dirty_ = false;
    setStatus("Guardado: " + filename_ + " (" + std::to_string(lines_.size()) + " lineas)");
}

void Editor::setStatus(const std::string& msg) {
    statusMsg_ = msg;
}

int Editor::clampCol() {
    int maxCol = static_cast<int>(lines_[cursorRow_].size());
    if (mode_ == Mode::NORMAL && maxCol > 0) maxCol--;
    if (cursorCol_ > maxCol) cursorCol_ = maxCol;
    if (cursorCol_ < 0) cursorCol_ = 0;
    return cursorCol_;
}

std::string Editor::modeString() {
    switch (mode_) {
        case Mode::NORMAL: return "NORMAL";
        case Mode::INSERT: return "INSERT";
        case Mode::COMMAND: return "COMMAND";
        case Mode::VISUAL: return "VISUAL";
    }
    return "???";
}

void Editor::searchNext() {
    if (!searchActive_ || searchPattern_.empty()) return;

    int startRow = cursorRow_;
    int startCol = cursorCol_ + 1;

    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        int row = (startRow + i) % static_cast<int>(lines_.size());
        int col = (i == 0) ? startCol : 0;
        auto pos = lines_[row].find(searchPattern_, col);
        if (pos != std::string::npos) {
            cursorRow_ = row;
            cursorCol_ = static_cast<int>(pos);
            setStatus("/" + searchPattern_);
            return;
        }
    }
    setStatus("Pattern not found: " + searchPattern_);
}

void Editor::searchPrev() {
    if (!searchActive_ || searchPattern_.empty()) return;

    int startRow = cursorRow_;
    int startCol = cursorCol_ - 1;

    for (int i = 0; i < static_cast<int>(lines_.size()); i++) {
        int row = (startRow - i + static_cast<int>(lines_.size())) % static_cast<int>(lines_.size());
        std::string& line = lines_[row];
        int searchEnd = (i == 0) ? startCol : static_cast<int>(line.size());
        if (searchEnd < 0) continue;

        auto pos = line.rfind(searchPattern_, searchEnd);
        if (pos != std::string::npos) {
            cursorRow_ = row;
            cursorCol_ = static_cast<int>(pos);
            setStatus("?" + searchPattern_);
            return;
        }
    }
    setStatus("Pattern not found: " + searchPattern_);
}

void Editor::processVisual(int key) {
    switch (key) {
        case KEY_ESCAPE:
            mode_ = Mode::NORMAL;
            break;
        case 'h': case KEY_ARROW_LEFT:
            if (cursorCol_ > 0) cursorCol_--;
            break;
        case 'l': case KEY_ARROW_RIGHT:
            if (cursorCol_ < static_cast<int>(lines_[cursorRow_].size()) - 1)
                cursorCol_++;
            break;
        case 'k': case KEY_ARROW_UP:
            if (cursorRow_ > 0) cursorRow_--;
            break;
        case 'j': case KEY_ARROW_DOWN:
            if (cursorRow_ < static_cast<int>(lines_.size()) - 1)
                cursorRow_++;
            break;
        case 'y':
            yankSelection();
            mode_ = Mode::NORMAL;
            break;
        case 'd':
            yankSelection();
            deleteSelection();
            mode_ = Mode::NORMAL;
            break;
    }
}

void Editor::yankSelection() {
    int startRow = std::min(visualStartRow_, cursorRow_);
    int endRow = std::max(visualStartRow_, cursorRow_);
    yankBuffer_.clear();
    for (int i = startRow; i <= endRow; i++)
        yankBuffer_.push_back(lines_[i]);
    yankIsLine_ = true;
    setStatus(std::to_string(endRow - startRow + 1) + " lines yanked");
}

void Editor::deleteSelection() {
    int startRow = std::min(visualStartRow_, cursorRow_);
    int endRow = std::max(visualStartRow_, cursorRow_);
    pushUndo();
    lines_.erase(lines_.begin() + startRow, lines_.begin() + endRow + 1);
    if (lines_.empty()) lines_.push_back("");
    cursorRow_ = std::min(startRow, static_cast<int>(lines_.size()) - 1);
    cursorCol_ = 0;
    dirty_ = true;
}

void Editor::pushUndo() {
    undoStack_.push_back({lines_, cursorRow_, cursorCol_});
    if (undoStack_.size() > 100) undoStack_.pop_front();
    redoStack_.clear();
}

void Editor::undo() {
    if (undoStack_.empty()) {
        setStatus("Already at oldest change");
        return;
    }
    redoStack_.push_back({lines_, cursorRow_, cursorCol_});
    auto& snap = undoStack_.back();
    lines_ = snap.lines;
    cursorRow_ = snap.row;
    cursorCol_ = snap.col;
    undoStack_.pop_back();
    dirty_ = true;
}

void Editor::redo() {
    if (redoStack_.empty()) {
        setStatus("Already at newest change");
        return;
    }
    undoStack_.push_back({lines_, cursorRow_, cursorCol_});
    auto& snap = redoStack_.back();
    lines_ = snap.lines;
    cursorRow_ = snap.row;
    cursorCol_ = snap.col;
    redoStack_.pop_back();
    dirty_ = true;
}

}
