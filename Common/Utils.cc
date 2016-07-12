//
// Created by kevin on 6/23/16.
//

// Required for getHome
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdexcept>

// Required for I/O
#include <fstream>

// Boost Includes
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/filesystem.hpp>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

// Our includes
#include "Utils.h"

using namespace std;
using namespace DDSMesos::Common;

string Utils::getHome() {
    const char* ptHome = getenv("HOME");
    if (ptHome == nullptr || *ptHome == '\0') {
        // Home environment variable is unset
        const struct passwd *pw = getpwuid(getuid());
        if (pw == nullptr || (ptHome = pw->pw_dir) == nullptr) {
            throw runtime_error("Cannot retrieve Home directory");
        }
    }
    return ptHome;
}

void Utils::setupLogging(const string& logFileName) {
    namespace logging = boost::log;
    namespace keywords = boost::log::keywords;
    string logFilePath;
    try {
        boost::filesystem::path homePath(getHome());
        boost::filesystem::path logFile(logFileName);
        logFilePath = (homePath / logFile).string();
    } catch (const exception *ex) {
        logFilePath = "/tmp/" + logFileName;
    }
    logging::add_file_log(keywords::file_name = logFilePath,
                          keywords::auto_flush = true,
                          keywords::format = "[%TimeStamp%]: %Message%");

}

string Utils::decode64(const string &val) {
    using namespace boost::archive::iterators;
    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
    return boost::algorithm::trim_right_copy_if(std::string(It(std::begin(val)), It(std::end(val))), [](char c) {
        return c == '\0';
    });
}

string Utils::encode64(const string &val) {
    using namespace boost::archive::iterators;
    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
    auto tmp = std::string(It(std::begin(val)), It(std::end(val)));
    return tmp.append((3 - val.size() % 3) % 3, '=');
}

string Utils::readFromFile(const string &fileName) {
    ifstream fStream (fileName);

    // Get the size
    fStream.seekg(0, ios::end);
    size_t size = fStream.tellg();

    // Contiguous storage is guaranteed in C++11
    string buffer (size, ' ');

    // Restore seek
    fStream.seekg(0);

    // write
    fStream.read(&buffer[0], size);

    return buffer;
}

void Utils::writeToFile(const std::string &fileName, const std::string& fileData) {
    ofstream fStream (fileName);
    fStream << fileData;
}