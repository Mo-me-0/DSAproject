#include <iostream>
#include <string>
#include "fileUtils.cpp"

using namespace std;

int main(int argc, char* argv[]){
  FileUtils fu;
  bool n = fu.createDirectory("./minigit");
  bool m = fu.fileExists("minigit/m.txt");
  fu.readFile("./example.txt");
  fu.writeFile("./example.txt", "random content");
  string content = fu.readFile("./example.txt");
  cout <<content <<endl;
  return 0;
}
