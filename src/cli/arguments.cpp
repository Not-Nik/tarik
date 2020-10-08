// Matrix (c) Nikolas Wipper 2020

#include "arguments.h"

#include <iostream>
#include <utility>

ArgumentParser::ArgumentParser(int argc, const char * argv[], std::string toolName) {
    this->name = std::move(toolName);
    this->passed = std::vector<std::string>(argv, argv + argc);
    this->it = this->passed.begin();
    this->it++;

    this->options.push_back({"help", "Show this message and exit", HELP_OPTION, false, ""});
}

void ArgumentParser::addOption(const Option & option) {
    if (option.name != "help")
        this->options.push_back(option);
    else
        throw std::runtime_error("Overwrite the help option by overriding the help function");
}

void ArgumentParser::help() {
    std::cout << this->name << "\n";
    std::cout << "Usage: " << this->passed[0] << " [options] inputs\n\n";
    std::cout << "Options:\n";
    for (const auto & option : this->options) {
        std::cout << "--" << option.name;
        if (option.hasArg)
            std::cout << "=<" << option.argumentName << ">";
        std::cout << " - " << option.description << "\n";
    }
    exit(0);
}

ParsedOption ArgumentParser::parseNextArg() {
    if (this->passed.size() == 1)
        this->help();
    if (this->it == this->passed.end()) return {nullptr, ""};
    std::string raw = *it;
    it++;
    if (raw.find("--") == 0) {
        std::string option = raw.substr(2);
        std::string argument;
        if (option.find('=') != std::string::npos) {
            argument = option.substr(option.find('=') + 1);
            option = option.substr(0, option.find('='));
        }
        if (option == "help")
            this->help();
        for (auto & i : this->options) {
            if (i.name == option) {
                if (i.hasArg) {
                    if (argument.empty()) {
                        std::cerr << i.name << " requires an argument" << std::endl;
                        exit(1);
                    }
                } else {
                    if (!argument.empty()) {
                        std::cerr << i.name << " doesn't accept arguments" << std::endl;
                        exit(1);
                    }
                }
                return {&i, argument};
            }
        }
        std::cerr << "Option ignored: " << raw << "\n";
    } else
        this->inputs.push_back(raw);
    return {};
}
