#pragma once

#include <cstdlib>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>
#include <stdexcept>

// https://github.com/LLVMParty/args
class ArgumentParser
{
protected:
    void addPositional(const std::string & name, std::string & value, const std::string & help, bool required = false)
    {
        auto fn = [this, &value]()
        {
            value = arg;
        };
        positionalArgs.push_back(Arg{ name, help, required, fn });
    }

    void addString(const std::string & flagname, std::string & value, const std::string & help, bool required = false)
    {
        auto fn = [this, flagname, &value]
        {
            if(arg.substr(0, flagname.length()) == flagname)
            {
                if(arg.length() == flagname.length())
                {
                    // -flagname <value>
                    if(i + 1 >= argc)
                    {
                        throw std::runtime_error("missing value for '" + flagname + "' argument");
                    }
                    value = argv[++i];
                    if(value.empty())
                    {
                        throw std::runtime_error("empty value for '" + flagname + "' argument");
                    }
                    markExtracted(flagname);
                }
                else if(arg[flagname.length()] == '=')
                {
                    // -flagname=<value>
                    value = arg.substr(flagname.length() + 1);
                    markExtracted(flagname);
                }
            }
        };
        flagArgs.push_back(Arg{ flagname, help, required, fn });
    }

    void addBool(const std::string & flagname, bool & value, const std::string & help, bool required = false)
    {
        auto fn = [this, flagname, &value]
        {
            if(arg.substr(0, flagname.length()) == flagname)
            {
                if(arg.length() == flagname.length())
                {
                    // -flagname
                    value = true;
                    markExtracted(flagname);
                }
                else if(arg[flagname.length()] == '=')
                {
                    // -flagname=<value>
                    auto strValue = arg.substr(flagname.length() + 1);
                    if(strValue.empty())
                    {
                        throw std::runtime_error("empty value for '" + flagname + "' argument");
                    }
                    value = strValue == "1" || strValue == "true";
                    markExtracted(flagname);
                }
            }
        };
        flagArgs.push_back(Arg{ flagname, help, required, fn });
    }

    explicit ArgumentParser(std::string description) : description(std::move(description))
    {
    }

public:
    virtual ~ArgumentParser() = default;
    ArgumentParser(const ArgumentParser &) = delete;
    ArgumentParser & operator=(const ArgumentParser &) = delete;
    ArgumentParser(ArgumentParser &&) = delete;
    ArgumentParser & operator=(ArgumentParser &&) = delete;

    void parseOrExit(int argc, char** argv, const char* helpflag = "-help")
    {
        bool help = false;
        addBool(helpflag, help, "Show this message");

        try
        {
            parse(argc, argv);
        }
        catch(const std::exception & e)
        {
            printf("Error: %s\n\nHelp:\n%s\n", e.what(), helpStr().c_str());
            std::exit(help ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        if(help)
        {
            puts(helpStr().c_str());
            std::exit(EXIT_SUCCESS);
        }
    }

    void parse(int argc, char** argv)
    {
        this->argc = argc;
        this->argv = argv;
        bool seenRequired = false;
        for(const auto & positionalArg : positionalArgs)
        {
            if(positionalArg.name.empty())
            {
                throw std::runtime_error("cannot add positional argument without name");
            }
            if(!positionalArg.required)
            {
                if(seenRequired)
                {
                    throw std::runtime_error("cannot add required positional argument after an optional one");
                }
            }
            else
            {
                seenRequired = true;
            }
        }
        for(const auto & flagArg : flagArgs)
        {
            if(flagArg.name.empty())
            {
                throw std::runtime_error("cannot add argument without name");
            }
            if(flagArg.name[0] != '-')
            {
                throw std::runtime_error("invalid argument name '" + flagArg.name + "'");
            }
        }
        size_t positionalIndex = 0;
        for(i = 1; i < argc; i++)
        {
            arg = std::string(argv[i]);
            if(arg.empty())
            {
                continue;
            }
            if(arg[0] == '-')
            {
                didExtract = false;
                for(const auto & flag : flagArgs)
                {
                    flag.fn();
                }
                if(!didExtract)
                {
                    throw std::runtime_error("unknown argument '" + arg + "'");
                }
            }
            else
            {
                if(positionalIndex + 1 > positionalArgs.size())
                {
                    throw std::runtime_error("unexpected positional argument '" + arg + "'");
                }
                const auto & positionalArg = positionalArgs[positionalIndex++];
                if(positionalArg.name[0] == '-')
                {
                    markExtracted(positionalArg.name);
                }
                positionalArg.fn();
            }
        }
        for(const auto & flagArg : flagArgs)
        {
            if(!flagArg.required)
            {
                continue;
            }
            if(flagsExtracted.count(flagArg.name) == 0)
            {
                throw std::runtime_error("required argument '" + flagArg.name + "' missing");
            }
        }
        for(size_t i = positionalIndex; i < positionalArgs.size(); i++)
        {
            const auto & positionalArg = positionalArgs[i];
            if(positionalArg.required)
            {
                if(flagsExtracted.count(positionalArg.name) > 0)
                {
                    continue;
                }
                throw std::runtime_error("required positional argument missing");
            }
        }
    }

    std::string helpStr() const
    {
        std::string help;
        help += "  ";
        help += argv[0];
        help += " {OPTIONS}";

        for(const auto & positionalArg : positionalArgs)
        {
            help += " ";
            if(!positionalArg.required)
            {
                help += '[';
            }
            if(positionalArg.name[0] == '-')
            {
                help += "[" + positionalArg.name + "]";
                help += " <value>";
            }
            else
            {
                help += positionalArg.name;
            }
            if(!positionalArg.required)
            {
                help += ']';
            }
        }
        help += '\n';

        if(!description.empty())
        {
            help += "\n    ";
            help += description;
            help += "\n\n";
        }

        help += "  OPTIONS:\n";

        size_t maxLen = 0;
        for(const auto & flagArg : flagArgs)
        {
            if(flagArg.name.size() > maxLen)
            {
                maxLen = flagArg.name.size();
            }
        }
        for(const auto & flagArg : flagArgs)
        {
            help += "\n    ";
            help += flagArg.name;
            for(size_t i = 0; i < maxLen - flagArg.name.size(); i++)
            {
                help += ' ';
            }
            help += "  ";
            help += flagArg.help;
            if(!flagArg.required)
            {
                help += " (optional)";
            }
        }

        return help;
    }

private:
    struct Arg
    {
        std::string           name;
        std::string           help;
        bool                  required = false;
        std::function<void()> fn;

        Arg() = default;
        Arg(std::string name, std::string help, bool required, std::function<void()> fn)
            : name(std::move(name))
            , help(std::move(help))
            , required(required)
            , fn(std::move(fn))
        {
        }
    };

    std::string      description;
    std::vector<Arg> positionalArgs;
    std::vector<Arg> flagArgs;

    int                             i = 1;
    int                             argc = 0;
    char**                          argv = nullptr;
    bool                            didExtract = false;
    std::string                     arg;
    std::unordered_set<std::string> flagsExtracted;

    void markExtracted(const std::string & flagname)
    {
        didExtract = true;
        if(flagsExtracted.count(flagname) > 0)
        {
            throw std::runtime_error("duplicate value for '" + flagname + "' argument");
        }
        flagsExtracted.insert(flagname);
    }
};
