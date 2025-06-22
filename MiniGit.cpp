#include <iostream>
#include <string>
#include "fileUtils.cpp"
#include "commitList.cpp"

using namespace std;

const string MINIGIT_DIR = ".minigit/";
const string OBJECT_DIR = MINIGIT_DIR + "objects/";
const string STAGING_AREA = MINIGIT_DIR + "index";
const string REFS_DIR = MINIGIT_DIR + "refs/";
const string HEAD_DIR = REFS_DIR + "heads/";
const string HEAD_FILE = MINIGIT_DIR + "HEAD";


class MiniGit{
  private:
    CommitList commits;
    string currentCommit;
    string currentBranch;
    //unordered_map<string, string> branches; //branch name -> commit hash

  public:
    bool initialize(){
      if (fileExists(MINIGIT_DIR)){
        cout <<"Minigit repository already exists.\n";
        return true;
      }
    
      if (createDirectory(MINIGIT_DIR) && 
          createDirectory(OBJECT_DIR) && 
          createDirectory(REFS_DIR) &&
          createDirectory(HEAD_DIR) &&
          writeFile(HEAD_DIR + "main", "\n") &&
          writeFile(STAGING_AREA, "") &&
          writeFile(HEAD_FILE, "ref: refs/heads/main\n")){
          
          currentCommit = "";
          //branches["main"] = "";
          currentBranch = "main";
          cout <<"Minigit has been successfully initialized in " <<MINIGIT_DIR <<"\n";
          return true;
          }
      cout <<"Error: failed to initialize Minigit repository.\n";
      return false;
      }
      
    //add files to the repository 
    bool addFiles(const string& filename){
      if (!fileExists(MINIGIT_DIR)) {
        cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
        return false;
      }
      if (!fileExists(filename)) {
        cout <<"Error: Couldn't find " <<filename <<endl;
        return false;
      }
      
      string filecontent = readFile(filename);
      string blobHash = generateHash(filecontent);
      
      createDirectory(OBJECT_DIR + blobHash + "/");
      writeFile(OBJECT_DIR + blobHash + "/" + blobHash, filecontent);
      
      unordered_map<string, string> stagingArea = readStagingArea();
      stagingArea[filename] = blobHash;
      if (!writeStagingArea(stagingArea)){
       cout <<"Error: Couldn't update staging area " <<filename <<endl;
       return false;
      }
      cout <<"Successfully added " <<filename <<" (blob: " <<blobHash.substr(0, 7) <<")\n";
      return true;
    }
    
    //to create snapshots of current file virsion
    bool commit(const string& message){
      if (!fileExists(MINIGIT_DIR)) {
        cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
        return false;
      }
      unordered_map<string, string> stagingArea = readStagingArea();
      if (stagingArea.empty()){
        cout <<"Notting to commit, working tree is clean.\n";
        return false;
      }
      
      string parentHash = getHeadHash();
      
      CommitNode newCommit(message, parentHash);
      newCommit.fileblobs = stagingArea;
      newCommit.computeAndSetHash();

      if(!writeFile(OBJECT_DIR + newCommit.commitHash, commits.commitData(newCommit))){
        cout << "Error: Could not write commit object.\n";
        return false;
      }
      
      if(!updateHead(newCommit.commitHash)){
        cout <<"Error: couldn't update the head.\n";
        return false;
      }
      
      if (!writeFile(STAGING_AREA, "")){
        cout << "Warning: Couldn't clear staging area after commit.\n";
      }
      
      cout << "Committed: " << newCommit.commitHash.substr(0, 7) << " " << newCommit.message << std::endl;
      return true;
    }
    
    //displays all commit from head backwards
    void viewLog(){
      if (!fileExists(MINIGIT_DIR)) {
        cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
        return;
      }
      string currentHash = getHeadHash();
      if (currentHash.empty()){
        cout <<"There are no commits yet.\n";
        return;
      }
      
      while(!currentHash.empty()){
        CommitNode commit = readCommit(currentHash);
        cout << "commitID: " << commit.commitHash << endl;
        cout << "Date & time:   " << commit.timestamp << endl;
        cout << "\t" << commit.message << endl;
        cout << "---------------------------------------------" <<endl;
        currentHash = commit.parent;
      }
    }
/*    
    void branch(){
      if (!fileExists(MINIGIT_DIR)) {
          cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
          return false;
      }
      //
    }
*/  
    bool branching(const string& name) {
        if (!fileExists(MINIGIT_DIR)) {
            cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
            return false;
        }

        string currentHash = getHeadHash();
        if (currentHash.empty()) {
            cout << "Error: No commits to branch " << currentBranch <<". Create a commit first.\n";
            return false;
        }

        string branchPath = HEAD_DIR + name;
        if (fileExists(branchPath)) {
            cout << "Error: Branch '" << name << "' already exists.\n";
            return false;
        }

        if (writeFile(branchPath, currentHash + "\n")) {
            cout << "Created branch '" << name << "' pointing to " << currentHash << endl;
            return true;
        }
        cout << "Error: Could not create branch file for '" << name << "'.\n";
        return false;
    }

    unordered_map<string, string> readStagingArea() {
      unordered_map <string, string> sa;
      string content = readFile(STAGING_AREA);
      stringstream ss(content);
      string line;
      while (getline(ss, line)) {
          size_t spacePos = line.find(' ');
          if (spacePos != string::npos) {
              string filePath = line.substr(0, spacePos);
              string blobHash = line.substr(spacePos + 1);
              sa[filePath] = blobHash;
          }
        }
      return sa;
    }
    
    bool writeStagingArea(const unordered_map<string, string>& stagingArea) {
      stringstream ss;
      for (const auto& entry : stagingArea) {
          ss << entry.first << " " << entry.second << "\n";
      }
      return writeFile(STAGING_AREA, ss.str());
    }
    
    
    string getHeadHash(){
      if (!fileExists(HEAD_FILE)){
        return "";
      }
      string headContent = readFile(HEAD_FILE);
      if (headContent.empty()) return "";
      
      if (headContent.rfind("ref: ", 0) == 0){
        string refPath = headContent.substr(5);
        if (!refPath.empty() && refPath.back() == '\n') {
            refPath.pop_back();
        }
        string branchRef = MINIGIT_DIR + refPath;
        string targethash = readFile(branchRef);
        if (!targethash.empty() && targethash.back() == '\n') {
            targethash.pop_back();
        }
        return targethash;
      }
      return headContent;
    }
    
    CommitNode readCommit(string& currentHash){
      string commitPath = OBJECT_DIR + currentHash;
      string data = readFile(commitPath);
      if (data.empty()) {
        return CommitNode();
    }
    return commits.deserialize(data);
  }
  
  bool updateHead(const string& commitHash) {
    string headContent = readFile(HEAD_FILE);
    if (headContent.rfind("ref: ", 0) == 0) {
        string refPath = headContent.substr(5);
        if (!refPath.empty() && refPath.back() == '\n') {
            refPath.pop_back();
        }
        string branchRef = MINIGIT_DIR + refPath;
        return writeFile(branchRef, commitHash + "\n");
    } else {
        return writeFile(HEAD_FILE, commitHash + "\n");
    }
  }
  
  bool checkOut(const string& target) {
    if (!fileExists(MINIGIT_DIR)) {
        cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
        return false;
    }

    string targetCommitHash;
    string branchPath = HEAD_DIR + target;

    if (fileExists(branchPath)) {
        targetCommitHash = readFile(branchPath);
        if (!targetCommitHash.empty() && targetCommitHash.back() == '\n') {
            targetCommitHash.pop_back();
        }
        if (targetCommitHash.empty()) {
             cout << "Error: Branch '" << target << "' has no commits yet. Cannot switch to it.\n";
             return false;
        }

        if (!writeFile(HEAD_FILE, "ref: refs/heads/" + target + "\n")) {
            cout << "Error: Could not update HEAD to branch " << target << endl;
            return false;
        }
    } else {
        // Corrected path for checking if target is a commit hash
        if (!fileExists(OBJECT_DIR + target + "/" + target)) {
            cout << "Error: Neither branch '" << target << "' nor commit '" << target << "' found.\n";
            return false;
        }
        targetCommitHash = target;
        if (!writeFile(HEAD_FILE, targetCommitHash + "\n")) {
            cout << "Error: Could not update HEAD to commit " << target <<endl;
            return false;
        }
    }

    CommitNode targetCommit = readCommit(targetCommitHash);

    error_code ec;
    for (const auto& entry : filesystem::directory_iterator(".", ec)) {
        std::string currentPath = entry.path().string();
        if (currentPath == MINIGIT_DIR || filesystem::path(currentPath).filename() == "minigit" || filesystem::path(currentPath).filename() == "minigit.exe") continue;

        if (entry.is_directory(ec)) continue;

        string canonicalPath = filesystem::relative(entry.path(), filesystem::current_path()).string();

        if (fileExists(canonicalPath) &&
            targetCommit.fileblobs.find(canonicalPath) == targetCommit.fileblobs.end()) {
            removeFile(canonicalPath);
        }
    }

    for (const auto& entry : targetCommit.fileblobs) {
        const string& filename = entry.first;
        const string& blobHash = entry.second;

        string blobContent = readFile(OBJECT_DIR + blobHash + "/" + blobHash);
        if (blobContent.empty() && !fileExists(OBJECT_DIR + blobHash + "/" + blobHash)) {
            cout << "Warning: Blob " << blobHash << " for file " << filename << " not found. Skipping." <<endl;
            continue;
        }

        if (!writeFile(filename, blobContent)) {
            cout << "Error: Could not restore file " << filename <<endl;
            return false;
        }
    }

    if (!writeFile(STAGING_AREA, "")) {
        cout << "Warning: Could not clear staging area after checkout." <<endl;
    }

    cout << "Switched to '" << target << "' (" << targetCommitHash << ")" <<endl;
    return true;
  }  
};
