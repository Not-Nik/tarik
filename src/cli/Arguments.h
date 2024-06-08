// tarik (c) Nikolas Wipper 2020-2024

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef TARIK_SRC_CLI_ARGUMENTS_H_
#define TARIK_SRC_CLI_ARGUMENTS_H_

#include <map>
#include <vector>
#include <string>

class Option {
    static unsigned int option_count;

public:
    std::string name, description;
    unsigned int option_id;
    bool has_arg;
    std::string argument_name;
    char short_name;

    Option(std::string name_, std::string description_, bool has_arg_, std::string argument_name_, char short_name_ = 0)
        : name(std::move(name_)),
          description(std::move(description_)),
          option_id(option_count++),
          has_arg(has_arg_),
          argument_name(std::move(argument_name_)),
          short_name(short_name_) {}

    Option(const Option &) = delete;
    Option(Option &&) = default;
};

class ParsedOption {
public:
    Option *option = nullptr;
    std::string argument {};

    bool operator==(const ParsedOption &other) const {
        return option == other.option && argument == other.argument;
    }

    bool operator==(Option &other) const {
        return option == std::addressof(other);
    }

    bool operator==(Option *other) const {
        return option == other;
    }
};

class ArgumentParser {
    std::string name;
    std::vector<std::string> passed;
    std::vector<std::string>::iterator it;

    std::map<std::string, std::vector<Option *>> options;
    std::vector<std::string> inputs;

protected:
    virtual void help();

public:
    class iterator {
        ArgumentParser *parser = nullptr;
        ParsedOption override;
        bool overriden;

        ParsedOption last;

        friend bool operator==(iterator &me, iterator &other);

    public:
        explicit iterator(ArgumentParser *parser_)
            : parser(parser_),
              overriden(false) {}

        explicit iterator(ParsedOption override_)
            : override(std::move(override_)),
              overriden(true) {}

        ParsedOption operator++() {
            if (overriden)
                return override;
            if (!last.option) {
                parser->parse_next_arg();
            }
            return last = parser->parse_next_arg();
        }

        ParsedOption operator*() {
            if (overriden)
                return override;
            if (!last.option) {
                last = parser->parse_next_arg();
            }
            return last;
        }
    };

    ArgumentParser(int argc, const char *argv[], std::string toolName);

    Option *add_option(Option option, const std::string &category);
    Option *add_option(std::string name,
                       std::string category,
                       std::string description,
                       bool has_arg = false,
                       std::string argument_name = "",
                       char short_name = 0);

    ParsedOption parse_next_arg();

    [[nodiscard]] std::vector<std::string> get_inputs();

    iterator begin() {
        return iterator(this);
    }

    [[nodiscard]] iterator end() const {
        return iterator(ParsedOption());
    }
};

inline bool operator==(ArgumentParser::iterator &me, ArgumentParser::iterator &other) {
    if (me.overriden != other.overriden)
        return *me == *other;
    if (me.overriden)
        return me.override == other.override;
    return me.parser == other.parser;
}

#endif //TARIK_SRC_CLI_ARGUMENTS_H_
