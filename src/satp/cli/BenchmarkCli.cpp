#include "satp/cli/BenchmarkCli.h"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>

#include "satp/cli/config/CommandParser.h"
#include "satp/cli/config/Printer.h"
#include "satp/cli/config/RunConfigParams.h"

using namespace std;

namespace satp::cli {
    namespace {
        [[nodiscard]] optional<RunMode> runModeByCommand(const string_view commandName) {
            if (commandName == "run") return RunMode::Normal;
            if (commandName == "runstream") return RunMode::Streaming;
            if (commandName == "runmerge") return RunMode::Merge;
            return nullopt;
        }

        [[nodiscard]] const char *runUsageByMode(const RunMode mode) {
            if (mode == RunMode::Streaming) return "Uso: runstream <algo|all>";
            if (mode == RunMode::Merge) return "Uso: runmerge <algo|all>";
            return "Uso: run <algo|all>";
        }
    } // namespace

    int BenchmarkCli::run() {
        cout << "SATP benchmark CLI. Digita 'help' per i comandi.\n";

        for (string line; cout << "> " && getline(cin, line);) {
            const auto cmd = config::parseCommand(line);
            if (cmd.name.empty()) continue;

            if (cmd.name == "help") {
                config::printHelp();
                continue;
            }
            if (cmd.name == "show") {
                config::printConfig(cfg_);
                continue;
            }
            if (cmd.name == "list") {
                config::printAlgorithms();
                continue;
            }
            if (cmd.name == "set") {
                if (cmd.args.size() < 2) {
                    cout << "Uso: set <param> <value>\n";
                    continue;
                }
                if (!config::setParam(cfg_, cmd.args[0], cmd.args[1])) {
                    cout << "Parametro o valore non valido\n";
                }
                continue;
            }
            if (const auto mode = runModeByCommand(cmd.name)) {
                if (cmd.args.empty()) {
                    cout << runUsageByMode(*mode) << '\n';
                    continue;
                }
                executor_.run(cfg_, cmd.args, *mode);
                continue;
            }
            if (cmd.name == "quit") {
                break;
            }

            cout << "Comando sconosciuto. Digita 'help'.\n";
        }

        return 0;
    }
} // namespace satp::cli
