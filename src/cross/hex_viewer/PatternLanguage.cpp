#include "PatternLanguage.h"

#include <string>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <wolv/utils/string.hpp>

#include <pl/pattern_language.hpp>
#include <pl/pattern_visitor.hpp>
#include <pl/patterns/pattern.hpp>
#include <pl/patterns/pattern_array_dynamic.hpp>
#include <pl/patterns/pattern_array_static.hpp>
#include <pl/patterns/pattern_bitfield.hpp>
#include <pl/patterns/pattern_boolean.hpp>
#include <pl/patterns/pattern_character.hpp>
#include <pl/patterns/pattern_enum.hpp>
#include <pl/patterns/pattern_float.hpp>
#include <pl/patterns/pattern_padding.hpp>
#include <pl/patterns/pattern_pointer.hpp>
#include <pl/patterns/pattern_signed.hpp>
#include <pl/patterns/pattern_string.hpp>
#include <pl/patterns/pattern_struct.hpp>
#include <pl/patterns/pattern_union.hpp>
#include <pl/patterns/pattern_unsigned.hpp>
#include <pl/patterns/pattern_wide_character.hpp>
#include <pl/patterns/pattern_wide_string.hpp>
#include <pl/patterns/pattern_error.hpp>

using namespace pl;

class ApiPatternVisitor : public PatternVisitor
{
public:
    explicit ApiPatternVisitor(const PatternRunArgs* visitor)
        : m_args(visitor)
    {
    }

    void visit(ptrn::PatternBitfieldField & pattern) override
    {
        formatValue(&pattern);
    }

    void visit(ptrn::PatternBoolean & pattern) override
    {
        formatValue(&pattern);
    }

    void visit(ptrn::PatternFloat & pattern) override
    {
        formatValue(&pattern);
    }

    void visit(ptrn::PatternSigned & pattern) override
    {
        formatValue(&pattern);
    }

    void visit(ptrn::PatternUnsigned & pattern) override
    {
        formatValue(&pattern);
    }

    void visit(ptrn::PatternCharacter & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternEnum & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternString & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternWideCharacter & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternWideString & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::Pattern & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternPadding & pattern) override
    {
        // TODO: add padding support?
        wolv::util::unused(pattern);
    }

    void visit(ptrn::PatternStruct & pattern) override
    {
        formatObject(&pattern);
    }

    void visit(ptrn::PatternUnion & pattern) override
    {
        formatObject(&pattern);
    }

    void visit(ptrn::PatternBitfield & pattern) override
    {
        formatObject(&pattern);
    }

    void visit(ptrn::PatternArrayDynamic & pattern) override
    {
        formatArray(&pattern);
    }

    void visit(ptrn::PatternArrayStatic & pattern) override
    {
        formatArray(&pattern);
    }

    void visit(ptrn::PatternBitfieldArray & pattern) override
    {
        formatArray(&pattern);
    }

    void visit(ptrn::PatternError & pattern) override
    {
        formatString(&pattern);
    }

    void visit(ptrn::PatternPointer & pattern) override
    {
        formatPointer(&pattern);
    }

    void treePush(TreeNode node)
    {
        m_treeStack.push_back(node);
    }

    void treePop()
    {
        m_treeStack.pop_back();
    }

    TreeNode treeTop()
    {
        return m_treeStack.empty() ? m_args->root : m_treeStack.back();
    }

private:
    static std::string debugPattern(const char* title, const ptrn::Pattern* pattern)
    {
        std::string p;
        p += ::fmt::format("{}(\n", title);
        p += ::fmt::format("  offset = {:#x}\n", pattern->getOffset());
        p += ::fmt::format("  size = {:#x}\n", pattern->getSize());
        p += ::fmt::format("  variableName = {}\n", pattern->getVariableName()); // name of the variable (or var[array_idx])
        p += ::fmt::format("  displayName = {}\n",
                           pattern->getDisplayName()); // [[name("Whatever")]] (falls back to variable name)
        p += ::fmt::format("  formattedName = {}\n",
                           pattern->getFormattedName()); // formatted type name (falls back to type name)
        p += ::fmt::format("  typeName = {}\n", pattern->getTypeName()); // name of the type (without struct or array size)
        p += ::fmt::format("  line = {}\n", pattern->getLine()); // source line
        p += ::fmt::format("  visibility = {}\n", (int) pattern->getVisibility());
        //p += format("  endian = {}\n", (int) pattern->getEndian());
        //p += format("  section = {}\n", pattern->getSection());
        p += ::fmt::format("  comment = {}\n", pattern->getComment());
        p += ::fmt::format("  color = {:#x}\n", pattern->getColor());
        p += ::fmt::format("  sealed = {}\n", pattern->isSealed());
        p += ::fmt::format(")");
        return p;
    }

    TreeNode apiVisit(const char* reason, const ptrn::Pattern* pattern, const std::string & value)
    {
        // TODO: array elements,
        auto formattedTypeName = pattern->getFormattedName();
        auto displayName = pattern->getDisplayName();
        auto comment = pattern->getComment();
        VisitInfo info =
        {
            .type_name = formattedTypeName.c_str(),
            .variable_name = displayName.c_str(),
            .offset = pattern->getOffset(),
            .size = pattern->getSize(),
            .line = pattern->getLine(),
            .color = pattern->getColor(),
            .value = value.c_str(),
            .comment = comment.c_str(),
            .reason = reason,
            .big_endian = pattern->getEndian() == std::endian::big,
        };
        return m_args->visit(m_args->userdata, treeTop(), &info);
    }

    static std::string readFormatter(const ptrn::Pattern* pattern)
    {
        if(const auto & functionName = pattern->getReadFormatterFunction(); !functionName.empty())
        {
            return pattern->toString();
        }
        else
        {
            return {};
        }
    }

    void formatValue(const ptrn::Pattern* pattern)
    {
        if(pattern->getVisibility() == ptrn::Visibility::Hidden) return;
        if(pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

        auto value = readFormatter(pattern);
        if(value.empty())
        {
            // TODO: sealed support?

            // char, bool, u128, i128, double, std::string, std::shared_ptr<ptrn::Pattern>
            value = std::visit(wolv::util::overloaded
            {
                // TODO: print full-width
                [&](std::integral auto value) -> std::string
                { return ::fmt::format("0x{:0{}X}\t{}", value, pattern->getSize() * 2, value); },
                [&](std::floating_point auto value) -> std::string
                { return ::fmt::format("{}", value); },
                [&](const std::string & value) -> std::string
                { return ::fmt::format("\"{}\"", value); },
                [&](bool value) -> std::string
                { return value ? "true" : "false"; },
                [&](char value) -> std::string
                { return ::fmt::format("'{}'", value); },
                [&](const std::shared_ptr<ptrn::Pattern> & value) -> std::string
                { return ::fmt::format("\"{}\"", value->toString()); },
                [&](const u128 & value) -> std::string
                {return ::fmt::format("{}", value); },
                [&](const i128 & value) -> std::string
                {return ::fmt::format("{}", value); },
            }, pattern->getValue());
        }

        apiVisit("formatValue", pattern, value);
    }

    void formatString(const ptrn::Pattern* pattern)
    {
        if(pattern->getVisibility() == ptrn::Visibility::Hidden) return;
        if(pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

        apiVisit("formatString", pattern, pattern->toString());
    }

    template<typename T>
    void formatObject(T* pattern)
    {
        if(pattern->getVisibility() == ptrn::Visibility::Hidden) return;
        if(pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

        auto objectNode = apiVisit("formatObject", pattern, readFormatter(pattern));
        if(!pattern->isSealed())
        {
            treePush(objectNode);
            pattern->forEachEntry(0, pattern->getEntryCount(), [&](u64, auto member)
            {
                member->accept(*this);
            });
            treePop();
        }
    }

    template<typename T>
    void formatArray(T* pattern)
    {
        if(pattern->getVisibility() == ptrn::Visibility::Hidden) return;
        if(pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

        auto arrayNode = apiVisit("formatArray", pattern, readFormatter(pattern));
        if(!pattern->isSealed())
        {
            treePush(arrayNode);
            pattern->forEachEntry(0, pattern->getEntryCount(), [&](u64, auto member)
            {
                member->accept(*this);
            });
            treePop();
        }
    }

    void formatPointer(ptrn::PatternPointer* pattern)
    {
        if(pattern->getVisibility() == ptrn::Visibility::Hidden) return;
        if(pattern->getVisibility() == ptrn::Visibility::TreeHidden) return;

        auto pointerNode = apiVisit("formatPointer", pattern, readFormatter(pattern));
        if(!pattern->isSealed())
        {
            treePush(pointerNode);
            pattern->getPointedAtPattern()->accept(*this);
            treePop();
        }
    }

private:
    const PatternRunArgs* m_args = nullptr;
    std::vector<TreeNode> m_treeStack;
};

PatternStatus PatternRun(const PatternRunArgs* args)
{
    // Create and configure Pattern Language runtime
    PatternLanguage runtime;

    auto logCallback = [args](auto level, const auto & message)
    {
        std::string stripped = message;
        while(!stripped.empty() && std::isspace(stripped.back()))
        {
            stripped.pop_back();
        }
        if(args->log_handler != nullptr)
        {
            args->log_handler(args->userdata, (LogLevel)(int) level, stripped.c_str());
        }
        else
        {
            switch(level)
            {
                using
                enum core::LogConsole::Level;

            case Debug:
                ::fmt::print("[DEBUG] {}\n", stripped);
                break;
            case Info:
                ::fmt::print("[INFO]  {}\n", stripped);
                break;
            case Warning:
                ::fmt::print("[WARN]  {}\n", stripped);
                break;
            case Error:
                ::fmt::print("[ERROR] {}\n", stripped);
                break;
            }
        }
    };
    runtime.setLogCallback(logCallback);

    std::vector<std::fs::path> includePaths(args->includes_count);
    for(size_t i = 0; i < includePaths.size(); i++)
    {
#ifdef _WIN32
        includePaths[i] = *wolv::util::utf8ToWstring(args->includes_data[i]);
#else
        includePaths[i] = args->includes_data[i];
#endif
    }
    runtime.setIncludePaths(includePaths);

    for(size_t i = 0; i < args->defines_count; i++)
    {
        runtime.addDefine(args->defines_data[i]);
    }

    auto dangerCallback = [args]()
    {
        return args->allow_dangerous_functions;
    };
    runtime.setDangerousFunctionCallHandler(dangerCallback);
    auto dataCallback = [args](u64 address, void* buffer, size_t size)
    {
        if(!args->data_source(args->userdata, address, buffer, size))
        {
            core::err::E0011.throwError(fmt::format("Failed to read address 0x{:X}[0x{:X}].", address, size));
        }
    };
    runtime.setDataSource(args->base, args->size, dataCallback);

    // Execute pattern file
    auto filename = args->filename;
    if(filename == nullptr)
    {
        filename = api::Source::DefaultSource;
    }
    if(!runtime.executeString(args->source, filename))
    {
        auto compileErrors = runtime.getCompileErrors();
        if(!compileErrors.empty())
        {
            if(args->compile_error == nullptr)
            {
                fmt::print("Compilation failed\n");
            }

            for(const auto & error : compileErrors)
            {
                auto pretty = error.format();
                if(args->compile_error == nullptr)
                {
                    fmt::print("{}\n", pretty);
                }
                else
                {
                    const auto & location = error.getLocation();
                    CompileError cerror =
                    {
                        .location = {
                            .file = location.source->source.c_str(),
                            .line = location.line,
                            .column = location.column,
                            .length = location.length,
                        },
                        .message = error.getMessage().c_str(),
                        .description = error.getMessage().c_str(),
                        .pretty = pretty.c_str(),
                    };
                    args->compile_error(args->userdata, &cerror);
                }
            }
            return PatternCompileError;
        }
        else
        {
            auto error = runtime.getEvalError().value();
            if(args->eval_error == nullptr)
            {
                fmt::print("Pattern Error: {}:{} -> {}\n", error.line, error.column, error.message);
            }
            else
            {
                EvalError cerror =
                {
                    .location = {
                        .file = filename,
                        .line = error.line,
                        .column = error.column,
                        .length = 0,
                    },
                    .pretty = error.message.c_str(),
                };
                args->eval_error(args->userdata, &cerror);
            }

            return PatternEvalError;
        }
    }

    try
    {
        ApiPatternVisitor visitor(args);
        for(const auto & pattern : runtime.getPatterns())
        {
            pattern->accept(visitor);
        }
        return PatternSuccess;
    }
    catch(const std::exception & e)
    {
        args->log_handler(args->userdata, LogLevelError, e.what());
        return PatternEvalError;
    }
}
