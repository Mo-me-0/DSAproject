#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

struct CommitNode {
    std::string hash;
    std::string message;
    std::string timestamp;
    std::string parent;
    std::unordered_map<std::string, std::string> files;
    CommitNode* next;

    CommitNode(std::string h, std::string m, std::string t, std::string p,
               const std::unordered_map<std::string, std::string>& f)
        : hash(h), message(m), timestamp(t), parent(p), files(f), next(nullptr) {}
};

class CommitList {
private:
    CommitNode* head;
    CommitNode* tail;

public:
    CommitList() : head(nullptr), tail(nullptr) {}

    void addCommit(const std::string& hash, const std::string& message,
                   const std::string& timestamp, const std::string& parent,
                   const std::unordered_map<std::string, std::string>& files) {
        CommitNode* newNode = new CommitNode(hash, message, timestamp, parent, files);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    void logCommits() const {
        CommitNode* current = head;
        std::vector<CommitNode*> stack;
        while (current) {
            stack.push_back(current);
            current = current->next;
        }
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            std::cout << "Commit: " << (*it)->hash << "\n";
            std::cout << "Message: " << (*it)->message << "\n";
            std::cout << "Timestamp: " << (*it)->timestamp << "\n\n";
        }
    }

    CommitNode* getLatestCommit() const {
        return tail;
    }

    ~CommitList() {
        while (head) {
            CommitNode* temp = head;
            head = head->next;
            delete temp;
        }
    }
};
