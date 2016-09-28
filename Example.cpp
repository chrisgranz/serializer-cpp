/*
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

#include "Serializer.hpp"
#include "SerializerJSON.hpp"

#include <cstdio>
#include <vector>

static const char* g_JSON =
"[ \"hello\", \"world\", \"json\" ]";

struct Vec3 { float x, y, z; };

int main()
{
	SerializerJSON serializer;
	ParserJSON parser(g_JSON);
	auto root = parser.GetRoot();

	// register our types
	SERIALIZER_REGISTER_TYPE(serializer, Vec3, SerializerJSON::TEXT_EXPORT_MINIMAL);
	SERIALIZER_REGISTER_TYPE_MEMBER(serializer, Vec3, x, SerializerJSON::TEXT_EXPORT_NO_NAMES);
	SERIALIZER_REGISTER_TYPE_MEMBER(serializer, Vec3, y, SerializerJSON::TEXT_EXPORT_NO_NAMES);
	SERIALIZER_REGISTER_TYPE_MEMBER(serializer, Vec3, z, SerializerJSON::TEXT_EXPORT_NO_NAMES);

	SERIALIZER_REGISTER_TYPE(serializer, std::vector<std::string>, 0);
	SERIALIZER_REGISTER_TYPE(serializer, std::vector<Vec3>, 0);

	// run some tests
	std::vector<Vec3> v1{{2,3,1}, {2,66,4}, {9,8,7}};
	std::vector<std::string> v2{"hello", "world", "sweet"};
	Vec3 a{1,2,3};

	serializer.JSONWrite(stdout, &v1);
	printf("\n");

	serializer.JSONWrite(stdout, &v2);
	printf("\n");

	serializer.JSONWrite(stdout, &a, "test");
	printf("\n");

	root->Print();
	printf("\n\n");

	return 0;
}

