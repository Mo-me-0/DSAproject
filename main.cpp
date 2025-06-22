#include "MiniGit.cpp"

using namespace std;


void info(){
    cout << "Command and Arguments: \n";
    cout << "./minigit init                               ->   initialize an empty git repository in the current dir\n";
    cout << "./minigit add <'.'or 'file_name(s)'>           ->   add the file(s) to staging area ('.' for all files)\n";
    cout << "./minigit commit -m <'commit message'>       ->   commit your staging files\n";
    cout << "./minigit log                                ->   show commit history\n";
    cout << "./minigit branch <branch_name>               ->   create a new branch\n";
    cout << "./minigit branch <branch>               ->   view branch list\n";
    cout << "./minigit checkout <branch_name_or_commit_hash> ->   switch to a branch or a commit\n";
    cout << "./minigit merge <branch_name>                ->   merge changes from another branch\n";
    cout << "./minigit diff <file1> <file2>               ->   show differences between two files\n";
}


int main(int argc, char* argv[]){
  
  MiniGit git;
  
  if (argc >= 2){
    string command = argv[1];
    if (command == "init"){
      git.initialize();
    } else if (command == "add") {
            if (argc < 3) {
                cout << "missing arguments!" << endl;
                cout << "Provide a file or '.' to add all files in current directory e.g." << endl;
                cout << "./minigit add <file_name> or ./minigit add ." << endl;
            } else {
                string target = string(argv[2]);
                if (target == ".") {
                    error_code ec;
                    for (const auto& entry : filesystem::directory_iterator(".", ec)) {
                        // Ensure the entry exists and is a regular file before attempting to add
                        if (entry.exists(ec) && entry.is_regular_file(ec)) { 
                            string filePath = entry.path().string();
                            // Skip the minigit executable and any files/directories within .minigit/
                            // The MINIGIT_DIR constant is assumed to be accessible from MiniGit.cpp
                            if (filesystem::path(filePath).filename() != "minigit" && 
                                filesystem::path(filePath).filename() != "minigit.exe" &&
                                filePath.rfind(MINIGIT_DIR, 0) != 0) { 
                                git.addFiles(filePath);
                            }
                        }
                    } 
                    if (ec) {
                        cout <<"Error listing files in current directory: " << ec.message() <<endl;
                    }
                } else {
                    for (int i = 2; i < argc; ++i) {
                        git.addFiles(string(argv[i]));
                    }
                }
            }
        } else if (command == "commit") {
            if (argc == 4 && string(argv[2]) == "-m") {
                string message = string(argv[3]);
                git.commit(message);
            } else {
                cout << "missing arguments!\n";
                cout << "Provide with a message field e.g.\n";
                cout << "./minigit commit -m 'my commit message'" << endl;
            }
        } else if (command == "log"){
              git.viewLog();
            } else if (command == "branch") {
            if (argc < 3) {
                git.showBranches();
            } else {
                string name = string(argv[2]);
                git.branching(name);
            }
        } else if (command == "checkout"){
              git.checkOut(argv[2]);
        } else if (command == "merge") {
            if (argc < 3) {
                cout << "missing arguments!" << endl;
                cout << "Provide a branch name to merge from e.g." << endl;
                cout << "./minigit merge <branch_name>" << endl;
            } else {
                git.mergeBranch(argv[2]);
            }
        } else if (command == "diff") {
            if (argc < 4) {
                cout << "missing arguments!" << endl;
                cout << "Provide two file paths e.g." << endl;
                cout << "./minigit diff <file1> <file2>" << endl;
            } else {
                string file1 = string(argv[2]);
                string file2 = string(argv[3]);
                git.diffViewer(file1, file2);
            }
        }else{
          cout <<"Invalid Commmand\n";
          info();
        }
      } else {
          cout<< "Minigit is a custom lightweight version of Git we implemented for this project.\n";
          info();
      }
  
  return 0;
}
