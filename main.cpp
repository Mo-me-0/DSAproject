//#include <iostream>
//#include <string>
//#include <filesystem>
//#include "fileUtils.cpp"
//#include "commitList.cpp"
#include "MiniGit.cpp"

using namespace std;

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
                cout << "missing arguments!\n";
                cout << "Provide a branch name e.g.\n";
                cout << "./minigit branch <branch_name>\n";
            } else {
                string name = string(argv[2]);
                git.branching(name);
            }
        } else if (command == "checkout"){
              git.checkOut(argv[2]);
        } else{
          cout <<"Invalid Commmand\n";
        }
      } else {
          cout<< "Minigit is a custom lightweight version of Git we implemented for this project.\n" 
          << "Use the following commands:\n"
          <<"\t./Minigit init -> to initialize the repository.\n";
      }
  
  return 0;
}
