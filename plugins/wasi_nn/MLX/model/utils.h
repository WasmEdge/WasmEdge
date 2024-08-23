#pragma once
#include <iostream>
#include <string>
#include <vector>
std::vector<std::string> splitString(const std::string &S, char Delim);
std::string joinString(std::vector<std::string> &S, char Delim);
bool endsWith(std::string const &Value, std::string const &Ending);
bool startsWith(std::string const &Value, std::string const &Starting);