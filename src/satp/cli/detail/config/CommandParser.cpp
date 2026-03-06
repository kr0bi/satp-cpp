#include "satp/cli/detail/config/CommandParser.h"

#include <sstream>

using namespace std;

namespace satp::cli::config {
    Command parseCommand(const string &line) {
        istringstream iss(line);
        Command cmd;
        iss >> cmd.name;
        for (string token; iss >> token;) {
            cmd.args.push_back(token);
        }
        return cmd;
    }
} // namespace satp::cli::config
