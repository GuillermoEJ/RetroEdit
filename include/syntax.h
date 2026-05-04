#pragma once
#include <string>
#include <vector>
#include <unordered_set>

namespace retro {

enum class TokenType { KEYWORD, TYPE, STRING, NUMBER, COMMENT, NORMAL };

struct Token {
    int start;
    int length;
    TokenType type;
};

class Syntax {
public:
    static void detectLanguage(const std::string& filename);
    static std::vector<Token> highlight(const std::string& line);
    static bool hasLanguage();

private:
    static std::string extension_;
    static std::unordered_set<std::string> keywords_;
    static std::unordered_set<std::string> types_;
    static bool inBlockComment_;
};

}
