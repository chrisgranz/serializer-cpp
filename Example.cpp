
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

