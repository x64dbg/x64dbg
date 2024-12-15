#include "preprocessor.h"

#include <vector>
#include <stdexcept>
#include <unordered_map>

struct Line
{
	size_t number = 0;
	bool comment = false;
	std::string text;
	std::string eolcomment;

	std::string str() const
	{
		std::string s;
		s += "line ";
		s += std::to_string(number);
		if (comment)
			s += " (comment)";
		s += ": ";
		s += text;
		s += eolcomment;
		return s;
	}

	void print() const
	{
		puts(str().c_str());
	}
};

struct Tokenizer
{
	struct exception : public std::runtime_error
	{
		exception(const Line& line, const std::string& message = std::string())
			: std::runtime_error(message + "\n=========\n" + line.str() + "\n=========")
		{
		}
	};

	const Line& line;
	size_t position = 0;

	Tokenizer(const Line& line)
		: line(line) { }

	int peek() const
	{
		if (position >= line.text.length())
			return EOF;
		return line.text[position];
	}

	char consume()
	{
		if (position >= line.text.length())
			error("cannot consuum");
		return line.text[position++];
	}

	void skip_spaces(bool required = false)
	{
		auto oldPosition = position;
		while (true)
		{
			auto ch = peek();
			if (ch == ' ' || ch == '\t')
				consume();
			else
				break;
		}
		if (required && oldPosition == position)
			error("whitespace was expected, none found");
	}

	void error(const std::string& message)
	{
		throw exception(line, std::to_string(line.number) + ":" + std::to_string(position + 1) + " " + message);
	}

	std::string identifier()
	{
		std::string name;
		while (true)
		{
			auto ch = peek();
			if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
			{
				name.push_back(consume());
			}
			else if (!name.empty() && (ch >= '0' && ch <= '9'))
			{
				name.push_back(consume());
			}
			else
			{
				break;
			}
		}
		if (name.empty())
			error("expected identifier");
		return name;
	}

	std::string remainder()
	{
		std::string result;
		while (true)
		{
			auto ch = peek();
			if (ch == EOF)
				break;
			result.push_back(consume());
		}
		return result;
	}

	std::string until(char expected)
	{
		std::string result;
		while (true)
		{
			auto ch = peek();
			if (ch == EOF)
				error("unexpected end of file");
			if (ch == expected)
				break;
			result.push_back(consume());
		}
		return result;
	}

	void expect(int expected)
	{
		auto actual = peek();
		if(actual != expected)
		{
			auto chstr = [](int ch)
			{
				std::string result;
				if(ch == EOF)
				{
					result = "EOF";
				}
				else
				{
					result += "'";
					result += (char)ch;
					result += "'";
				}
				return result;
			};
			error("expected character " + chstr(expected) + ", got " + chstr(actual));
		}
		if(expected != EOF)
		{
			consume();
		}
	}
};

std::string remove_block_comments(const std::string& input)
{
	std::string result;
	bool inComment = false;
	for (size_t i = 0; i < input.length(); i++)
	{
		if (inComment)
		{
			if (input[i] == '*' && i + 1 < input.length() && input[i + 1] == '/')
				inComment = false;
		}
		else
		{
			if (input[i] == '/' && i + 1 < input.length() && input[i + 1] == '*')
				inComment = true;
		}
		if (!inComment)
			result += input[i];
	}
	return result;
}

std::string remove_line_comments(std::string& input)
{
	std::string line;
	auto removeComment = [&line]()
	{
		auto commentIdx = line.find("//");
		if (commentIdx != std::string::npos)
		{
			line.resize(commentIdx);
		}
	};

	std::string result;
	for (auto ch : input)
	{
		if (ch == '\r')
		{
			continue;
		}

		if (ch == '\n')
		{
			removeComment();
			result += line;
			result += '\n';
			line.clear();
		}
		else
		{
			line.push_back(ch);
		}
	}

	if (!line.empty())
	{
		removeComment();
		result += line;
	}
	return result;
}

// TODO: support comments
std::vector<Line> split_lines(const std::string& input)
{
	auto input_uncommented = remove_block_comments(input);
	std::vector<Line> lines;
	Line line;

	size_t lineNumber = 1;
	line.number = lineNumber;
	for (auto ch : input)
	{
		if (ch == '\r')
			continue;

		if (ch == '\n')
		{
			lineNumber++;
			if (!line.text.empty() && line.text.back() == '\\')
			{
				// continuation
				line.text.back() = '\n';
			}
			else
			{
				lines.push_back(line);
				line.number = lineNumber;
				line.text.clear();
			}
		}
		else
		{
			line.text.push_back(ch);
		}
	}

	if (!line.text.empty())
	{
		lines.push_back(line);
		line.text.clear();
	}

	for (auto& line : lines)
	{
		line.text = remove_line_comments(line.text);
	}

	return lines;
}

//Taken from: https://stackoverflow.com/a/24315631
void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while ((start_pos = s.find(from, start_pos)) != std::string::npos)
	{
		s.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

std::string preprocess(const std::string& input, std::string& error, const std::unordered_map<std::string, std::string>& definitions)
{
	auto lines = split_lines(input);
	std::vector<Line> final;
	struct Scope
	{
		size_t lineIndex = 0;
		std::string condition;
		bool value = false;
	};
	std::vector<Scope> stack;
	auto state = definitions;
	auto emitting = [&stack]()
	{
		for (const auto& s : stack)
			if (!s.value)
				return false;
		return true;
	};
	auto evaluate = [&state](Tokenizer& t)
	{
		// TODO: support proper expression evaluation
		auto id = t.identifier();
		if(id == "defined")
		{
			t.expect('(');
			auto name = t.identifier();
			t.expect(')');
			t.expect(EOF);
			return state.count(name) > 0;
		}

		t.error("unsupported expression '" + id + "'");
		return false;
	};
	for (size_t i = 0; i < lines.size(); i++)
	{
		const auto& line = lines[i];
		Tokenizer t(lines[i]);
		t.skip_spaces();

		if (t.peek() == '#')
		{
			t.consume();
			t.skip_spaces();

			auto directive = t.identifier();
			//line.print();

			if (directive == "if")
			{
				t.skip_spaces(true);
				auto pos = t.position;
				auto expression = t.remainder();
				while(!expression.empty() && std::isspace(expression.back()))
					expression.pop_back();
				t.position = pos;
				auto result = evaluate(t);
				//printf("#if(%s): %d\n", expression.c_str(), result);
				stack.push_back({i, expression, result});
			}
			else if (directive == "ifndef")
			{
				t.skip_spaces(true);
				auto identifier = t.identifier();
				//printf("#ifndef(%s)\n", identifier.c_str());
				stack.push_back({ i, "!defined(" + identifier + ")", state.count(identifier) == 0 });
				//printf("emitting: %d\n", emitting());
			}
			else if (directive == "ifdef")
			{
				t.skip_spaces(true);
				auto identifier = t.identifier();
				//printf("#ifdef(%s)\n", identifier.c_str());
				stack.push_back({ i, identifier, state.count(identifier) != 0 });
				//printf("emitting: %d\n", emitting());
			}
			else if (directive == "else")
			{
				if (stack.empty())
					t.error("no matching #if for #else");
				if (!stack.back().value)
				{
					stack.back().value = true;
				}
				//printf("#else (%s)\n", stack.back().condition.c_str());
				//printf("emitting: %d\n", emitting());
			}
			else if (directive == "endif")
			{
				if (stack.empty())
					t.error("no matching #if for #endif");
				//printf("#endif (%s)\n", stack.back().condition.c_str());
				stack.pop_back();
				//printf("emitting: %d\n", emitting());
			}
			else if (directive == "define")
			{
				t.skip_spaces(true);
				auto identifier = t.identifier();
				if (t.peek() == '(')
				{
					t.consume();
					t.skip_spaces();
					std::vector<std::string> parameters;
					while (true)
					{
						auto ch = t.peek();
						if (ch == ')')
							break;
						if (ch == EOF)
							throw std::runtime_error("expected ')', got EOF instead");

						auto argument = t.identifier();
						parameters.push_back(argument);
						t.skip_spaces();
						ch = t.peek();
						if (ch == ')')
							break;
						else if (ch == ',')
						{
							t.consume();
							t.skip_spaces();
						}
						else
							t.error("expect ',' or ')' got something else (too lazy sry)");
					}
					t.consume();
					t.skip_spaces();
					auto token = t.remainder();

					std::string pretty;
					for (size_t i = 0; i < parameters.size(); i++)
					{
						if (i > 0)
							pretty += ", ";
						pretty += parameters[i];
					}

					//printf("#define %s('%s' = '%s')\n", identifier.c_str(), pretty.c_str(), token.c_str());
				}
				else
				{
					// TODO: cut out comments
					t.skip_spaces();
					auto token = t.remainder();
					if (token.empty())
					{
						//printf("#define(%s)\n", identifier.c_str());
					}
					else
					{
						//printf("#define('%s' = '%s')\n", identifier.c_str(), token.c_str());
					}
					if (emitting())
					{
						state[identifier] = token;
					}
				}
			}
			else if (directive == "include")
			{
				t.skip_spaces();
				auto type = t.peek();
				if (type == '\"')
				{
					t.consume();
					auto file = t.until('\"');
					//printf("#include \"%s\"\n", file.c_str());
				}
				else if (type == '<')
				{
					t.consume();
					auto file = t.until('>');
					//printf("#include <%s>\n", file.c_str());
				}
				else
				{
					t.error("invalid syntax for #include");
				}
			}
			else if(directive == "pragma")
			{
				t.skip_spaces(true);
				auto type = t.identifier();
				if(type == "once")
				{
					//printf("#pragma once");
					// TODO: implement something?
				}
				else
				{
					t.error("unsupported #pragma type '" + type + "'");
				}
			}
			else
			{
				//printf("directive: '%s'\n", directive.c_str());
				t.error("unknown directive '" + directive + "'");
			}
		}
		else if (emitting())
		{
			final.push_back(line);
		}
	}

	std::string result;
	for (const auto& line : final)
	{
		result += line.text;
		result += '\n';
	}

	// TODO: strip out comments
	// TODO: somehow prevent replacing inside strings IsInsideString(position)
	// TODO: recursively replace


	// HACK: not proper
	for (const auto& itr : state)
	{
		ReplaceAll(result, itr.first, itr.second);
	}

	return result;
}
