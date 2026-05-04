#pragma once
#include <string>
#include <vector>

namespace retro {

struct Commit {
    std::string id;
    std::string message;
    std::string timestamp;
    std::string snapshot;
};

class VCS {
public:
    static void init(const std::string& path);
    static void commit(const std::string& filepath, const std::string& message);
    static std::vector<Commit> log(const std::string& filepath);
    static std::string diff(const std::string& filepath);
    static std::string status(const std::string& filepath);
    static void createBranch(const std::string& filepath, const std::string& name);
    static void checkoutBranch(const std::string& filepath, const std::string& name);
    static std::string checkoutCommit(const std::string& filepath, const std::string& hash);
    static std::vector<std::string> listBranches(const std::string& filepath);
    static std::string currentBranch(const std::string& filepath);
    static void stash(const std::string& filepath);
    static std::string stashPop(const std::string& filepath);

private:
    static std::string repoDir(const std::string& filepath);
    static std::string hashContent(const std::string& content);
    static std::string readFile(const std::string& path);
    static void writeFile(const std::string& path, const std::string& content);
    static std::string currentTimestamp();
};

}
