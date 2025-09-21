#ifndef ARG_PARSE_H
#define ARG_PARSE_H

#include <cstdio>
#include <list>
#include <map>
#include <string>

enum ArgumentMode
{
    FLAG,
    STORE,
};

struct ArgumentDescription
{
    const std::string id;
    std::list<std::string> aliases;
    const ArgumentMode mode;
    const std::string description;
    const char *default_value;

    ArgumentDescription(
        std::string const &id,
        std::string const &description,
        ArgumentMode const &mode = ArgumentMode::FLAG,
        const char *default_value = nullptr);
};

struct ParsedArgument
{
    const ArgumentDescription &argument;
    std::string value;
    bool is_set;

    ParsedArgument(ArgumentDescription const &argument, const char *parsed_value);
};

class ArgumentParser
{
  private:
    const std::string name;
    std::list<ArgumentDescription> arguments;
    std::map<const std::string, ArgumentDescription> ids;
    std::map<const std::string, ArgumentDescription> aliases;
    std::map<const std::string, ParsedArgument> parsed_arguments;

  public:
    ArgumentParser(std::string const &program_name);
    void add_argument(
        std::string const &id,
        std::string const &short_name,
        std::string const &long_name,
        std::string const &description,
        ArgumentMode const &mode = ArgumentMode::FLAG,
        const char *default_value = nullptr);

    bool parse(const char **args, const int argc, FILE *errorf);

    ParsedArgument get(std::string const &id);
    void print_usage(FILE *file);
};

#endif
