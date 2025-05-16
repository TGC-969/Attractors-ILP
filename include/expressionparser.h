#ifndef EXPRESSIONPARSER_H
#define EXPRESSIONPARSER_H

#include "Node.h"
#include <vector>
#include <string>
#include <unordered_map>

bool parseExpressions(
    const std::string& filename,
    std::unordered_map<std::string, std::shared_ptr<Node>>& nodes,
    std::unordered_map<std::string, int>& nameToId
);

#endif
