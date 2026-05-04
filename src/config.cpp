#include "config.h"
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace retro {

Config& Config::instance() {
    static Config cfg;
    return cfg;
}

void Config::load(const std::string& path) {
    std::string configPath = path;
    if (configPath.empty()) {
        configPath = ".retroeditrc";
        if (!fs::exists(configPath)) {
            const char* home = std::getenv("HOME");
            if (!home) home = std::getenv("USERPROFILE");
            if (home) configPath = std::string(home) + "/.retroeditrc";
        }
    }

    std::ifstream f(configPath);
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        while (!key.empty() && key.back() == ' ') key.pop_back();
        while (!val.empty() && val.front() == ' ') val.erase(val.begin());

        if (key == "theme") theme = val;
        else if (key == "tab_size") tabSize = std::stoi(val);
        else if (key == "relative_numbers") relativeNumbers = (val == "true" || val == "1");
        else if (key == "crt_effect") crtEffect = (val == "true" || val == "1");
        else if (key == "show_clock") showClock = (val == "true" || val == "1");
    }
}

}
