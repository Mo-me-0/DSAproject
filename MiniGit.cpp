#include <iostream>
#include <string>
#include <set>
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
    
    // Displays all local branches
    void showBranches() {
      if (!fileExists(MINIGIT_DIR)) {
        cout << "Error: initialize Minigit first\n" << "run ./minigit init\n";
        return;
      }

      string headRefContent = readFile(HEAD_FILE);
      string currentBranchName = "";
      // Extract current branch name from HEAD_FILE
      if (headRefContent.rfind("ref: refs/heads/", 0) == 0){
        currentBranchName = headRefContent.substr(string("ref: refs/heads/").length());
        // Remove trailing newline character if present
        if (!currentBranchName.empty() && currentBranchName.back() == '\n') {
          currentBranchName.pop_back();
        }
      } // If HEAD is detached, currentBranchName will remain empty, which is fine for display

      error_code ec;
      // Check if the directory exists and is actually a directory
      if (!filesystem::exists(HEAD_DIR, ec) || !filesystem::is_directory(HEAD_DIR, ec)) {
          cout << "Error: Branches directory not found or not a directory: " << HEAD_DIR << endl;
          return;
      }

      cout << "Branches:\n";
      // Iterate through the files (branches) in the HEAD_DIR
      for (const auto& entry : filesystem::directory_iterator(HEAD_DIR, ec)) {
          if (entry.is_regular_file(ec)) { // Ensure it's a file and not a subdirectory
              string branchName = entry.path().filename().string();
              string branchPrefix = (branchName == currentBranchName) ? "\t*" : "\t"; // Mark current branch
              cout << branchPrefix << branchName << endl;
          }
      }
      if (ec) { // Check for errors during directory iteration
          cout << "Error listing branches: " << ec.message() << endl;
      }
    }
    
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
  
  string getFileContentFromCommit(const CommitNode& commit, const string& filename) {
    auto it = commit.fileblobs.find(filename);
    if (it != commit.fileblobs.end()) {
        return readFile(OBJECT_DIR + it->second);
    }
    return "";
  }
  
  void writeBlob(const string& content, const string& blobHash) {
    writeFile(OBJECT_DIR + blobHash, content);
  }
  
  string findLCA(const string& commitHash1, const string& commitHash2) {
    set<string> path1;
    string current = commitHash1;
    while (!current.empty()) {
        path1.insert(current);
        CommitNode c = readCommit(current);
        current = c.parent;
    }

    current = commitHash2;
    while (!current.empty()) {
        if (path1.count(current)) {
            return current;
        }
        CommitNode c = readCommit(current);
        current = c.parent;
    }
    return "";
  }
  
  bool mergeBranch(const string& name) {
    if (!fileExists(MINIGIT_DIR)) {
        cout <<"Error: initialize Minigit first\n" << "run ./minigit init\n";
        return false;
    }

    string currentBranchCommitHash = getHeadHash();
    string targetBranchPath = HEAD_DIR + name;

    if (!fileExists(targetBranchPath)) {
        cout << "Error: Branch '" << name << "' does not exist.\n";
        return false;
    }

    string targetBranchCommitHash = readFile(targetBranchPath);
    if (!targetBranchCommitHash.empty() && targetBranchCommitHash.back() == '\n') {
        targetBranchCommitHash.pop_back();
    }

    if (currentBranchCommitHash.empty() || targetBranchCommitHash.empty()) {
        cout << "Error: One of the branches has no commits to merge.\n";
        return false;
    }

    if (currentBranchCommitHash == targetBranchCommitHash) {
        cout << "Already up to date.\n";
        return true;
    }

    string lcaHash = findLCA(currentBranchCommitHash, targetBranchCommitHash);
    if (lcaHash.empty()) {
        cout << "Error: Could not find a common ancestor for merge.\n";
        return false;
    }

    CommitNode lcaCommit = readCommit(lcaHash);
    CommitNode currentCommit = readCommit(currentBranchCommitHash);
    CommitNode targetCommit = readCommit(targetBranchCommitHash);

    unordered_map<string, string> mergedFileBlobs = currentCommit.fileblobs;
    bool conflictDetected = false;
    error_code ec;

    set<string> allFiles;
    for (const auto& entry : lcaCommit.fileblobs) allFiles.insert(entry.first);
    for (const auto& entry : currentCommit.fileblobs) allFiles.insert(entry.first);
    for (const auto& entry : targetCommit.fileblobs) allFiles.insert(entry.first);

    for (const string& filename : allFiles) {
        string lcaContent = getFileContentFromCommit(lcaCommit, filename);
        string currentContent = getFileContentFromCommit(currentCommit, filename);
        string targetContent = getFileContentFromCommit(targetCommit, filename);

        bool inLCA = lcaCommit.fileblobs.count(filename);
        bool inCurrent = currentCommit.fileblobs.count(filename);
        bool inTarget = targetCommit.fileblobs.count(filename);

        if (inCurrent && inTarget) {
            if (currentContent == targetContent) {
                string newBlobHash = generateHash(currentContent);
                writeBlob(currentContent, newBlobHash);
                mergedFileBlobs[filename] = newBlobHash;
                writeFile(filename, currentContent);
            } else if (currentContent == lcaContent) {
                string newBlobHash = generateHash(targetContent);
                writeBlob(targetContent, newBlobHash);
                mergedFileBlobs[filename] = newBlobHash;
                writeFile(filename, targetContent);
            } else if (targetContent == lcaContent) {
                string newBlobHash = generateHash(currentContent);
                writeBlob(currentContent, newBlobHash);
                mergedFileBlobs[filename] = newBlobHash;
                writeFile(filename, currentContent);
            } else {
                conflictDetected = true;
                cout << "CONFLICT: both modified " << filename << endl;
                string conflictContent = "<<<<<<< HEAD\n" + currentContent +
                                              "=======\n" + targetContent +
                                              ">>>>>>> " + name + "\n";
                string conflictBlobHash = generateHash(conflictContent);
                writeBlob(conflictContent, conflictBlobHash);
                writeFile(filename, conflictContent);
                mergedFileBlobs[filename] = conflictBlobHash;
            }
        } else if (inCurrent && !inTarget) {
            if (inLCA && lcaContent == currentContent) {
                mergedFileBlobs.erase(filename);
                removeFile(filename);
            } else {
                string newBlobHash = generateHash(currentContent);
                writeBlob(currentContent, newBlobHash);
                mergedFileBlobs[filename] = newBlobHash;
                writeFile(filename, currentContent);
            }
        } else if (!inCurrent && inTarget) {
            if (inLCA && lcaContent == targetContent) {
                mergedFileBlobs.erase(filename);
                removeFile(filename);
            } else {
                string newBlobHash = generateHash(targetContent);
                writeBlob(targetContent, newBlobHash);
                mergedFileBlobs[filename] = newBlobHash;
                writeFile(filename, targetContent);
            }
        } else if (!inLCA && inCurrent && !inTarget) {
            string newBlobHash = generateHash(currentContent);
            writeBlob(currentContent, newBlobHash);
            mergedFileBlobs[filename] = newBlobHash;
            writeFile(filename, currentContent);
        } else if (!inLCA && !inCurrent && inTarget) {
            string newBlobHash = generateHash(targetContent);
            writeBlob(targetContent, newBlobHash);
            mergedFileBlobs[filename] = newBlobHash;
            writeFile(filename, targetContent);
        }
    }

    if (conflictDetected) {
        cout << "Automatic merge failed; fix conflicts in working directory, then 'minigit add .' and 'minigit commit -m \"Merge...\"'.\n";
    } else {
        cout << "Merge successful.\n" ;

        unordered_map<string, string> newStagingArea;
        for (const auto& entry : mergedFileBlobs) {
            string content = readFile(entry.first);
            string newBlobHash = generateHash(content);
            writeBlob(content, newBlobHash);
            newStagingArea[entry.first] = newBlobHash;
        }
        writeStagingArea(newStagingArea);

        if (!conflictDetected) {
            string msg = "Merge branch '" + name + "' into " + getHeadHash();
            commit(msg);
        }
    }
    return true;
  }
  
  void diffViewer(const string& f1, const string& f2) {
    ifstream a(f1), b(f2);
    if (!a.is_open() || !b.is_open()) {
        cout << "Error: Could not open one or both files for diff: " << f1 << ", " << f2 << endl;
        return;
    }

    string la, lb;
    int line = 1;
    bool hasDiff = false;
    while (true) {
        bool readA = static_cast<bool>(getline(a, la));
        bool readB = static_cast<bool>(getline(b, lb));

        if (!readA && !readB) break;

        if (readA && readB) {
            if (la != lb) {
                cout << "Line " << line << ":\n";
                cout << "=> " << la << endl;
                cout << "=> " << lb << endl;
                hasDiff = true;
            }
        } else if (readA) {
            cout << "Line " << line << ":\n";
            cout << "=> " << la << endl;
            hasDiff = true;
        } else if (readB) {
            cout << "Line " << line << ":\n";
            cout << "=> " << lb << endl;
            hasDiff = true;
        }
        line++;
    }
    if (!hasDiff) {
        cout << "Files are identical.\n";
    }
  }
  
};
