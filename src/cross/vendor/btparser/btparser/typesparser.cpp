#include "types.h"
#include "helpers.h"

using namespace Types;

#include "lexer.h"

#define dprintf printf
#define QT_TRANSLATE_NOOP(ctx, s) s

struct Parser
{
	Lexer lexer;
	std::string owner;
	std::vector<std::string>& errors;

	std::vector<Lexer::TokenState> tokens;
	size_t index = 0;
	Model model;
	std::unordered_map<std::string, size_t> structUnions;
	int anonymousId = 0;

	Parser(const std::string& code, const std::string& owner, std::vector<std::string>& errors)
		: owner(owner), errors(errors)
	{
		lexer.SetInputData(code);
	}

	Lexer::TokenState& getToken(size_t i)
	{
		if (index >= tokens.size() - 1)
			i = tokens.size() - 1;
		return tokens[i];
	}

	Lexer::TokenState& curToken()
	{
		return getToken(index);
	}

	bool isToken(Lexer::Token token)
	{
		return getToken(index).Token == token;
	}

	bool isStructLike()
	{
		auto tok = getToken(index).Token;
		switch (tok)
		{
		case Lexer::tok_struct:
		case Lexer::tok_union:
		case Lexer::tok_enum:
		case Lexer::tok_class:
			return true;
		default:
			return false;
		}
	}

	bool isTokenList(std::initializer_list<Lexer::Token> il)
	{
		size_t i = 0;
		for (auto l : il)
			if (getToken(index + i++).Token != l)
				return false;
		return true;
	}

	void errLine(const Lexer::TokenState& token, const std::string& message)
	{
		auto error = StringUtils::sprintf("[line %zu:%zu] %s", token.CurLine + 1, token.LineIndex, message.c_str());
		errors.push_back(std::move(error));
	}

	void eatSemic()
	{
		while (curToken().Token == Lexer::tok_semic)
			index++;
	}

	bool parseVariable(const std::vector<Lexer::TokenState>& tlist, QualifiedType& type, std::string& name, Lexer::Token kind)
	{
		std::string stype; // TODO: get rid of this variable
		type = QualifiedType();
		switch (kind)
		{
		case Lexer::tok_struct:
			type.kind = "struct";
			break;
		case Lexer::tok_class:
			type.kind = "class";
			break;
		case Lexer::tok_union:
			type.kind = "union";
			break;
		case Lexer::tok_enum:
			type.kind = "enum";
			break;
		default:
			break;
		}
		name.clear();

		bool sawPointer = false;
		bool isKeyword = true;
		size_t i = 0;
		for (; i < tlist.size(); i++)
		{
			const auto& t = tlist[i];
			if (t.Is(Lexer::tok_const))
			{
				if (i == 0 || type.pointers.empty())
					type.isConst = true;
				else
					type.pointers.back().isConst = true;
				continue;
			}

			auto isType = t.IsType();
			if (!isType)
			{
				isKeyword = false;
			}

			if (isType)
			{
				if (isKeyword)
				{
					if (!stype.empty())
						stype += ' ';
					stype += lexer.TokString(t);
				}
				else
				{
					errLine(t, "invalid keyword in type");
					return false;
				}
			}
			else if (t.Is(Lexer::tok_identifier))
			{
				if (stype.empty())
				{
					stype = t.IdentifierStr;
				}
				else if (i + 1 == tlist.size())
				{
					name = t.IdentifierStr;
				}
				else
				{
					errLine(t, "invalid identifier in type");
					return false;
				}
			}
			else if (t.Is(Lexer::tok_op_mul) || t.Is(Lexer::tok_op_and))
			{
				if (stype.empty())
				{
					errLine(t, "unexpected * in type");
					return false;
				}

				if (sawPointer && stype.back() != '*')
				{
					errLine(t, "unexpected * in type");
					return false;
				}

				if (!sawPointer)
					type.name = stype;

				type.pointers.emplace_back();

				// Apply the pointer to the type on the left
				stype += '*';
				sawPointer = true;
			}
			else
			{
				errLine(t, "invalid token in type");
				return false;
			}
		}
		if (stype.empty())
			__debugbreak();
		if (!sawPointer)
			type.name = stype;
		return true;
	}

	bool parseFunction(Lexer::Token retkind, std::vector<Lexer::TokenState>& rettypes, Function& fn, bool ptr)
	{
		if (rettypes.empty())
		{
			errLine(curToken(), "expected return type before function pointer type");
			return false;
		}

		// TODO: calling conventions

		std::string retname;
		if (!parseVariable(rettypes, fn.rettype, retname, retkind))
			return false;

		if (ptr)
		{
			if (!retname.empty())
			{
				errLine(rettypes.back(), "invalid return type in function pointer");
				return false;
			}

			if (!isToken(Lexer::tok_op_mul) && !isToken(Lexer::tok_op_and))
			{
				errLine(curToken(), "expected * in function pointer type");
				return false;
			}
			index++;

			while (isToken(Lexer::tok_const) || isToken(Lexer::tok_volatile))
				index++;

			if (!isToken(Lexer::tok_identifier))
			{
				if (isToken(Lexer::tok_parclose))
				{
					// unnamed function pointer
					fn.name = "";
				}
				else
				{
					errLine(curToken(), "expected identifier in function pointer type");
					return false;
				}
			}
			else
			{
				fn.name = lexer.TokString(curToken());
				index++;
			}

			if (!isToken(Lexer::tok_parclose))
			{
				errLine(curToken(), "expected ) after function pointer type name");
				return false;
			}
			index++;

			if (!isToken(Lexer::tok_paropen))
			{
				errLine(curToken(), "expected ( for start of parameter list in function pointer type");
				return false;
			}
			index++;
		}
		else if (retname.empty())
		{
			errLine(rettypes.back(), "function name cannot be empty");
			return false;
		}
		else
		{
			fn.name = retname;
		}

		auto kind = Lexer::tok_eof;
		std::vector<Lexer::TokenState> tlist;
		auto startToken = curToken();
		auto finalizeArgument = [&]()
		{
			Member am;
			if (!parseVariable(tlist, am.type, am.name, kind))
				return false;
			fn.args.push_back(am);
			kind = Lexer::tok_eof;
			tlist.clear();
			startToken = curToken();
			return true;
		};
		while (!isToken(Lexer::tok_parclose))
		{
			if (isToken(Lexer::tok_comma))
			{
				index++;
				if (!finalizeArgument())
					return false;
			}

			if (isStructLike())
			{
				if (tlist.empty() && getToken(index + 1).Token == Lexer::tok_identifier)
				{
					kind = curToken().Token;
					index++;
				}
				else
				{
					errLine(curToken(), "unsupported struct/union/enum in function argument");
					return false;
				}
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;

				// Primitive type
				tlist.push_back(t);
			}
			else if (t.Is(Lexer::tok_op_mul) || t.Is(Lexer::tok_op_and))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in function type argument list");
					return false;
				}
				index++;

				tlist.push_back(t);
			}
			else if (isTokenList({ Lexer::tok_subopen, Lexer::tok_subclose }))
			{
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected [ in function type argument list");
					return false;
				}
				index += 2;

				Lexer::TokenState fakePtr;
				fakePtr.Token = Lexer::tok_op_mul;
				fakePtr.CurLine = t.CurLine;
				fakePtr.LineIndex = t.LineIndex;
				if (tlist.size() > 1 && tlist.back().Is(Lexer::tok_identifier))
				{
					tlist.insert(tlist.end() - 1, fakePtr);
				}
				else
				{
					tlist.push_back(fakePtr);
				}
			}
			else if (t.Is(Lexer::tok_varargs))
			{
				if (!tlist.empty())
				{
					errLine(t, "unexpected ... in function type argument list");
					return false;
				}

				index++;
				if (!isToken(Lexer::tok_parclose))
				{
					errLine(curToken(), "expected ) after ... in function type argument list");
					return false;
				}

				Member am;
				am.type = QualifiedType("...");
				fn.args.push_back(am);
				break;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				auto startToken = curToken();
				index++;

				// Function pointer argument to a function
				Function subfn;
				subfn.typeonly = true;
				if (!parseFunction(kind, tlist, subfn, true))
				{
					return false;
				}
				kind = Lexer::tok_eof;

				// Create fake tokens
				auto typeToken = tlist.back();
				typeToken.Token = Lexer::tok_identifier;
				typeToken.IdentifierStr = fn.name + "_" + subfn.name + "_fnptr";

				auto nameToken = startToken;
				nameToken.Token = Lexer::tok_identifier;
				nameToken.IdentifierStr = subfn.name;

				// Replace the return type with the fake tokens
				tlist.clear();
				tlist.push_back(typeToken);
				tlist.push_back(nameToken);

				// Add the function to the model
				subfn.name = typeToken.IdentifierStr;
				model.functions.push_back(subfn);
			}
			else
			{
				errLine(curToken(), "unsupported token in function type argument list");
				return false;
			}
		}
		index++;

		if (tlist.empty())
		{
			// Do nothing
		}
		else if (tlist.size() == 1 && tlist[0].Token == Lexer::tok_void)
		{
			if (!fn.args.empty())
			{
				errLine(tlist[0], "invalid argument type: void");
				return false;
			}
			return true;
		}
		else if (!finalizeArgument())
		{
			return false;
		}

		return true;
	}

	bool parseMember(StructUnion& su)
	{
		Member m;
		bool sawPointer = false;
		auto kind = Lexer::tok_eof;
		std::vector<Lexer::TokenState> tlist;
		auto startToken = curToken();

		auto finalizeMember = [&]()
		{
			if (tlist.size() < 2)
			{
				errLine(startToken, "not enough tokens in member");
				return false;
			}

			if (!parseVariable(tlist, m.type, m.name, kind))
				return false;
			kind = Lexer::tok_eof;

			if (m.type.name == "void" && !m.type.isPointer())
			{
				errLine(startToken, "void is not a valid member type");
				return false;
			}

			if (m.type.empty() || m.name.empty())
				__debugbreak();

			su.members.push_back(m);
			return true;
		};

		while (!isToken(Lexer::tok_semic))
		{
			if (isToken(Lexer::tok_eof))
			{
				errLine(curToken(), "unexpected eof in member");
				return false;
			}

			if (isStructLike())
			{
				if (getToken(index + 1).Token == Lexer::tok_identifier)
				{
					kind = curToken().Token;
					index++;
				}
				else
				{
					errLine(curToken(), "unsupported struct/union/enum in member");
					return false;
				}
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;
				// Primitive type / name
				tlist.push_back(t);
			}
			else if (t.Is(Lexer::tok_op_mul) || t.Is(Lexer::tok_op_and))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in member");
					return false;
				}

				if (sawPointer && tlist.back().Token != Lexer::tok_op_mul && tlist.back().Token != Lexer::tok_op_and)
				{
					errLine(curToken(), "unexpected * in member");
					return false;
				}

				index++;

				tlist.push_back(t);
				sawPointer = true;
			}
			else if (t.Is(Lexer::tok_subopen))
			{
				index++;

				// Array
				if (!isToken(Lexer::tok_number))
				{
					errLine(curToken(), "expected number token after array");
					return false;
				}
				m.arrsize = (int)curToken().NumberVal;
				index++;

				if (!isToken(Lexer::tok_subclose))
				{
					errLine(curToken(), "expected ] after array size");
					return false;
				}
				index++;

				break;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				index++;

				// Function pointer type
				Function subfn;
				subfn.typeonly = true;
				if (!parseFunction(kind, tlist, subfn, true))
				{
					return false;
				}
				kind = Lexer::tok_eof;

                // TODO: eat nested {} as well?
				if (!isToken(Lexer::tok_semic))
				{
                    errLine(curToken(), "expected ; after member function type");
					return false;
				}
				eatSemic();

				// Create fake tokens
				auto typeToken = tlist.back();
				typeToken.Token = Lexer::tok_identifier;
				typeToken.IdentifierStr = su.name + "_" + subfn.name + "_fnptr";

				auto nameToken = startToken;
				nameToken.Token = Lexer::tok_identifier;
				nameToken.IdentifierStr = subfn.name;

				// Replace the return type with the fake tokens
				tlist.clear();
				tlist.push_back(typeToken);
				tlist.push_back(nameToken);

				// Add the function to the model
				subfn.name = typeToken.IdentifierStr;
				model.functions.push_back(subfn);

				return finalizeMember();
			}
			else if (t.Is(Lexer::tok_comma))
			{
				// Comma-separated members
				index++;

				if (!finalizeMember())
					return false;

				// Remove the name from the type
				if (tlist.back().Token != Lexer::tok_identifier)
					__debugbreak();
				tlist.pop_back();

				// Remove the pointer from the type
				while (!tlist.empty() && (tlist.back().Token == Lexer::tok_op_mul || tlist.back().Token == Lexer::tok_op_and))
					tlist.pop_back();
				sawPointer = false;

				m = Member();
			}
			else if (t.Is(Lexer::tok_virtual))
			{
				// Parse a virtual function declaration
				index++;

				// Parse the function declaration
				Function vfunction;
				if (!parseFunctionTop(vfunction, true))
					return false;
				su.vtable.push_back(vfunction);
			}
			else if (t.Is(Lexer::tok_brclose) && tlist.empty())
			{
				return true;
			}
			else
			{
				__debugbreak();
			}
		}

		if (!isToken(Lexer::tok_semic))
		{
			errLine(curToken(), "expected ; after member");
			return false;
		}
		eatSemic();

		if (!finalizeMember())
			return false;

		return true;
	}

	bool parseStructUnion(bool inTypedef, std::string* outName = nullptr)
	{
		auto startToken = curToken();
		if (isToken(Lexer::tok_struct) || isToken(Lexer::tok_class) || isToken(Lexer::tok_union))
		{
			StructUnion su;
			su.isunion = isToken(Lexer::tok_union);
			index++;
			if (isToken(Lexer::tok_identifier))
			{
				su.name = curToken().IdentifierStr;
				index++;
			}
			else
			{
				su.name = "__anonymous_" + std::to_string(anonymousId++);
			}
			if(outName != nullptr)
			{
				*outName = su.name;
			}

			if (isToken(Lexer::tok_bropen))
			{
				index++;
				while (!isToken(Lexer::tok_brclose))
				{
					if (isToken(Lexer::tok_eof))
					{
						errLine(curToken(), StringUtils::sprintf("unexpected eof in %s", su.isunion ? "union" : "struct"));
						return false;
					}
					if (isToken(Lexer::tok_bropen))
					{
						errLine(curToken(), "nested blocks are not allowed!");
						return false;
					}
					if (!parseMember(su))
						return false;
				}
				index++;

				// Handle forward declarations
				auto found = structUnions.find(su.name);
				if (found != structUnions.end())
				{
					auto& oldSu = model.structUnions[found->second];
					if (oldSu.size != -1)
					{
						errLine(startToken, "cannot redeclare type");
						return false;
					}
					// Replace the forward declared type with the full type
					oldSu = su;
				}
				else
				{
					structUnions.emplace(su.name, model.structUnions.size());
					model.structUnions.push_back(su);
				}

				if(!inTypedef)
				{
					if (!isToken(Lexer::tok_semic))
					{
						errLine(curToken(), "expected semicolon!");
						return false;
					}
					eatSemic();
				}
				return true;
			}
			else if (isToken(Lexer::tok_semic) && !su.name.empty())
			{
				// Forward declaration
				su.size = -1;
				auto found = structUnions.find(su.name);
				if (found == structUnions.end())
				{
					structUnions.emplace(su.name, model.structUnions.size());
					model.structUnions.push_back(su);
				}
				if(!inTypedef)
					eatSemic();
				return true;
			}
			else
			{
				errLine(curToken(), "invalid struct token sequence!");
				return false;
			}
		}
		return true;
	}

	bool parseEnum(bool inTypedef, std::string* outName = nullptr)
	{
		if (isToken(Lexer::tok_enum))
		{
			Enum e;
			std::string etype;
			index++;
			if(curToken().Is(Lexer::tok_identifier))
			{
				e.name = curToken().IdentifierStr;
				index++;
			}
			else
			{
				e.name = "__anonymous_" + std::to_string(anonymousId++);
			}
			if(outName != nullptr)
			{
				*outName = e.name;
			}

			// TODO: support custom enum types (: type)

			if (curToken().Is(Lexer::tok_bropen))
			{
				index++;

				etype = "int";

				while (!isToken(Lexer::tok_brclose))
				{
					if (isToken(Lexer::tok_eof))
					{
						errLine(curToken(), "unexpected eof in enum");
						return false;
					}
					if (isToken(Lexer::tok_bropen))
					{
						errLine(curToken(), "nested blocks are not allowed!");
						return false;
					}

					if (!e.values.empty())
					{
						if (isToken(Lexer::tok_comma))
						{
							index++;
							if (isToken(Lexer::tok_brclose))
							{
								// Support final comma
								break;
							}
						}
						else
						{
							errLine(curToken(), "expected comma in enum");
							return false;
						}
					}

					if (!isToken(Lexer::tok_identifier))
					{
						errLine(curToken(), StringUtils::sprintf("expected identifier in enum, got '%s'", lexer.TokString(curToken()).c_str()));
						return false;
					}

					EnumValue v;
					v.name = lexer.TokString(curToken());
					index++;

					if (isToken(Lexer::tok_assign))
					{
						bool negative = false;
						index++;
						if (isToken(Lexer::tok_op_min))
						{
							index++;
							negative = true;
						}
						if (!isToken(Lexer::tok_number))
						{
							errLine(curToken(), "expected number after = in enum");
							return false;
						}
						v.value = curToken().NumberVal;
						if (negative)
						{
							v.value = -(int64_t)v.value;
						}
						index++;
					}
					else
					{
						v.value = e.values.empty() ? 0 : e.values.back().value + 1;
					}
					e.values.push_back(v);
				}
				index++; //eat tok_brclose

				model.enums.emplace_back(e, etype);
				if(!inTypedef)
				{
					if (!isToken(Lexer::tok_semic))
					{
						errLine(curToken(), "expected semicolon!");
						return false;
					}
					eatSemic();
				}
				return true;
			}
			else
			{
				errLine(curToken(), "invalid enum token sequence!");
				return false;
			}
			__debugbreak();
		}
		return true;
	}

	bool parseTypedef()
	{
		// TODO: support "typedef struct foo { members... };"
		// TODO: support "typedef enum foo { members... };"

		if (isToken(Lexer::tok_typedef))
		{
			index++;

			auto startToken = curToken();

			bool sawPointer = false;
			std::vector<Lexer::TokenState> tlist;
			auto kind = Lexer::tok_eof;
			while (!isToken(Lexer::tok_semic))
			{
				if (isToken(Lexer::tok_eof))
				{
					errLine(curToken(), "unexpected eof in typedef");
					return false;
				}

				if(isToken(Lexer::tok_comma))
				{
					// TODO
					errLine(curToken(), "unsupported comma in typedef");
					return false;
				}

				if (isStructLike())
				{
					// typedef struct/enum/union
					// Consume the 'struct'-like token
					kind = curToken().Token;
					index++;

					if(curToken().Is(Lexer::tok_bropen) || isTokenList({ Lexer::tok_identifier, Lexer::tok_bropen }))
					{
						// 'typedef struct {' OR 'typedef struct Name {'
						index--;
						std::string structName;
						if(kind == Lexer::tok_enum)
						{
							if(!parseEnum(true, &structName))
								return false;
						}
						else
						{
							if(!parseStructUnion(true, &structName))
								return false;
						}
						tlist.push_back({ Lexer::tok_identifier, structName });
					}
					else if(isTokenList({Lexer::tok_identifier, Lexer::tok_identifier}))
					{
						// typedef struct Name Name
						// NOTE: fallthrough
					}
					else if(isTokenList({Lexer::tok_identifier, Lexer::tok_op_mul}))
					{
						// typedef struct Name1 *
						// NOTE: fallthrough
					}
					else
					{
						errLine(curToken(), "unsupported typedef struct");
					}
				}

				const auto& t = curToken();
				if (t.IsType() || t.Token == Lexer::tok_identifier || t.Token == Lexer::tok_const)
				{
					// Primitive type
					index++;
					tlist.push_back(t);
				}
				else if (t.Is(Lexer::tok_op_mul) || t.Is(Lexer::tok_op_and))
				{
					// Pointer to the type on the left
					if (tlist.empty())
					{
						errLine(curToken(), "unexpected * in member");
						return false;
					}

					if (sawPointer && tlist.back().Token != Lexer::tok_op_mul && tlist.back().Token != Lexer::tok_op_and)
					{
						errLine(curToken(), "unexpected * in member");
						return false;
					}

					tlist.push_back(t);
					sawPointer = true;

					index++;
				}
				else if (t.Token == Lexer::tok_paropen)
				{
					// Function pointer type

					index++;

					Function fn;
					fn.typeonly = true;
					if (!parseFunction(kind, tlist, fn, true))
					{
						return false;
					}
					kind = Lexer::tok_eof;

					if (!isToken(Lexer::tok_semic))
					{
                        errLine(curToken(), "expected ; after function pointer typedef");
						return false;
					}
					eatSemic();

					model.functions.push_back(fn);

					// TODO: handle pointer stuff correctly?

					return true;
				}
				else
				{
					t.Throw("unsupported token in typedef");
				}
			}
			eatSemic();

			if (tlist.size() < 2)
			{
				errLine(startToken, "not enough tokens in typedef");
				return false;
			}

			Member tm;
			if (!parseVariable(tlist, tm.type, tm.name, kind))
				return false;
			model.types.push_back(tm);
		}
		return true;
	}

	bool parseFunctionTop(Function& fn, bool isVirtual)
	{
		fn = {};

		bool sawPointer = false;
		auto kind = Lexer::tok_eof;
		std::vector<Lexer::TokenState> tlist;
		while (!isToken(Lexer::tok_semic))
		{
			if (isToken(Lexer::tok_eof))
			{
				errLine(curToken(), "unexpected eof in function");
				return false;
			}

			if (isStructLike())
			{
				if (tlist.empty() && getToken(index + 1).Token == Lexer::tok_identifier)
				{
					kind = curToken().Token;
					index++;
				}
				else
				{
					errLine(curToken(), "unexpected struct/union/enum in function");
					return false;
				}
			}

			const auto& t = curToken();
			if (t.IsType() || t.Is(Lexer::tok_identifier) || t.Is(Lexer::tok_const))
			{
				index++;
				// Primitive type / name
				tlist.push_back(t);
			}
			else if (isTokenList({ Lexer::tok_op_neg, Lexer::tok_identifier }))
			{
				index++;
				auto td = curToken();
				index++;
				// Destructor name
				td.IdentifierStr = "~" + td.IdentifierStr;
				auto tvoid = t;
				tvoid.Token = Lexer::tok_void;
				tlist.push_back(std::move(tvoid));
				tlist.push_back(std::move(td));
			}
			else if (t.Is(Lexer::tok_op_mul) || t.Is(Lexer::tok_op_and))
			{
				// Pointer to the type on the left
				if (tlist.empty())
				{
					errLine(curToken(), "unexpected * in function");
					return false;
				}

				if (sawPointer && tlist.back().Token != Lexer::tok_op_mul && tlist.back().Token != Lexer::tok_op_and)
				{
					errLine(curToken(), "unexpected * in function");
					return false;
				}

				index++;

				tlist.push_back(t);
				sawPointer = true;
			}
			else if (t.Is(Lexer::tok_paropen))
			{
				index++;

				// Function pointer type
				if (!parseFunction(kind, tlist, fn, false))
				{
					return false;
				}
				kind = Lexer::tok_eof;

				if (!isToken(Lexer::tok_semic))
				{
					if (isVirtual)
					{
						auto endToken = curToken();
						while (true)
						{
							auto tEnd = curToken();
							if (tEnd.Is(Lexer::tok_eof))
							{
								errLine(curToken(), "unexpected eof after virtual function");
								return false;
							}

							if (tEnd.Is(Lexer::tok_const) || tEnd.Is(Lexer::tok_override))
							{
								index++;
								continue;
							}

							if (tEnd.Is(Lexer::tok_semic))
							{
								break;
							}

							if (tEnd.Is(Lexer::tok_assign))
							{
								index++;
								tEnd = curToken();
								if (!tEnd.Is(Lexer::tok_number) || tEnd.NumberVal != 0)
								{
									errLine(endToken, "expected = 0 after virtual function type");
									return false;
								}
								index++;
								break;
							}
							else
							{
								errLine(endToken, "expected = 0 after virtual function type");
								return false;
							}
						}
					}

                    if(isToken(Lexer::tok_bropen))
                    {
                        index++;

                        int depth = 1;
                        while(true)
                        {
                            if(isToken(Lexer::tok_eof))
                            {
                                errLine(curToken(), "unexpected eof in function body");
                                return false;
                            }

                            if(isToken(Lexer::tok_bropen))
                            {
                                depth++;
                            }
                            else if(isToken(Lexer::tok_brclose))
                            {
                                depth--;
                                if(depth == 0)
                                {
                                    index++;
                                    break;
                                }
                            }
                            index++;
                        }
                    }
                    else if (!isToken(Lexer::tok_semic))
					{
                        errLine(curToken(), "expected ; after top-level function type");
						return false;
					}
				}
				eatSemic();

				return true;
			}
			else if(t.Is(Lexer::tok___attribute__))
			{
				index++;

				if(!isTokenList({Lexer::tok_paropen, Lexer::tok_paropen}))
				{
					errLine(curToken(), "expected (( after __attribute__");
					return false;
				}
				index += 2;

				std::vector<Lexer::TokenState> attrTokens;

				int parStack = 0;
				while(true)
				{
					attrTokens.push_back(curToken());

					if(curToken().Is(Lexer::tok_paropen))
					{
						index++;
						parStack++;
					}
					else if(curToken().Is(Lexer::tok_parclose))
					{
						index++;
						if(parStack == 0 && curToken().Is(Lexer::tok_parclose))
						{
							index++;
							attrTokens.pop_back();
							break;
						}

						parStack--;
						if(parStack < 0)
						{
							errLine(curToken(), "mismatched parens");
						}
					}
					else
					{
						index++;
					}
				}

				// TODO: attach attribute tokens to the function?
			}
			else
			{
				__debugbreak();
			}
		}
		return false;
	}

	bool LoadModel(TypeManager& typeManager)
	{
		//Add all base struct/union types first to avoid errors later
		for (auto& su : model.structUnions)
		{
			auto success = su.isunion ? typeManager.AddUnion(owner, su.name) : typeManager.AddStruct(owner, su.name);
			if (!success)
			{
				//TODO properly handle errors
				dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add %s %s;\n"), su.isunion ? "union" : "struct", su.name.c_str());
				su.name.clear(); //signal error
				return false;
			}
		}

		//Add simple typedefs
		for (auto& type : model.types)
		{
			// TODO: support pointers
			if (type.type.isPointer() || type.type.isConst)
				__debugbreak();
			auto success = typeManager.AddType(owner, type.type.name, type.name);
			if (!success)
			{
				//TODO properly handle errors
				dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add typedef %s %s;\n"), type.type.pretty().c_str(), type.name.c_str());
				return false;
			}
		}

		//Add enums
		for (auto& kv : model.enums)
		{
			auto& e = kv.first;
			const auto& etype = kv.second;
			auto success = typeManager.AddEnum(owner, e.name, etype);
			if (!success)
			{
				dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add enum %s;\n"), e.name.c_str());
				e.name.clear(); // signal error
				return false;
			}
			else
			{
				for (const auto& v : e.values)
				{
					if (!typeManager.AddEnumerator(e.name, v.name, v.value))
					{
						dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add enum member %s.%s = %llu;\n"), e.name.c_str(), v.name.c_str(), v.value);
						return false;
					}
				}
			}
		}

		//Add base function types to avoid errors later
		for (auto& function : model.functions)
		{
			auto success = typeManager.AddFunction(owner, function.name, function.rettype, function.callconv, function.noreturn, function.typeonly);
			if (!success)
			{
				//TODO properly handle errors
				dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add function %s %s()\n"), function.rettype.pretty().c_str(), function.name.c_str());
				function.name.clear(); //signal error
				return false;
			}
		}

		//Add struct/union members
		for (auto& su : model.structUnions)
		{
			if (su.name.empty()) //skip error-signalled structs/unions
				continue;
			for (auto& member : su.members)
			{
				auto success = typeManager.AddMember(su.name, member.type, member.name, member.arrsize, member.offset);
				if (!success)
				{
					//TODO properly handle errors
					dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add member %s %s.%s;\n"), member.type.pretty().c_str(), su.name.c_str(), member.name.c_str());
					return false;
				}
			}
		}

		//Add function arguments
		for (auto& function : model.functions)
		{
			if (function.name.empty()) //skip error-signalled functions
				continue;
			for (size_t i = 0; i < function.args.size(); i++)
			{
				auto& arg = function.args[i];
				if (arg.name.empty())
					arg.name = "__unnamed_arg_" + std::to_string(i);
				auto success = typeManager.AddArg(function.name, arg.type, arg.name);
				if (!success)
				{
					//TODO properly handle errors
					dprintf(QT_TRANSLATE_NOOP("DBG", "Failed to add argument %s[%zu]: %s %s;\n"), function.name.c_str(), i, arg.type.pretty().c_str(), arg.name.c_str());
					return false;
				}
			}
		}

		return true;
	}

	bool Parse()
	{
		std::string error;
		if (!lexer.DoLexing(tokens, error))
		{
			errors.push_back(error);
			return false;
		}

		while (!isToken(Lexer::tok_eof))
		{
			auto curIndex = index;
			if (!parseTypedef())
				return false;
			if (!parseStructUnion(false))
				return false;
			if (!parseEnum(false))
				return false;
			eatSemic();
			if (curIndex == index)
			{
				Function fn;
				if (!parseFunctionTop(fn, false))
					return false;
				model.functions.push_back(fn);
			}
		}
		return true;
	}
};

bool TypeManager::ParseTypes(const std::string& code, const std::string& owner, std::vector<std::string>& errors)
{
	Parser p(code, owner, errors);
	return p.Parse() && p.LoadModel(*this);
}

bool Types::ParseModel(const std::string& code, const std::string& owner, std::vector<std::string>& errors, Model& model)
{
	Parser p(code, owner, errors);
	if (!p.Parse())
		return false;
	model = std::move(p.model);
	return true;
}
