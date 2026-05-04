#pragma once
#include <string>

namespace retro {

struct Config {
    std::string theme = "green";
    int tabSize = 4;
    bool relativeNumbers = false;
    bool crtEffect = false;
    bool showClock = true;

    static Config& instance();
    void load(const std::string& path = "");
};

}
