// MiniGit: A Custom Version Control System
// Language: C++
// Features: init, add, commit, log, branch, checkout, merge (now included)

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <ctime>
#include <string>
#include <algorithm>

namespace fs = std::filesystem;

struct Commit {
    std::string hash;
    std::string message;
    std::string timestamp;
    std::string parent;
    std::unordered_map<std::string, std::string> files; // filename -> blob hash
};

std::unordered_map<std::string, Commit> commits; // hash -> commit
std::unordered_map<std::string, std::string> branches; // branch -> commit hash
std::string HEAD = "master";
std::string current_commit = "";
std::unordered_map<std::string, std::string> staging_area; // filename -> blob hash

std::string get_timestamp() {
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return buf;
}

std::string hash_content(const std::string& content) {
    std::hash<std::string> hasher;
    return std::to_string(hasher(content));
}

void init() {
    fs::create_directory(".minigit");
    fs::create_directory(".minigit/objects");
    fs::create_directory(".minigit/refs");
    fs::create_directory(".minigit/refs/heads");
    std::ofstream(".minigit/HEAD") << "master";
    branches["master"] = "";
    current_commit = "";
    std::cout << "Initialized empty MiniGit repository." << std::endl;
}

void save_blob(const std::string& filename) {
    std::ifstream in(filename);
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    std::string blob_hash = hash_content(content);
    std::ofstream(".minigit/objects/" + blob_hash) << content;
    staging_area[filename] = blob_hash;
    std::cout << "Added " << filename << " to staging area." << std::endl;
}

void commit(const std::string& message) {
    if (staging_area.empty()) {
        std::cout << "Nothing to commit." << std::endl;
        return;
    }

    Commit new_commit;
    new_commit.message = message;
    new_commit.timestamp = get_timestamp();
    new_commit.parent = current_commit;

    if (!current_commit.empty())
        new_commit.files = commits[current_commit].files; // inherit files

    for (auto& [file, blob] : staging_area)
        new_commit.files[file] = blob;

    std::string commit_string = message + new_commit.timestamp + new_commit.parent;
    new_commit.hash = hash_content(commit_string);
    commits[new_commit.hash] = new_commit;
    branches[HEAD] = new_commit.hash;
    current_commit = new_commit.hash;
    staging_area.clear();

    std::cout << "Committed as " << new_commit.hash << std::endl;
}

void log() {
    std::string hash = current_commit;
    while (!hash.empty()) {
        Commit c = commits[hash];
        std::cout << "Commit: " << hash << "\nMessage: " << c.message
                  << "\nTimestamp: " << c.timestamp << "\n" << std::endl;
        hash = c.parent;
    }
}

void branch(const std::string& name) {
    branches[name] = current_commit;
    std::cout << "Created branch '" << name << "' at commit " << current_commit << std::endl;
}

void checkout(const std::string& name) {
    if (branches.find(name) != branches.end()) {
        current_commit = branches[name];
        HEAD = name;
    } else if (commits.find(name) != commits.end()) {
        current_commit = name;
        HEAD = "(detached HEAD)";
    } else {
        std::cout << "No such branch or commit." << std::endl;
        return;
    }

    if (!current_commit.empty()) {
        Commit c = commits[current_commit];
        for (auto& [file, blob] : c.files) {
            std::ifstream in(".minigit/objects/" + blob);
            std::ofstream out(file);
            out << in.rdbuf();
        }
    }
    std::cout << "Checked out " << name << std::endl;
}

std::string find_lca(const std::string& a, const std::string& b) {
    std::unordered_set<std::string> ancestors;
    std::string temp = a;
    while (!temp.empty()) {
        ancestors.insert(temp);
        temp = commits[temp].parent;
    }
    temp = b;
    while (!temp.empty()) {
        if (ancestors.count(temp)) return temp;
        temp = commits[temp].parent;
    }
    return "";
}

void merge(const std::string& target_branch) {
    if (branches.find(target_branch) == branches.end()) {
        std::cout << "Branch not found." << std::endl;
        return;
    }
    std::string target_commit = branches[target_branch];
    std::string base = find_lca(current_commit, target_commit);
    if (base.empty()) {
        std::cout << "No common ancestor found. Cannot merge." << std::endl;
        return;
    }

    Commit base_commit = commits[base];
    Commit curr_commit = commits[current_commit];
    Commit tgt_commit = commits[target_commit];

    std::unordered_map<std::string, std::string> result_files = curr_commit.files;
    bool conflict = false;

    for (const auto& [file, tgt_blob] : tgt_commit.files) {
        const auto& base_blob = base_commit.files.find(file);
        const auto& curr_blob = curr_commit.files.find(file);

        if (curr_blob == curr_commit.files.end()) {
            result_files[file] = tgt_blob; // file added in target only
        } else if (base_blob != base_commit.files.end() && base_blob->second != curr_blob->second && base_blob->second != tgt_blob && curr_blob->second != tgt_blob) {
            std::cout << "CONFLICT: both modified " << file << std::endl;
            conflict = true;
        } else {
            result_files[file] = tgt_blob;
        }
    }

    Commit new_commit;
    new_commit.timestamp = get_timestamp();
    new_commit.message = "Merge branch '" + target_branch + "' into " + HEAD + "";
    new_commit.parent = current_commit;
    new_commit.files = result_files;
    new_commit.hash = hash_content(new_commit.message + new_commit.timestamp + new_commit.parent);

    commits[new_commit.hash] = new_commit;
    current_commit = new_commit.hash;
    branches[HEAD] = new_commit.hash;

    std::cout << (conflict ? "Merge completed with conflicts." : "Merge successful.") << std::endl;
}

int main() {
    std::string cmd;
    while (true) {
        std::cout << "MiniGit> ";
        std::getline(std::cin, cmd);

        if (cmd == "exit") break;
        else if (cmd == "init") init();
        else if (cmd.substr(0, 4) == "add ") save_blob(cmd.substr(4));
        else if (cmd.substr(0, 7) == "commit ") commit(cmd.substr(8));
        else if (cmd == "log") log();
        else if (cmd.substr(0, 7) == "branch ") branch(cmd.substr(7));
        else if (cmd.substr(0, 8) == "checkout ") checkout(cmd.substr(8));
        else if (cmd.substr(0, 6) == "merge ") merge(cmd.substr(6));
        else std::cout << "Unknown command." << std::endl;
    }
    return 0;
}
