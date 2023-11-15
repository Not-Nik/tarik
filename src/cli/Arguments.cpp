// tarik (c) Nikolas Wipper 2020-2023

#include "Arguments.h"

#include <utility>
#include <sstream>
#include <iostream>

unsigned int Option::option_count = 0;

ArgumentParser::ArgumentParser(int argc, const char *argv[], std::string toolName) {
    name = std::move(toolName);
    passed = std::vector<std::string>(argv, argv + argc);
    it = passed.begin();
    it++;

    options.push_back(new Option("help", "Show this message and exit", false, ""));
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
    auto generate_help_string = [](Option *option) {
        std::stringstream r;
        r << "    --" << option->name;
        if (option->has_arg)
            r << "=<" << option->argument_name << ">";
        if (option->short_name) {
            r << ", -" << option->short_name;
            if (option->has_arg)
                r << " <" << option->argument_name << ">";
        }
        return r.str();
    };
    std::vector<std::pair<std::string, std::string>> help_strings;
    size_t longest = 0;
    for (const auto &option: options) {
        help_strings.emplace_back(generate_help_string(option), option->description);
        if (help_strings.back().first.size() > longest) longest = help_strings.back().first.size();
    }

    for (const auto &hs: help_strings) {
        std::cout << hs.first << std::string(longest - hs.first.size() + 3, ' ') << hs.second << "\n";
    }
}

ParsedOption ArgumentParser::parse_next_arg() {
    std::sort(options.begin(), options.end(), [](Option *first, Option *second) {
        return first->name < second->name;
    });
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
        char short_n = raw.substr(1,1)[0];

        for (auto &i :options) {
            if (i->short_name == short_n) {
                if (i->has_arg) {
                    if (it == passed.end()) {
                        std::cerr << i->name << " requires an argument" << std::endl;
                        exit(1);
                    }
                    std::string arg = *it;
                    it++;
                    return {i, arg};
                }
                return {i, {}};
            }
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
