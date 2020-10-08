// Matrix (c) Nikolas Wipper 2020

#ifndef SENO_TOOLS_H
#define SENO_TOOLS_H

#include <map>
#include <vector>
#include <string>

#define HELP_OPTION 0

class Option
{
public:
    std::string name, description;
    int optionID;
    bool hasArg;
    std::string argumentName;
};

class ParsedOption
{
public:
    Option * option;
    std::string argument;
};

class ArgumentParser
{
    std::string name;
    std::vector<std::string> passed;
    std::vector<std::string>::iterator it;

    std::vector<Option> options;
    std::vector<std::string> inputs;
protected:
    virtual void help();
public:
    ArgumentParser(int argc, const char * argv[], std::string toolName);

    void addOption(const Option& option);

    ParsedOption parseNextArg();

    std::vector<std::string> getInputs() { return this->inputs; }
};

#endif //SENO_TOOLS_H
