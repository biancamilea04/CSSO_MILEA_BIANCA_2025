#pragma once
#include <set>
#include <string>
#include <cstdlib>
#include <functional>

//
// functii punctul II b
//

 std::set<int, std::greater<> > getPidValue(const char *input) {
    std::string inputStr(input);
    std::set<int, std::greater<> > pidValue;
    const std::string delimiter = "\n";
    size_t pos = 0;

    while ((pos = inputStr.find(delimiter)) != std::string::npos) {
        std::string token = inputStr.substr(0, pos);

        int pos1 = token.find(": ");
        int pos2 = token.find(",");

        std::string key = token.substr(pos1 + 2, pos2 - pos1 - 2);

        int pid = atoi(key.c_str());
        pidValue.insert(pid);

        inputStr.erase(0, pos + delimiter.length());
    }

    return pidValue;
}

std::set<int> selectPids(std::set<int, std::greater<> > allPids) {
    std::set<int> pids;
    for (int i = 0; i < 5; i++) {
        if (allPids.empty()) {
            break;
        }
        int pid = *allPids.begin();
        pids.insert(pid);
        allPids.erase(allPids.begin());
    }
    return pids;
}


