#ifndef UTF8INI_H
#define UTF8INI_H

#include <string>
#include <map>
#include <vector>

class Utf8Ini
{
public:
    /**
    \brief Serialize to the INI file format.
    */
    inline std::string Serialize() const
    {
        std::string output;
        for(const auto & section : mSections)
        {
            if(output.length())
                appendLine(output, "");
            appendLine(output, makeSectionText(section.first));
            for(const auto & keyvalue : section.second)
                if(keyvalue.first.length())
                    appendLine(output, makeKeyValueText(keyvalue.first, keyvalue.second));
        }
        return output;
    }

    /**
    \brief Deserialize from the INI file format.
    \param data The INI data.
    \param [out] errorLine The error line (only has a meaning when this function failed).
    \return true if it succeeds, false if it fails.
    */
    inline bool Deserialize(const std::string & data, int & errorLine)
    {
        //initialize
        errorLine = 0;
        Clear();

        //read lines
        std::vector<std::string> lines;
        std::string curLine;
        for(auto ch : data)
        {
            switch(ch)
            {
            case '\r':
                break;
            case '\n':
                lines.push_back(trim(curLine));
                curLine.clear();
                break;
            default:
                curLine += ch;
            }
        }
        if(curLine.length())
            lines.push_back(trim(curLine));

        //parse lines
        std::string section, key, value;
        for(const auto & line : lines)
        {
            errorLine++;
            switch(getLineType(line))
            {
            case LineType::Invalid:
                return false;

            case LineType::Comment:
            case LineType::Empty:
                continue;

            case LineType::KeyValue:
            {
                if(!section.length() ||
                        !parseKeyValueLine(line, key, value) ||
                        !SetValue(section, key, value))
                    return false;
            }
            break;

            case LineType::Section:
            {
                if(!parseSectionLine(line, section))
                    return false;
            }
            break;

            default:
                return false;
            }
        }
        return true;
    }

    /**
    \brief Sets a value. This will overwrite older values.
    \param section The section. Must have a length greater than one.
    \param key The key. Must have a length greater than one.
    \param value The value. Can be empty (effectively removing the value).
    \return true if the value was set successfully, false otherwise.
    */
    inline bool SetValue(const std::string & section, const std::string & key, const std::string & value)
    {
        auto trimmedSection = trim(section);
        auto trimmedKey = trim(key);
        if(!trimmedSection.length() || !trimmedKey.length())
            return false;
        auto found = mSections.find(trimmedSection);
        if(found != mSections.end())
            found->second.emplace(std::move(trimmedKey), value);
        else
        {
            KeyValueMap keyValueMap;
            keyValueMap.emplace(std::move(trimmedKey), value);
            mSections.emplace(std::move(trimmedSection), std::move(keyValueMap));
        }
        return true;
    }

    /**
    \brief Removes all key/value pairs from a section.
    \param section The section to clear.
    \return true if it succeeds, false otherwise.
    */
    inline bool ClearSection(const std::string & section)
    {
        auto trimmedSection = trim(section);
        if(!trimmedSection.length())
            return false;
        auto found = mSections.find(trimmedSection);
        if(found == mSections.end())
            return false;
        mSections.erase(found);
        return true;
    }

    /**
    \brief Removes all sections.
    */
    inline void Clear()
    {
        mSections.clear();
    }

    /**
    \brief Gets a value.
    \param section The section.
    \param key The key.
    \return The value. Empty string when the value was not found or empty.
    */
    inline std::string GetValue(const std::string & section, const std::string & key) const
    {
        auto trimmedSection = trim(section);
        auto trimmedKey = trim(key);
        if(!trimmedSection.length() || !trimmedKey.length())
            return "";
        auto sectionFound = mSections.find(trimmedSection);
        if(sectionFound == mSections.end())
            return "";
        const auto & keyValueMap = sectionFound->second;
        auto keyFound = keyValueMap.find(trimmedKey);
        if(keyFound == keyValueMap.end())
            return "";
        return keyFound->second;
    }

    /**
    \brief Gets the section names.
    \return List of section names.
    */
    inline std::vector<std::string> Sections() const
    {
        std::vector<std::string> sections;
        sections.reserve(mSections.size());
        for(const auto & section : mSections)
            sections.push_back(section.first);
        return sections;
    }

    /**
    \brief Gets keys in a given section.
    \param section The section.
    \return List of keys in the section. Empty if the section is not found or empty.
    */
    inline std::vector<std::string> Keys(const std::string & section) const
    {
        std::vector<std::string> keys;
        auto trimmedSection = trim(section);
        if(trimmedSection.length())
        {
            auto found = mSections.find(trimmedSection);
            if(found != mSections.end())
            {
                keys.reserve(found->second.size());
                for(const auto & key : found->second)
                    keys.push_back(key.first);
            }
        }
        return std::move(keys);
    }

private:
    typedef std::map<std::string, std::string> KeyValueMap;
    std::map<std::string, KeyValueMap> mSections;

    enum class LineType
    {
        Invalid,
        Empty,
        Section,
        KeyValue,
        Comment
    };

    static inline LineType getLineType(const std::string & line)
    {
        auto len = line.length();
        if(!len)
            return LineType::Empty;
        if(line[0] == '[' && line[len - 1] == ']')
            return LineType::Section;
        if(line[0] == ';')
            return LineType::Comment;
        if(line.find('=') != std::string::npos)
            return LineType::KeyValue;
        return LineType::Invalid;
    }

    static inline std::string trim(const std::string & str)
    {
        auto len = str.length();
        if(!len)
            return "";
        size_t pre = 0;
        while(str[pre] == ' ')
            pre++;
        size_t post = 0;
        while(str[len - post - 1] == ' ' && post < len)
            post++;
        auto sublen = len - post - pre;
        return sublen > 0 ? str.substr(pre, len - post - pre) : "";
    }

    static inline bool parseKeyValueLine(const std::string & line, std::string & key, std::string & value)
    {
        auto pos = line.find('=');
        key = trim(line.substr(0, pos));
        value = trim(line.substr(pos + 1));
        auto len = value.length();
        if(len && value[0] == '\"' && value[len - 1] == '\"')
            value = unescapeValue(value);
        return true;
    }

    static inline bool parseSectionLine(const std::string & line, std::string & section)
    {
        section = trim(line.substr(1, line.length() - 2));
        return section.length() > 0;
    }

    static inline void appendLine(std::string & output, const std::string & line)
    {
        if(output.length())
            output += "\r\n";
        output += line;
    }

    static inline std::string makeSectionText(const std::string & section)
    {
        return "[" + section + "]";
    }

    static inline std::string makeKeyValueText(const std::string & key, const std::string & value)
    {
        return key + "=" + escapeValue(value);
    }

    static inline bool needsEscaping(const std::string & value)
    {
        auto len = value.length();
        return len && (value[0] == ' ' || value[len - 1] == ' ' ||
                       value.find('\n') != std::string::npos ||
                       value.find('\"') != std::string::npos);
    }

    static inline std::string escapeValue(const std::string & value)
    {
        if(!needsEscaping(value))
            return value;
        std::string escaped = "\"";
        for(auto ch : value)
        {
            switch(ch)
            {
            case '\"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
            }
        }
        escaped += "\"";
        return std::move(escaped);
    }

    static inline std::string unescapeValue(const std::string & str)
    {
        std::string result;
        auto bEscaped = false;
        for(auto ch : str)
        {
            if(!bEscaped)
            {
                switch(ch)
                {
                case '\"':
                    break;
                case '\\':
                    bEscaped = true;
                    break;
                default:
                    result += ch;
                }
            }
            else
            {
                switch(ch)
                {
                case 'r':
                    result += '\r';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 't':
                    result += '\t';
                    break;
                default:
                    result += ch;
                }
                bEscaped = false;
            }
        }
        if(bEscaped)
            result += '\\';
        return std::move(result);
    }
};

#endif //UTF8INI_H