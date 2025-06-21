
#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <string>

namespace FileUtils {
    bool createDir(const std::string& path);
    bool exists(const std::string& path);
    void writeFile(const std::string& path, const std::string& content, bool append = false);
    std::string readFile(const std::string& path);
}

#endif // FILEUTILS_H
