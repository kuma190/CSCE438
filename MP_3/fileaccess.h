#ifndef FILEACCESS_H    // To make sure you don't declare the function more than once by including the header multiple times.
#define FILEACCESS_H

#include <string>
#include <vector>

void addUsertoFile(std::string username,std::string filename);
std::vector<std::string> getUsersFromFile(std::string filename);
#endif