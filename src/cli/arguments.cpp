// tarik (c) Nikolas Wipper 2020

#include "arguments.h"

#include <iostream>
#include <utility>

unsigned int Option::option_count = 0;

ArgumentParser::ArgumentParser(int argc, const char *argv[], std::string toolName) {
    name = std::move(toolName);
    passed = std::vector<std::string>(argv, argv + argc);
    it = passed.begin();
    it++;

    options.push_back(new Option("help", "Show this message and exits", false, ""));
}

Option *ArgumentParser::add_option(Option option) {
    if (option.name != "help")
        options.push_back(new Option(std::move(option)));
    else
        throw std::runtime_error("Overwrite the help option by overriding the help function");
    return options.back();
}

Option *ArgumentParser::add_option(std::string name_, std::string description_, bool has_arg_, std::string argument_name_, char short_name) {
    Option o(std::move(name_), std::move(description_), has_arg_, std::move(argument_name_), short_name);
    return add_option(std::move(o));
}

void ArgumentParser::help() {
    std::cout << "Usage: " << passed[0] << " [options] inputs\n\n";
    std::cout << "Options:\n";
    for (const auto &option: options) {
        std::cout << "    --" << option->name;
        if (option->has_arg)
            std::cout << "=<" << option->argument_name << ">";
        std::cout << " - " << option->description << "\n";
    }
}

ParsedOption ArgumentParser::parse_next_arg() {
    if (passed.size() == 1) {
        help();
        return {};
    }
    if (it == passed.end()) return {nullptr, ""};
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
            help();
        for (auto &i: options) {
            if (i->name == option) {
                if (i->has_arg) {
                    if (argument.empty()) {
                        std::cerr << i->name << " requires an argument" << std::endl;
                        exit(1);
                    }
                } else if (!argument.empty()) {
                    std::cerr << i->name << " doesn't accept arguments" << std::endl;
                    exit(1);
                }

                return {i, argument};
            }
        }
        std::cerr << "Option ignored: " << raw << "\n";
    } else if (raw.find('-') == 0) {
        --it;

        std::string shorts = raw.substr(1);
        while (!shorts.empty()) {
            char s = shorts[0];
            shorts = shorts.substr(1);
            *it = "-" + shorts;

            for (auto &i: options) {
                if (i->short_name == s) {
                    ParsedOption res;
                    if (i->has_arg) {
                        auto arg = it + 1;
                        if (arg == passed.end()) {
                            exit(1);
                        }
                        res = {i, *arg};
                        passed.erase(arg);
                    } else {
                        res = {i, ""};
                    }
                    return res;
                }
            }
        }
        if (shorts.empty()) {
            it++;
        }
    } else
        inputs.push_back(raw);
    return {};
}

std::vector<std::string> ArgumentParser::get_inputs() {
    for (; this->it != this->passed.end(); it++)
        this->inputs.push_back(*it);
    return this->inputs;
}
