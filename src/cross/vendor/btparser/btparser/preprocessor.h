#pragma once

#include <string>
#include <unordered_map>

std::string preprocess(const std::string& input, std::string& error, const std::unordered_map<std::string, std::string>& definitions);