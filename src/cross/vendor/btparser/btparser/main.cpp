#include "testfiles.h"
#include "lexer.h"
#include "helpers.h"
#include "preprocessor.h"
#include "types.h"

#include <stdio.h>
#include <unordered_set>

bool TestLexer(Lexer& lexer, const std::string& filename)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return false;
	}
	std::string actual;
	actual.reserve(65536);
	auto success = lexer.Test([&](const std::string& line)
	{
		actual.append(line);
	});
	std::string expected;
	if (FileHelper::ReadAllText("tests\\exp_lex\\" + filename, expected) && expected == actual)
	{
		printf("lexer test for \"%s\" success!\n", filename.c_str());
		return true;
	}
	if (success)
		return true;
	printf("lexer test for \"%s\" failed...\n", filename.c_str());
	FileHelper::WriteAllText("expected.out", expected);
	FileHelper::WriteAllText("actual.out", actual);
	return false;
}

bool DebugLexer(Lexer& lexer, const std::string& filename, bool output)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return false;
	}
	auto success = lexer.Test([](const std::string& line)
	{
		printf("%s", line.c_str());
	}, output);
	if (output)
		puts("");
	return success;
}

void GenerateExpected(Lexer& lexer, const std::string& filename)
{
	if (!lexer.ReadInputFile("tests\\" + filename))
	{
		printf("failed to read \"%s\"\n", filename.c_str());
		return;
	}
	std::string actual;
	actual.reserve(65536);
	lexer.Test([&](const std::string& line)
	{
		actual.append(line);
	});
	FileHelper::WriteAllText("tests\\exp_lex\\" + filename, actual);
}

void GenerateExpectedTests()
{
	Lexer lexer;
	for (auto file : testFiles)
		GenerateExpected(lexer, file);
}

void RunLexerTests()
{
	Lexer lexer;
	for (auto file : testFiles)
		TestLexer(lexer, file);
}

void DebugLexerTests(bool output = true)
{
	Lexer lexer;
	for (auto file : testFiles)
		DebugLexer(lexer, file, output);
}

static std::vector<std::string> SplitNamespace(const std::string& name, std::string& type)
{
	auto namespaces = StringUtils::Split(name, ':');
	if (namespaces.empty())
	{
		type.clear();
	}
	else
	{
		type = namespaces.back();
		namespaces.pop_back();
	}
	return namespaces;
}

static void HandleVTable(Types::Model& model)
{
	std::unordered_set<std::string> classes;
	std::unordered_set<std::string> visited;
	auto handleType = [&](const Types::QualifiedType& type)
	{
		if (!type.kind.empty())
		{
			const auto& name = type.name;
			const auto& kind = type.kind;
			if (visited.count(name) != 0)
				return;
			visited.insert(name);
			std::string rettype;
			auto namespaces = SplitNamespace(name, rettype);
			if (kind == "class" && rettype[0] == 'I')
				classes.insert(name);
			for (const auto& ns : namespaces)
			{
				printf("namespace %s {\n", ns.c_str());
			}
			printf("    %s %s {\n    };\n", kind.c_str(), rettype.c_str());
			for (const auto& ns : namespaces)
				printf("}\n");
		}
	};
	for (const auto& su : model.structUnions)
	{
		for (const auto& vf : su.vtable)
		{
			handleType(vf.rettype);
			for (const auto& arg : vf.args)
				handleType(arg.type);
		}
	}
	for (const auto& f : model.functions)
	{
		handleType(f.rettype);
		for (const auto& arg : f.args)
			handleType(arg.type);
	}

	puts("Used classes:");
	for (const auto& name : classes)
	{
		printf("%s\n", name.c_str());
	}
	puts("vtable chuj");
}

bool DebugParser(const std::string& filename)
{
	std::string data;
	if (!FileHelper::ReadAllText(filename, data))
	{
		printf("Failed to read: %s\n", filename.c_str());
		return false;
	}

	std::string pperror;
	std::unordered_map<std::string, std::string> definitions;
	definitions["WIN32"] = "";
	definitions["_MSC_VER"] = "1337";
	auto ppData = preprocess(data, pperror, definitions);
	if (!pperror.empty())
	{
		printf("Preprocess error: %s\n", pperror.c_str());
		return false;
	}

	auto basename = filename;
	auto lastSlashIdx = basename.find_last_of("\\/");
	if(lastSlashIdx != std::string::npos)
	{
		basename = basename.substr(lastSlashIdx + 1);
	}

	FileHelper::WriteAllText(basename + ".pp.h", ppData);

	std::vector<std::string> errors;
	Types::Model model;
	if (!Types::ParseModel(ppData, filename, errors, model))
	{
		puts("Failed to parse types:");
		for (const auto& error : errors)
			puts(error.c_str());
		return false;
	}
	puts("ParseModel success!");

	HandleVTable(model);

	return true;
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		printf("Usage: test my.h\n");
		return EXIT_FAILURE;
	}
	DebugParser(argv[1]);
	return EXIT_SUCCESS;
}