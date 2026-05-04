#include "vcs.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

namespace retro {

void VCS::init(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    fs::create_directories(dir + "/branches");
    writeFile(dir + "/BRANCH", "main");
    writeFile(dir + "/branches/main", "");
}

void VCS::commit(const std::string& filepath, const std::string& message) {
    std::string dir = repoDir(filepath);
    if (!fs::exists(dir)) init(filepath);

    std::string content = readFile(filepath);
    std::string hash = hashContent(content).substr(0, 8);
    std::string ts = currentTimestamp();

    std::string commitData = hash + "\n" + message + "\n" + ts + "\n" + content;
    writeFile(dir + "/" + hash, commitData);

    std::string branch = currentBranch(filepath);
    std::string branchFile = dir + "/branches/" + branch;
    std::string head = readFile(branchFile);
    head = hash + "\n" + head;
    writeFile(branchFile, head);
}

std::vector<Commit> VCS::log(const std::string& filepath) {
    std::vector<Commit> result;
    std::string dir = repoDir(filepath);
    std::string branch = currentBranch(filepath);
    std::string branchFile = dir + "/branches/" + branch;
    if (!fs::exists(branchFile)) return result;

    std::string head = readFile(branchFile);
    std::istringstream iss(head);
    std::string hash;
    while (std::getline(iss, hash)) {
        if (hash.empty()) continue;
        std::string data = readFile(dir + "/" + hash);
        std::istringstream ds(data);
        Commit c;
        std::getline(ds, c.id);
        std::getline(ds, c.message);
        std::getline(ds, c.timestamp);
        std::string rest;
        std::getline(ds, rest, '\0');
        c.snapshot = rest;
        result.push_back(c);
    }
    return result;
}

std::string VCS::diff(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    auto commits = log(filepath);
    if (commits.empty()) return "No hay commits para comparar.";

    std::string current = readFile(filepath);
    std::string last = commits[0].snapshot;

    if (current == last) return "Sin cambios respecto al ultimo commit.";

    std::istringstream curStream(current);
    std::istringstream lastStream(last);
    std::string curLine, lastLine;
    std::string output;
    int lineNum = 1;

    std::vector<std::string> curLines, lastLines;
    while (std::getline(curStream, curLine)) curLines.push_back(curLine);
    while (std::getline(lastStream, lastLine)) lastLines.push_back(lastLine);

    size_t maxLines = std::max(curLines.size(), lastLines.size());
    for (size_t i = 0; i < maxLines; i++) {
        std::string cl = i < curLines.size() ? curLines[i] : "";
        std::string ll = i < lastLines.size() ? lastLines[i] : "";
        if (cl != ll) {
            if (i < lastLines.size())
                output += "\x1b[31m- " + std::to_string(i + 1) + ": " + ll + "\x1b[0m\n";
            if (i < curLines.size())
                output += "\x1b[32m+ " + std::to_string(i + 1) + ": " + cl + "\x1b[0m\n";
        }
    }
    return output;
}

std::string VCS::status(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    if (!fs::exists(dir)) return "VCS no inicializado. Usa :vinit";

    auto commits = log(filepath);
    if (commits.empty()) return "Sin commits. Usa :vcommit <mensaje>";

    std::string current = readFile(filepath);
    if (current == commits[0].snapshot)
        return "Clean (" + branch + ", " + std::to_string(commits.size()) + " commits)";
    return "Modified (" + branch + ", " + std::to_string(commits.size()) + " commits)";
}

std::string VCS::currentBranch(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    std::string branch = readFile(dir + "/BRANCH");
    if (branch.empty()) return "main";
    while (!branch.empty() && (branch.back() == '\n' || branch.back() == '\r'))
        branch.pop_back();
    return branch;
}

void VCS::createBranch(const std::string& filepath, const std::string& name) {
    std::string dir = repoDir(filepath);
    std::string current = currentBranch(filepath);
    std::string currentCommits = readFile(dir + "/branches/" + current);
    writeFile(dir + "/branches/" + name, currentCommits);
}

void VCS::checkoutBranch(const std::string& filepath, const std::string& name) {
    std::string dir = repoDir(filepath);
    if (!fs::exists(dir + "/branches/" + name)) return;
    writeFile(dir + "/BRANCH", name);
}

std::string VCS::checkoutCommit(const std::string& filepath, const std::string& hash) {
    std::string dir = repoDir(filepath);
    std::string data = readFile(dir + "/" + hash);
    if (data.empty()) return "";
    std::istringstream ds(data);
    std::string id, msg, ts;
    std::getline(ds, id);
    std::getline(ds, msg);
    std::getline(ds, ts);
    std::string rest;
    std::getline(ds, rest, '\0');
    return rest;
}

std::vector<std::string> VCS::listBranches(const std::string& filepath) {
    std::vector<std::string> result;
    std::string dir = repoDir(filepath) + "/branches";
    if (!fs::exists(dir)) return result;
    for (auto& entry : fs::directory_iterator(dir))
        result.push_back(entry.path().filename().string());
    return result;
}

void VCS::stash(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    std::string content = readFile(filepath);
    writeFile(dir + "/STASH", content);
}

std::string VCS::stashPop(const std::string& filepath) {
    std::string dir = repoDir(filepath);
    std::string stashFile = dir + "/STASH";
    if (!fs::exists(stashFile)) return "";
    std::string content = readFile(stashFile);
    fs::remove(stashFile);
    return content;
}

std::string VCS::repoDir(const std::string& filepath) {
    fs::path p(filepath);
    fs::path dir = p.parent_path() / ".retroedit" / p.filename().string();
    return dir.string();
}

std::string VCS::hashContent(const std::string& content) {
    std::hash<std::string> hasher;
    size_t h = hasher(content);
    std::stringstream ss;
    ss << std::hex << h;
    return ss.str();
}

std::string VCS::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void VCS::writeFile(const std::string& path, const std::string& content) {
    fs::path p(path);
    fs::create_directories(p.parent_path());
    std::ofstream f(path);
    f << content;
}

std::string VCS::currentTimestamp() {
    std::time_t t = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

}
