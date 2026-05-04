#include "syntax.h"
#include <algorithm>

namespace retro {

std::string Syntax::extension_;
std::unordered_set<std::string> Syntax::keywords_;
std::unordered_set<std::string> Syntax::types_;
bool Syntax::inBlockComment_ = false;

void Syntax::detectLanguage(const std::string& filename) {
    auto dot = filename.rfind('.');
    if (dot == std::string::npos) {
        extension_.clear();
        return;
    }
    extension_ = filename.substr(dot);

    keywords_.clear();
    types_.clear();
    inBlockComment_ = false;

    if (extension_ == ".cpp" || extension_ == ".c" || extension_ == ".h" || extension_ == ".hpp") {
        keywords_ = {"if", "else", "for", "while", "do", "switch", "case", "break",
                     "continue", "return", "class", "struct", "enum", "namespace",
                     "public", "private", "protected", "virtual", "override",
                     "const", "static", "inline", "template", "typename",
                     "new", "delete", "throw", "try", "catch", "using",
                     "typedef", "sizeof", "nullptr", "true", "false",
                     "include", "define", "ifdef", "ifndef", "endif", "pragma"};
        types_ = {"int", "float", "double", "char", "void", "bool", "long",
                  "short", "unsigned", "signed", "auto", "size_t", "string",
                  "vector", "map", "set", "uint8_t", "uint16_t", "uint32_t", "uint64_t"};
    } else if (extension_ == ".py") {
        keywords_ = {"if", "else", "elif", "for", "while", "def", "class",
                     "return", "import", "from", "as", "try", "except",
                     "finally", "with", "lambda", "yield", "raise", "pass",
                     "break", "continue", "and", "or", "not", "in", "is",
                     "True", "False", "None", "self", "async", "await"};
        types_ = {"int", "float", "str", "bool", "list", "dict", "tuple", "set", "bytes"};
    } else if (extension_ == ".js" || extension_ == ".ts") {
        keywords_ = {"if", "else", "for", "while", "do", "switch", "case",
                     "break", "continue", "return", "function", "var", "let",
                     "const", "class", "new", "this", "throw", "try", "catch",
                     "finally", "import", "export", "default", "from", "async",
                     "await", "yield", "typeof", "instanceof", "true", "false",
                     "null", "undefined"};
        types_ = {"number", "string", "boolean", "object", "any", "void", "never", "Array", "Promise"};
    } else if (extension_ == ".rs") {
        keywords_ = {"fn", "let", "mut", "if", "else", "for", "while", "loop",
                     "match", "return", "struct", "enum", "impl", "trait", "pub",
                     "use", "mod", "crate", "self", "super", "where", "async",
                     "await", "move", "ref", "true", "false", "unsafe"};
        types_ = {"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
                  "f32", "f64", "bool", "char", "str", "String", "Vec", "Option", "Result"};
    }
}

bool Syntax::hasLanguage() {
    return !keywords_.empty();
}

std::vector<Token> Syntax::highlight(const std::string& line) {
    std::vector<Token> tokens;
    int i = 0;
    int len = static_cast<int>(line.size());

    while (i < len) {
        if (inBlockComment_) {
            int start = i;
            while (i < len - 1) {
                if (line[i] == '*' && line[i + 1] == '/') {
                    i += 2;
                    inBlockComment_ = false;
                    break;
                }
                i++;
            }
            if (inBlockComment_) i = len;
            tokens.push_back({start, i - start, TokenType::COMMENT});
            continue;
        }

        if (i < len - 1 && line[i] == '/' && line[i + 1] == '/') {
            tokens.push_back({i, len - i, TokenType::COMMENT});
            break;
        }

        if (i < len - 1 && line[i] == '/' && line[i + 1] == '*') {
            int start = i;
            i += 2;
            inBlockComment_ = true;
            while (i < len - 1) {
                if (line[i] == '*' && line[i + 1] == '/') {
                    i += 2;
                    inBlockComment_ = false;
                    break;
                }
                i++;
            }
            if (inBlockComment_) i = len;
            tokens.push_back({start, i - start, TokenType::COMMENT});
            continue;
        }

        if (line[i] == '#' && (extension_ == ".py" || extension_ == ".rb")) {
            tokens.push_back({i, len - i, TokenType::COMMENT});
            break;
        }

        if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            int start = i;
            i++;
            while (i < len && line[i] != quote) {
                if (line[i] == '\\') i++;
                i++;
            }
            if (i < len) i++;
            tokens.push_back({start, i - start, TokenType::STRING});
            continue;
        }

        if (std::isdigit(line[i]) || (line[i] == '.' && i + 1 < len && std::isdigit(line[i + 1]))) {
            int start = i;
            while (i < len && (std::isalnum(line[i]) || line[i] == '.' || line[i] == 'x' || line[i] == 'X'))
                i++;
            tokens.push_back({start, i - start, TokenType::NUMBER});
            continue;
        }

        if (std::isalpha(line[i]) || line[i] == '_' || line[i] == '#') {
            int start = i;
            while (i < len && (std::isalnum(line[i]) || line[i] == '_'))
                i++;
            std::string word = line.substr(start, i - start);
            if (line[start] == '#' && start + 1 < len) {
                word = line.substr(start + 1, i - start - 1);
            }
            if (keywords_.count(word))
                tokens.push_back({start, i - start, TokenType::KEYWORD});
            else if (types_.count(word))
                tokens.push_back({start, i - start, TokenType::TYPE});
            else
                tokens.push_back({start, i - start, TokenType::NORMAL});
            continue;
        }

        tokens.push_back({i, 1, TokenType::NORMAL});
        i++;
    }

    return tokens;
}

}
