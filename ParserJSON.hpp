/*
 * SerializerCpp
 * Copyright (c) 2015-2016 Christopher D. Granz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>

///////////////////////////////////////////////////////////////////////////////
class ParserJSON
{
public:
	///////////////////////////////////////////////////////////////////////////
	// JSON data types
	///////////////////////////////////////////////////////////////////////////
	enum class DataType : unsigned char
	{
		Undefined = 0,
		Number,
		String,
		Boolean,
		Array,
		Object,
		Null,
	};

	///////////////////////////////////////////////////////////////////////////
	struct Node
	{
		DataType type;    // node type, see above
		std::string name; // node name, may be empty for array entries
		std::string data; // value if type is Number, String, Boolean, or Null
		std::vector<Node*> children; // pointers to children if type is Array or Object (data above not used in that case)

		///////////////////////////////////////////////////////////////////////
		inline Node(DataType type = DataType::Undefined)
			: type(type)
		{ }

		///////////////////////////////////////////////////////////////////////
		inline const Node* GetChild(const char* name) const
		{
			if (type != DataType::Object)
				return nullptr;

			for (auto& child : children)
			{
				if (child->name == name)
					return child;
			}

			return nullptr; // not found
		}

		///////////////////////////////////////////////////////////////////////
		inline const Node* GetChild(size_t index) const
		{
			if (type != DataType::Object && type != DataType::Array)
				return nullptr;

			if (index < children.size())
				return children[index];

			return nullptr; // not found
		}

		///////////////////////////////////////////////////////////////////////
		// For debugging purposes
		///////////////////////////////////////////////////////////////////////
		bool Print(int indentLevel = 0) const
		{
			for (int i = 0; i < indentLevel; ++i)
				printf("\t");

			if (name != "") // has a name
			{
				if (type == DataType::Array || type == DataType::Object)
				{
					printf("\"%s\" :\n", name.c_str());

					for (int i = 0; i < indentLevel; ++i)
						printf("\t");
				}
				else
					printf("\"%s\" : ", name.c_str());
			}

			switch (type)
			{
			case DataType::Undefined:
			case DataType::Number:
			case DataType::Boolean:
			case DataType::Null:
				printf("%s", data.c_str());
				break;

			case DataType::String:
				printf("\"%s\"", data.c_str());
				break;

			case DataType::Array:
				printf("[\n");

				for (decltype(children.size()) i = 0; i < children.size(); ++i)
				{
					assert(children[i] != nullptr);
					children[i]->Print(indentLevel + 1);

					if (i == (children.size() - 1))
						printf("\n");
					else
						printf(",\n");
				}

				for (int i = 0; i < indentLevel; ++i)
					printf("\t");

				printf("]");
				break;

			case DataType::Object:
				printf("{\n");

				for (decltype(children.size()) i = 0; i < children.size(); ++i)
				{
					assert(children[i] != nullptr);
					children[i]->Print(indentLevel + 1);

					if (i == (children.size() - 1))
						printf("\n");
					else
						printf(",\n");
				}

				for (int i = 0; i < indentLevel; ++i)
					printf("\t");

				printf("}");
				break;

			default: break;
			}

			return true;
		}
	};

	///////////////////////////////////////////////////////////////////////////
	enum class ParseError
	{
		None = 0,
		InternalError,
		BadFormat,
		BadNumberFormat,
		InvalidRoot,
		InvalidKey,
		MissingKeyValueSeperator,
		MissingComma,
		UnterminatedString,
		InvalidEscape,
		OutOfPlaceBrace,
		OutOfPlaceSquareBracket,
	};

private:
	std::vector<Node> m_nodes;     // we allocate nodes from here (allows for auto-cleanup)

	ParseError m_lastError;        // error code from last call to Parse()
	std::string m_lastErrorDesc;   // description of last error
	//std::string m_lastErrorLine;   // line which contains the error
	size_t m_lastErrorLineNo;         // current line number (starting at 1)
	size_t m_lastErrorCharNo;         // offset in line since last newline (starting at 1)

public:
	///////////////////////////////////////////////////////////////////////////
	inline ParserJSON()
		:
		m_lastError(ParseError::None)
	{ }

	///////////////////////////////////////////////////////////////////////////
	inline ParserJSON(std::string str, size_t reserveNodes = 100)
		:
		m_lastError(ParseError::None)
	{
		Parse(str.c_str(), reserveNodes);
	}

	///////////////////////////////////////////////////////////////////////////
	inline Node const* GetRoot()                 { return (m_nodes.size() == 0 ? nullptr : &m_nodes[0]); }
	inline ParseError GetLastError()             { return m_lastError; }
	inline const std::string& GetLastErrorDesc() { return m_lastErrorDesc; }

	///////////////////////////////////////////////////////////////////////////
	inline void PrintLastError()
	{
		printf("\n");

		switch (m_lastError)
		{
		case ParseError::None: printf("No error\n"); return;
		case ParseError::BadFormat: printf("BadFormat"); break;
		case ParseError::BadNumberFormat: printf("BadNumberFormat"); break;
		case ParseError::InvalidRoot: printf("InvalidRoot"); break;
		case ParseError::InvalidKey: printf("InvalidKey"); break;
		case ParseError::MissingKeyValueSeperator: printf("MissingKeyValueSeperator"); break;
		case ParseError::MissingComma: printf("MissingComma"); break;
		case ParseError::UnterminatedString: printf("UnterminatedString"); break;
		case ParseError::InvalidEscape: printf("InvalidEscape"); break;
		case ParseError::OutOfPlaceBrace: printf("OutOfPlaceBrace"); break;
		case ParseError::OutOfPlaceSquareBracket: printf("OutOfPlaceSquareBracket"); break;
		}

		printf(": (line %ld, char %ld) %s\n", (long)m_lastErrorLineNo, (long)m_lastErrorCharNo, m_lastErrorDesc.c_str());
	}

	///////////////////////////////////////////////////////////////////////////
	// JSON Number format: https://tools.ietf.org/html/rfc7159
	//
	// number = [minus] int [frac] [exp]
	// decimal - point = %x2E; .
	// digit1 - 9 = %x31 - 39; 1 - 9
	// e = %x65 / %x45; e E
	// exp = e [minus / plus] 1 * DIGIT
	// frac = decimal - point 1 * DIGIT
	// int = zero / (digit1 - 9 * DIGIT)
	// minus = %x2D; -
	// plus = %x2B; +
	// zero = %x30; 0
	///////////////////////////////////////////////////////////////////////////
	static inline bool IsNumber(const char* p)
	{
		// leading minus
		if (*p == '-')
			++p;

		// int part
		if (*p < '0' || *p > '9')
			return false;

		// int digits
		if (*p == '0') // zero by itself is a valid int
			++p;
		else // otherwise we have multiple digits (but no leading zero)
		{
			while (*p != '\0')
			{
				if (*p < '0' || *p > '9')
					break;

				++p;
			}
		}

		// optional fractional part
		if (*p == '.')
		{
			if (*++p == '\0')
				return false;

			while (*p != '\0')
			{
				if (*p < '0' || *p > '9')
					break;

				++p;
			}
		}

		// optional exponent part
		if (*p == 'e' || *p == 'E')
		{
			++p;

			if (*p == '+' || *p == '-')
				++p;

			if (*p == '\0')
				return false;

			while (*p != '\0')
			{
				if (*p < '0' || *p > '9')
					break;

				++p;
			}
		}

		if (*p != '\0')
			return false;

		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	static inline bool IsBoolean(const char* p) { return (!strcmp(p, "true") || !strcmp(p, "false")); }
	static inline bool IsNull(const char* p) { return !strcmp(p, "null"); }

private:
	///////////////////////////////////////////////////////////////////////////
	// Helper function to parse a JSON Number, Boolean, or Null.
	///////////////////////////////////////////////////////////////////////////
	inline int ParsePrimitive(const char* p, std::string& result)
	{
		if (p[0] == ':' || p[0] == '\t' || p[0] == '\r' || p[0] == '\n'
		  || p[0] == ' ' || p[0] == ',' || p[0] == ']' || p[0] == '}'
		  || p[0] == '\0')
		{
			m_lastError = ParseError::BadFormat;
			m_lastErrorDesc = "Unexpected end to JSON Number, Boolean, or Null";
			return -1;
		}

		for (int i = 1; p[i] != '\0'; ++i)
		{
			++m_lastErrorCharNo;

			if (p[i] < 32 || p[i] >= 127) // invalid character
				return -1;

			if (p[i] == ':' || p[i] == '\t' || p[i] == '\r' || p[i] == '\n'
			  || p[i] == ' ' || p[i] == ',' || p[i] == ']' || p[i] == '}')
			{
				result = std::string(&p[0], i);
				return (i - 1); // don't include delimiter
			}
		}

		m_lastError = ParseError::BadFormat;
		m_lastErrorDesc = "Unexpected end to JSON Number, Boolean, or Null";
		return -1; // never closed
	}

	///////////////////////////////////////////////////////////////////////////
	// Helper function to parse a JSON String.
	///////////////////////////////////////////////////////////////////////////
	inline int ParseString(const char* p, std::string& result)
	{
		if (p[0] != '\"')
		{
			m_lastError = ParseError::BadFormat;
			m_lastErrorDesc = "Unexpected start character for JSON String";
			return -1;
		}

		for (int i = 1; p[i] != '\0'; ++i)
		{
			++m_lastErrorCharNo;

			// quote indicates end of string
			if (p[i] == '\"')
			{
				result = std::string(&p[1], (i - 1));
				return i;
			}

			// control characters are not allowed in string (need escaping)
			if (p[i] == '\b' || p[i] == '\f' || p[i] == '\r' || p[i] == '\n' || p[i] == '\t')
				break;

			// backslash escape
			if (p[i] == '\\' && p[i + 1] != '\0')
			{
				++i;

				switch (p[i])
				{
				// allowed escaped symbols
				case '\"': case '/' : case '\\' : case 'b' :
				case 'f' : case 'r' : case 'n'  : case 't' :
					break;

				// escaped symbol \uXXXX
				case 'u':
					++i;

					for (int k = 0; k < 4 && p[i] != '\0'; ++k, ++i)
					{
						++m_lastErrorCharNo;

						// check for valid hexadecimal character
						if (!((p[i] >= '0' && p[i] <= '9') // 0-9
						  || (p[i] >= 'A' && p[i] <= 'F') // A-F
						  || (p[i] >= 'a' && p[i] <= 'f'))) // a-f
						{
							m_lastError = ParseError::InvalidEscape;
							m_lastErrorDesc = "Invalid escape character in JSON String";
							return -1; // error
						}
					}

					--i;
					break;

				// unexpected escape symbol
				default:
					m_lastError = ParseError::InvalidEscape;
					m_lastErrorDesc = "Invalid escape character in JSON String";
					return -1; // error
				}
			}
		}

		m_lastError = ParseError::UnterminatedString;
		m_lastErrorDesc = "Unterminated JSON String";
		return -1; // never closed
	}

public:
	///////////////////////////////////////////////////////////////////////////
	void Parse(const char* str, size_t reserveNodes = 100)
	{
		// reset everything
		m_nodes.clear();
		m_nodes.reserve(reserveNodes);
		m_lastError = ParseError::None;
		m_lastErrorDesc = "No error";
		m_lastErrorLineNo = 1;
		m_lastErrorCharNo = 1;

		// parser states
		enum class State
		{
			Root = 0,
			Key,
			Value,
			KeyValueSeparator,
			CommaOrEnd,
			Done,
		};

		//Node* root = nullptr;
		Node* curr = nullptr;
		State state = State::Root;
		std::vector<Node*> containerStack;

		for (size_t i = 0; str[i] != '\0'; ++i)
		{
			if (state == State::Done)
				break;

			// skip whitespace
			if (str[i] == ' ' || str[i] == '\t')
			{
				++m_lastErrorCharNo;
				continue;
			}

			if (str[i] == '\r')
				continue;

			if (str[i] == '\n')
			{
				++m_lastErrorLineNo;
				m_lastErrorCharNo = 1;
				continue;
			}

			// handle the character depending on the current parser state
			switch (state)
			{
			///////////////////////////////////////////////////////////////////
			case State::Root:
			{
				if (str[i] == '{')
				{
					m_nodes.push_back(Node(DataType::Object));
					m_nodes.back().name = "__rootObject";
					state = State::Key;
				}
				else if (str[i] == '[')
				{
					m_nodes.push_back(Node(DataType::Array));
					m_nodes.back().name = "__rootArray";
					state = State::Value;
				}
				else
				{
					m_lastError = ParseError::InvalidRoot;
					m_lastErrorDesc = "Root not valid JSON Object or Array";
					return; // unexpected char
				}

				containerStack.push_back(&m_nodes.back());
				break;
			}

			///////////////////////////////////////////////////////////////////
			case State::Key:
			{
				switch (str[i])
				{
				case '}':
				{
					if (containerStack.back()->type != DataType::Object)
					{
						m_lastError = ParseError::OutOfPlaceBrace;
						m_lastErrorDesc = "Out of place brace";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else if (containerStack.back()->type == DataType::Object)
						state = State::Key;
					else // parent is array
						state = State::Value;

					break;
				}

				case ']':
				{
					if (containerStack.back()->type != DataType::Array)
					{
						m_lastError = ParseError::OutOfPlaceSquareBracket;
						m_lastErrorDesc = "Out of place square bracket";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else if (containerStack.back()->type == DataType::Object)
						state = State::Key;
					else // parent is array
						state = State::Value;

					break;
				}

				case '\"':
				{
					m_nodes.push_back(Node(DataType::Undefined));
					curr = &m_nodes.back();
					auto len = ParseString(&str[i], curr->name);

					if (len == -1)
						return;

					i += len;
					state = State::KeyValueSeparator;
					break;
				}

				default:
					m_lastError = ParseError::InvalidKey;
					m_lastErrorDesc = "Key is not String";
					return;
				}

				break;
			}

			///////////////////////////////////////////////////////////////////
			case State::KeyValueSeparator:
			{
				if (str[i] != ':')
				{
					m_lastError = ParseError::MissingKeyValueSeperator;
					m_lastErrorDesc = "Missing key-value separator";
					return;
				}

				state = State::Value;
				break;
			}

			///////////////////////////////////////////////////////////////////
			case State::Value:
			{
				switch (str[i])
				{
				case '{':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::Object));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::Object;

					containerStack.back()->children.push_back(curr);
					containerStack.push_back(curr);
					curr = nullptr;
					state = State::Key;
					break;
				}

				case '[':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::Array));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::Array;

					containerStack.back()->children.push_back(curr);
					containerStack.push_back(curr);
					curr = nullptr;
					state = State::Value;
					break;
				}

				case '}':
				{
					if (containerStack.back()->type != DataType::Object)
					{
						m_lastError = ParseError::OutOfPlaceBrace;
						m_lastErrorDesc = "Out of place brace";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else if (containerStack.back()->type == DataType::Object)
						state = State::Key;
					else // parent is array
						state = State::Value;

					break;
				}

				case ']':
				{
					if (containerStack.back()->type != DataType::Array)
					{
						m_lastError = ParseError::OutOfPlaceSquareBracket;
						m_lastErrorDesc = "Out of place square bracket";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else if (containerStack.back()->type == DataType::Object)
						state = State::Key;
					else // parent is array
						state = State::Value;

					break;
				}

				case '\"':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::String));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::String;

					auto len = ParseString(&str[i], curr->data);

					if (len == -1)
						return;

					i += len;

					containerStack.back()->children.push_back(curr);
					curr = nullptr;
					state = State::CommaOrEnd;
					break;
				}

				// handle numbers
				case '-':
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::Number));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::Number;

					auto len = ParsePrimitive(&str[i], curr->data);

					if (len == -1)
						return;

					i += len;

					if (!IsNumber(curr->data.c_str()))
					{
						m_lastError = ParseError::BadNumberFormat;
						m_lastErrorDesc = "Invalid JSON Number format";
						return;
					}

					containerStack.back()->children.push_back(curr);
					curr = nullptr;
					state = State::CommaOrEnd;
					break;
				}

				// handle true/false
				case 't': case 'f':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::Boolean));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::Boolean;

					auto len = ParsePrimitive(&str[i], curr->data);

					if (len == -1)
						return;

					i += len;

					if (!IsBoolean(curr->data.c_str()))
					{
						m_lastError = ParseError::BadFormat;
						m_lastErrorDesc = "Value not JSON Number, String, Boolean, or Null";
						return;
					}

					containerStack.back()->children.push_back(curr);
					curr = nullptr;
					state = State::CommaOrEnd;
					break;
				}

				// handle null
				case 'n':
				{
					if (curr == nullptr)
					{
						m_nodes.push_back(Node(DataType::Null));
						curr = &m_nodes.back();
					}
					else
						curr->type = DataType::Null;

					auto len = ParsePrimitive(&str[i], curr->data);

					if (len == -1)
						return;

					i += len;

					if (!IsNull(curr->data.c_str()))
					{
						m_lastError = ParseError::BadFormat;
						m_lastErrorDesc = "Value not JSON Number, String, Boolean, or Null";
						return;
					}

					containerStack.back()->children.push_back(curr);
					curr = nullptr;
					state = State::CommaOrEnd;
					break;
				}

				default:
					m_lastError = ParseError::BadFormat;
					m_lastErrorDesc = "Value not JSON Number, String, Boolean, or Null";
					return;
				}

				break;
			}

			///////////////////////////////////////////////////////////////////
			case State::CommaOrEnd:
			{
				switch (str[i])
				{
				case ',':
				{
					if (containerStack.back()->type == DataType::Object)
						state = State::Key;
					else // parent is array
						state = State::Value;

					break;
				}

				case '}':
				{
					if (containerStack.back()->type != DataType::Object)
					{
						m_lastError = ParseError::OutOfPlaceBrace;
						m_lastErrorDesc = "Out of place brace";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else
						state = State::CommaOrEnd;

					break;
				}

				case ']':
				{
					if (containerStack.back()->type != DataType::Array)
					{
						m_lastError = ParseError::OutOfPlaceSquareBracket;
						m_lastErrorDesc = "Out of place square bracket";
						return;
					}

					containerStack.pop_back();

					if (containerStack.size() == 0) // root finished
						state = State::Done;
					else
						state = State::CommaOrEnd;

					break;
				}

				default:
					m_lastError = ParseError::MissingComma;
					m_lastErrorDesc = "Missing comma";
					return;
				}

				break;
			}

			///////////////////////////////////////////////////////////////////
			default: // this should never happen
				m_lastError = ParseError::InternalError;
				m_lastErrorDesc = "Internal parser state";
				return;
			}
		}
	}
};

