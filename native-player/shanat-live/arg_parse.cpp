#include "arg_parse.h"

#include <cstdio>
#include <stdexcept>

ArgumentDescription::ArgumentDescription(
    std::string const &id,
    std::string const &description,
    ArgumentMode const &mode,
    const char *default_value)
    : id(id)
    , mode(mode)
    , description(description)
    , default_value(default_value)
{
}

ParsedArgument::ParsedArgument(
    ArgumentDescription const &argument,
    const char *parsed_value)
    : argument(argument)
{
    if (parsed_value != nullptr)
    {
        is_set = true;
        value = parsed_value;
    }
    else
    {
        is_set = false;
        const char *default_value = argument.default_value;
        if (default_value != nullptr) value = default_value;
        else value = "";
    }
}

ArgumentParser::ArgumentParser(
    std::string const &program_name)
    : name(program_name)
    , aliases()
    , arguments()
    , parsed_arguments()
{
}

void ArgumentParser::add_argument(
    std::string const &id,
    std::string const &short_name,
    std::string const &long_name,
    std::string const &description,
    ArgumentMode const &mode,
    const char *default_value)
{
    if (!short_name.empty() && aliases.find(short_name) != aliases.end())
        throw std::runtime_error("Argument short alias " + short_name + " is already used.");
    else if (!long_name.empty() && aliases.find(long_name) != aliases.end())
        throw std::runtime_error("Argument long alias " + long_name + " is already used.");
    else if (short_name.empty() && long_name.empty())
        throw std::runtime_error("At least one alias must be given.");

    ArgumentDescription descr(id, description, mode, default_value);
    if (!short_name.empty())
    {
        descr.aliases.push_back(short_name);
        aliases.insert(std::make_pair(short_name, descr));
    }

    if (!long_name.empty())
    {
        descr.aliases.push_back(long_name);
        aliases.insert(std::make_pair(long_name, descr));
    }

    arguments.push_back(descr);
    ids.insert(std::make_pair(id, descr));
}

bool ArgumentParser::parse(const char **argv, const int argc, FILE *errorf)
{
    bool ok = true;
    for (int i = 1; i < argc; i++)
    {
        const char *arg_name = argv[i];

        auto it = aliases.find(arg_name);
        if (it == aliases.end())
        {
            if (errorf != nullptr) fprintf(errorf, "Unknown argument: %s\n", arg_name);
            ok = false;
            continue;
        }

        // Get the argument description
        ArgumentDescription &arg = it->second;

        // Check if we need to parse the rest
        char const *value = nullptr;
        switch (arg.mode)
        {
        case FLAG:
            // Give a valid value for the flag
            value = arg_name;
            break;
        case STORE:
            if (i < argc) value = argv[++i];
            break;
        }

        ParsedArgument parsed(arg, value);
        parsed_arguments.insert(std::make_pair(arg.id, parsed));
    }

    return ok;
}

ParsedArgument ArgumentParser::get(std::string const &id)
{
    auto parsed_it = parsed_arguments.find(id);
    if (parsed_it != parsed_arguments.end()) return parsed_it->second;

    // Argument wasn't parsed, check if it is known
    auto argument_id_it = ids.find(id);
    if (argument_id_it == ids.end())
        throw std::runtime_error("Unknown argument ID: " + id);
    // Known: return the argument with its default value
    return ParsedArgument(argument_id_it->second, nullptr);
}

void ArgumentParser::print_usage(FILE *file)
{
    fprintf(file, "Usage of %s\n", name.c_str());
    for (auto it = arguments.begin(); it != arguments.end(); it++)
    {
        ArgumentDescription &arg = *it;
        auto aliases = arg.aliases;
        size_t nb_aliases = aliases.size();
        int i = 0;
        for (auto it = aliases.begin(); it != aliases.end(); ++it)
        {
            // Print the alias
            fprintf(file, "%s", it->c_str());
            if (++i != nb_aliases) fprintf(file, ", ");
        }
        fprintf(file, "\t%s\n", arg.description.c_str());
    }
    fprintf(file, "\n");
}
