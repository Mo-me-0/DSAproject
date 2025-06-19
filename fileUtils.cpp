#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

using namespace std;

//this class handls the necessary tools needed to work with files
class FileUtils{
  public:
    //create file directory
    static bool createDirectory(const string& path){
      error_code error;//to capture any error 
      if (fileExists(path)){
        return true;// if file already exists
        }
      //if file doesn't exist it create the directory
      else if (filesystem::create_directories(path, error)){
              return true;//shows creating directory wasn's succesful
            } else {
                // if there is any error occur, it displays the error
                cout <<"Error creating directory " <<path <<": " << error.message() <<endl;
                return false;//shows creating directory wasn't successful
              }   
      }
      
    //check if the file is empty
    static bool fileExists(const string& path){
      return filesystem::exists(path); 
      }
      
    //read the contents of selected file
    static string readFile(const string& path){
      ifstream file(path);// opens the target file
      if (!file.is_open()){
      //check if file is open/available and return empty string and displays the errormessage
        cout << "Error: Could not open file for reading: " << path << std::endl;
        return "";
      }
      stringstream buffer;//creates string object as input/output object
      buffer <<file.rdbuf();//retrieves the related stream buffer from the opened file
      string content = buffer.str();//conversion of string data from a memory buffer
      file.close();//closes the file
      return content;//return the file content
    }

    //creates a file with the provided content or changes the content of existing file
    static bool writeFile(const string& path, const string& content){
      ofstream file(path);//opens the file
      if (!file.is_open()){
        //displays the error message if file couldn't be opened
        cout <<"Error: Could not open file for writing: " <<path <<endl;
        return false;
      }
      file <<content;//replaces the content to the file
      file.close();//closes the file
      return true;//approves that the changes are made
    }
  
};

