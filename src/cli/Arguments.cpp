// tarik (c) Nikolas Wipper 2020-2025

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "Arguments.h"

#include <utility>
#include <sstream>
#include <iostream>
#include <algorithm>

unsigned int Option::option_count = 0;

ArgumentParser::ArgumentParser(int argc, const char *argv[], std::string toolName) {
    name = std::move(toolName);
    passed = std::vector<std::string>(argv, argv + argc);
    it = passed.begin();
    it++;

    options["Miscellaneous"].push_back(new Option("help", "Show this message and exit", false, ""));
}

Option *ArgumentParser::add_option(Option option, const std::string &category) {
    if (!options.contains(category))
        options[category] = {};

    if (option.name != "help")
        options[category].push_back(new Option(std::move(option)));
    else
        throw std::runtime_error("Overwrite the help option by overriding the help function");
    return options[category].back();
}

Option *ArgumentParser::add_option(std::string name_,
                                   const std::string &category,
                                   std::string description,
                                   std::string
                                   argument_name,
                                   char short_name) {
    Option o(std::move(name_), std::move(description), true, std::move(argument_name), short_name);
    return add_option(std::move(o), category);
}

Option *ArgumentParser::add_option(std::string name_,
                                   const std::string &category,
                                   std::string description,
                                   char short_name) {
    Option o(std::move(name_), std::move(description), false, "", short_name);
    return add_option(std::move(o), category);
}

void ArgumentParser::help() {
    std::cout << "Usage: " << passed[0] << " [options] inputs\n";
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

    for (auto [category, options] : options) {
        help_strings.emplace_back("\n" + category + ":", "");

        std::sort(options.begin(),
                  options.end(),
                  [](Option *first, Option *second) {
                      return first->name < second->name;
                  });

        for (const auto &option : options) {
            help_strings.emplace_back(generate_help_string(option), option->description);
            if (help_strings.back().first.size() > longest)
                longest = help_strings.back().first.size();
        }
    }

    for (const auto &hs : help_strings) {
        std::cout << hs.first;
        if (!hs.second.empty())
            std::cout << std::string(longest - hs.first.size() + 3, ' ') << hs.second;
        std::cout << "\n";
    }
}

ParsedOption ArgumentParser::parse_next_arg() {
    if (passed.size() == 1) {
        help();
        return {};
    }
    if (it == passed.end())
        return {nullptr, ""};
    std::string raw = *it;
    it++;
    if (raw.find("--") == 0) {
        std::string option = raw.substr(2);
        std::string argument;
        if (option.find('=') != std::string::npos) {
            argument = option.substr(option.find('=') + 1);
            option = option.substr(0, option.find('='));
        }
        if (option == "help") {
            help();
            exit(0);
        }
        for (auto [_, options] : options) {
            for (auto &i : options) {
                if (i->name != option)
                    continue;
                if (i->has_arg && argument.empty()) {
                    std::cerr << i->name << " requires an argument" << std::endl;
                    exit(1);
                } else if (!i->has_arg && !argument.empty()) {
                    std::cerr << i->name << " doesn't accept arguments" << std::endl;
                    exit(1);
                }

                return {i, argument};
            }
        }
        std::cerr << "Option ignored: " << raw << "\n";
    } else if (raw.find('-') == 0) {
        char short_n = raw.substr(1, 1)[0];

        for (auto [_, options] : options) {
            for (auto &i : options) {
                if (i->short_name != short_n)
                    continue;
                std::string arg;
                if (i->has_arg) {
                    if (it == passed.end()) {
                        std::cerr << i->name << " requires an argument" << std::endl;
                        exit(1);
                    }
                    arg = *it++;
                }
                return {i, arg};
            }
        }
    } else
        inputs.push_back(raw);
    return {nullptr, raw};
}

std::vector<std::string> ArgumentParser::get_inputs() {
    for (; this->it != this->passed.end(); it++)
        this->inputs.push_back(*it);
    return this->inputs;
}
