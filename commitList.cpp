#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;


//a method that returns current time as a string
string getCurrentTime(){
  time_t t = time(0);
  tm* now = localtime(&t);
  string currentTime = to_string(now->tm_year + 1900) + "/" + to_string(now->tm_mon + 1) + "/" + to_string(now->tm_mday) +
                      "\t" + to_string(now->tm_hour) + ":" + to_string(now->tm_min) + ":" + to_string(now->tm_sec);
  return currentTime;
}

//the hash function to be used for ID's of commits and blobs 
string generateHash(const std::string& data) {
  unsigned long hash = 5381; // djb2 hash constant
  for (char c : data) {
      hash = ((hash << 5) + hash) + static_cast<unsigned char>(c); // hash * 33 + c
  }

  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << hash;
  return ss.str();
}

struct CommitNode {
    string commitHash;
    string timestamp;
    string parent;
    string message;
    unordered_map<string, string> fileblobs; //filename -> blob hash
    
    CommitNode(){
      this -> timestamp = "";
      this -> message = "";
      this -> parent = "";
      this -> commitHash = "";
    }
    
    CommitNode(const string& message, const string& parent) {
      this -> timestamp = getCurrentTime();
      this -> message = message;
      this -> parent = parent;
    }
    
    void computeAndSetHash() {
      string contentToHash = "message:" + message + "\n" +
                                  "timestamp:" + timestamp + "\n" +
                                  "parent:" + parent + "\n" +
                                  "files:";
      bool first = true;
      for (const auto& entry : fileblobs) {
          if (!first) contentToHash += ",";
          contentToHash += entry.first + "=" + entry.second;
          first = false;
      }
      contentToHash += "\n";
      this->commitHash = generateHash(contentToHash);
  }
    
};

class CommitList {
  private:
    CommitNode* head;
    
  public:
    CommitList() : head(nullptr){}

    void addCommit(const string& message, const string& parent) {
        CommitNode* newNode = new CommitNode(message, parent);
        if (!head) {
            head = newNode;
        } else {
            newNode->parent = head->commitHash;
            head = newNode;
      }
    }
    
    
    string commitData(CommitNode& cmt) {
      stringstream ss;
      ss << "commitHash:" <<cmt.commitHash <<"\n";
      ss << "message:" <<cmt.message <<"\n";
      ss << "timestamp:" <<cmt.timestamp <<"\n";
      ss << "parent:" <<cmt.parent <<"\n";
      ss << "files:";
      bool first = true;

      for (const auto& entry : cmt.fileblobs){
        if (!first) ss << ",";
        ss << entry.first <<"=" << entry.second;
        first = false;
      }
      ss <<"\n";
      return ss.str();
    }
    
    CommitNode deserialize(const string& data) {
      CommitNode c;
      stringstream ss(data);
      string line;
      while (getline(ss, line)) {
          size_t colonPos = line.find(':');
          if (colonPos == string::npos) continue;

          string key = line.substr(0, colonPos);
          string value = line.substr(colonPos + 1);

          if (key == "commitHash") c.commitHash = value;
          else if (key == "message") c.message = value;
          else if (key == "timestamp") c.timestamp = value;
          else if (key == "parent") c.parent = value;
          else if (key == "files") {
              stringstream filesSs(value);
              string fileEntry;
              while (getline(filesSs, fileEntry, ',')) {
                  size_t eqPos = fileEntry.find('=');
                  if (eqPos != string::npos) {
                      string filename = fileEntry.substr(0, eqPos);
                      string blobHash = fileEntry.substr(eqPos + 1);
                      c.fileblobs[filename] = blobHash;
                  }
              }
          }
      }
      return c;
    }

    CommitNode* getLatestCommit() const {
        return head;
    }

    ~CommitList() {}
};
