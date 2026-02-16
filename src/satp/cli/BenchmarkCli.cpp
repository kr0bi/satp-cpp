#include "satp/cli/BenchmarkCli.h"

#include <iostream>
#include <string>

#include "satp/cli/CliConfig.h"

namespace satp::cli {
    int BenchmarkCli::run() {
        std::cout << "SATP benchmark CLI. Digita 'help' per i comandi.\n";

        for (std::string line; std::cout << "> " && std::getline(std::cin, line);) {
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
                    std::cout << "Uso: set <param> <value>\n";
                    continue;
                }
                if (!config::setParam(cfg_, cmd.args[0], cmd.args[1])) {
                    std::cout << "Parametro o valore non valido\n";
                }
                continue;
            }
            if (cmd.name == "run") {
                if (cmd.args.empty()) {
                    std::cout << "Uso: run <algo|all>\n";
                    continue;
                }
                executor_.run(cfg_, cmd.args, RunMode::Normal);
                continue;
            }
            if (cmd.name == "runstream") {
                if (cmd.args.empty()) {
                    std::cout << "Uso: runstream <algo|all>\n";
                    continue;
                }
                executor_.run(cfg_, cmd.args, RunMode::Streaming);
                continue;
            }
            if (cmd.name == "runmerge") {
                if (cmd.args.empty()) {
                    std::cout << "Uso: runmerge <algo|all>\n";
                    continue;
                }
                executor_.run(cfg_, cmd.args, RunMode::Merge);
                continue;
            }
            if (cmd.name == "quit") {
                break;
            }

            std::cout << "Comando sconosciuto. Digita 'help'.\n";
        }

        return 0;
    }
} // namespace satp::cli
