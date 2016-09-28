
#pragma once

#include "Serializer.hpp"
#include "ParserJSON.hpp"

class SerializerJSON : public Serializer
{
private:
	///////////////////////////////////////////////////////////////////////////
	inline LoadStatusInfo JSONLoadPrimitive(
		unsigned char* data,
		const char* name,
		int typeID,
		const ParserJSON::Node* node)
	{
		assert(data != nullptr);
		assert(name != nullptr);
		assert(node != nullptr);

		/*
		if (typeID == RTTI::Wrapper<char>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToString())
			{
				printf("SerializerJSON: Node '%s' is not convertable to string for 'char' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((char*)data) = obj.ToString()[0];
		}
		else if (typeID == RTTI::Wrapper<unsigned char>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToString())
			{
				printf("SerializerJSON: Node '%s' is not convertable to string for 'uchar' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((uchar*)data) = (unsigned char)obj.ToString()[0];
		}
		else if (typeID == RTTI::Wrapper<int16_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'int16_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((int16_t*)data) = (int16_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<uint16_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'uint16_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((uint16_t*)data) = (uint16_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<int32_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'int32_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((int32_t*)data) = (int32_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<uint32_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'uint32_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((uint32_t*)data) = (uint32_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<int64_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'int64_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((int64_t*)data) = (int64_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<uint64_t>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToInteger())
			{
				printf("SerializerJSON: Node '%s' is not convertable to integer for 'uint64_t' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((int64_t*)data) = (int64_t)obj.ToInteger();
		}
		else if (typeID == RTTI::Wrapper<float>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToNumber())
			{
				printf("SerializerJSON: Node '%s' is not convertable to number for 'float' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((float*)data) = (float)obj.ToNumber();
		}
		else if (typeID == RTTI::Wrapper<double>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToNumber())
			{
				printf("SerializerJSON: Node '%s' is not convertable to number for 'double' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((double*)data) = (double)obj.ToNumber();
		}
		else if (typeID == RTTI::Wrapper<bool>::RTTI.TypeID)
		{
			if (!obj.IsBoolean())
			{
				printf("SerializerJSON: Node '%s' is not bool for 'bool' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((bool*)data) = obj.GetBoolean();
		}
		else if (typeID == RTTI::Wrapper<string>::RTTI.TypeID)
		{
			if (!obj.IsConvertibleToString())
			{
				printf("SerializerJSON: Node '%s' is not convertable to string for 'string' primitive", name);
				return LoadStatusInfo(LoadStatus::BadFormat);
			}

			*((std::string*)data) = obj.ToString();
		}
		else // unknown type
		{
			assert(false & "Unknown primitive type");
			return LoadStatusInfo(LoadStatus::BadFormat);
		}
*/

		return LoadStatusInfo(LoadStatus::Loaded);
	}

	///////////////////////////////////////////////////////////////////////////
	inline LoadStatusInfo JSONLoadHelper(
		unsigned char* data,
		const char* name,
		int typeID,
		ComplexType complexType,
		const VectorTypeDispatcherBase* vectorDispatcher,
		const std::vector<MemberData*>* members,
		size_t typeSize,
		const ParserJSON::Node* node,
		unsigned int nestedDepth)
	{
		assert(data != nullptr);
		//assert(node != nullptr);

		if (node == nullptr)
		{
			printf("SerializerJSON: Node '%s' not found", name);
			return LoadStatusInfo(LoadStatus::Missing);
		}

		if (nestedDepth > MAX_NESTED_DEPTH)
		{
			printf("SerializerJSON: Max nested depth exceeded");
			return LoadStatusInfo(LoadStatus::MaxNestDepthExceeded);
		}

		// check for complexType types first
		if (complexType == ComplexType::Enum)
			return JSONLoadEnum(data, name, typeID, node);
		else if (complexType == ComplexType::Struct)
			return JSONLoadStruct(data, name, typeID, node, nestedDepth);
		else if (complexType == ComplexType::Vector)
			return JSONLoadVector(data, name, typeID, vectorDispatcher, members, typeSize, node, nestedDepth);

		// otherwise it is a primitive type
		assert(complexType == ComplexType::None);
		return JSONLoadPrimitive(data, name, typeID, node);
	}

	///////////////////////////////////////////////////////////////////////////
	inline LoadStatusInfo JSONLoadEnum(
		unsigned char* data,
		const char* name,
		int typeID,
		const ParserJSON::Node* node)
	{
		assert(data != nullptr);
		assert(name != nullptr);
		assert(node != nullptr);

		assert(m_enumDefs.find(typeID) != m_enumDefs.end());
		auto& subEnum = m_enumDefs[typeID];

		if (node->type != ParserJSON::DataType::String)
		{
			printf("SerializerJSON: Node '%s' is not convertable to string for enum lookup", name);
			return LoadStatusInfo(LoadStatus::BadFormat);
		}

		auto key = node->data.c_str();
		auto it = subEnum.nameKeyMembers.find(key);

		if (it == subEnum.nameKeyMembers.end())
		{
			printf("SerializerJSON: Node '%s' enum not found for '%s'", name, key);
			return LoadStatusInfo(LoadStatus::Missing);
		}

		*((int*)data) = it->second;
		return LoadStatusInfo(LoadStatus::Loaded);
	}

	///////////////////////////////////////////////////////////////////////////
	inline LoadStatusInfo JSONLoadStruct(
		unsigned char* data,
		const char* name,
		int typeID,
		const ParserJSON::Node* node,
		unsigned int nestedDepth)
	{
		assert(data != nullptr);
		assert(name != nullptr);

		if (node == nullptr)
		{
			printf("SerializerJSON: Node '%s' is not an object for struct loading", name);
			return LoadStatusInfo(LoadStatus::BadFormat);
		}

		assert(m_structDefs.find(typeID) != m_structDefs.end());
		auto& s = m_structDefs[typeID];

		assert(s.complexType == ComplexType::Struct);

		LoadStatusInfo loadStatusInfo;
		loadStatusInfo.m_loadStatus = LoadStatus::Loaded;
		loadStatusInfo.m_subInfo = new LoadStatusInfo[s.members.size()];
		loadStatusInfo.m_subInfoSize = s.members.size();

		bool allMembersMissing = true;
		size_t i = 0;

		for (auto& m : s.members)
		{
			assert(m != nullptr);
			auto subNode = node->GetChild(m->name.c_str());

			std::string compositeName = name;

			if (compositeName.length() > 0)
				compositeName += ".";

			compositeName += m->name.c_str();

			loadStatusInfo.m_subInfo[i] = JSONLoadHelper(
				&data[m->byteOffset],
				compositeName.c_str(),
				m->typeID,
				m->complexType,
				m->vectorDispatcher,
				&m->members,
				m->typeSize,
				subNode,
				(nestedDepth + 1));

			if (loadStatusInfo.m_subInfo[i].Status() == LoadStatus::Loaded)
				allMembersMissing = false;

			++i;
		}

		// if there were no tags, let's try loading this struct in sequence (like a vector) instead
		if (allMembersMissing)
		{
			i = 0;
			// FIXME: not yet implemented
		}

		return loadStatusInfo;
	}

	///////////////////////////////////////////////////////////////////////////
	inline LoadStatusInfo JSONLoadVector(
		unsigned char* data,
		const char* name,
		int typeID,
		const VectorTypeDispatcherBase* vectorDispatcher,
		const std::vector<MemberData*>* members,
		size_t typeSize,
		const ParserJSON::Node* node,
		unsigned int nestedDepth)
	{
		if (node->type != ParserJSON::DataType::Array)
		{
			printf("SerializerJSON: Node '%s' is not an array for vector loading", name);
			return LoadStatusInfo(LoadStatus::BadFormat);
		}

		auto count = node->children.size();
		auto stride = typeSize;

		LoadStatusInfo loadStatusInfo;
		loadStatusInfo.m_loadStatus = LoadStatus::Loaded;
		loadStatusInfo.m_subInfo = new LoadStatusInfo[100]; // 100 seems reasonable to start with
		loadStatusInfo.m_subInfoSize = 100;
		size_t i = 0;

		assert(vectorDispatcher != nullptr);

		vectorDispatcher->resize(data, count);
		auto base = vectorDispatcher->base(data);

		// pull out the info about the type inside the vector
		assert(members != nullptr);
		auto m = (*members)[0];
		assert(m != nullptr);

		for (auto subNode : node->children)
		{
			// need to resize loaded status info?
			if (i == loadStatusInfo.m_subInfoSize)
			{
				auto subInfo = new LoadStatusInfo[loadStatusInfo.m_subInfoSize + 100];
				delete[] loadStatusInfo.m_subInfo;
				loadStatusInfo.m_subInfo = subInfo;
				loadStatusInfo.m_subInfoSize += 100;
			}

			std::string subName = name;

			if (subName.length() > 0)
				subName += ".";

			//subName += indexName;
			subName += subNode->name;

			loadStatusInfo.m_subInfo[i++] = JSONLoadHelper(
				&base[m->byteOffset],
				subName.c_str(),
				m->typeID,
				m->complexType,
				m->vectorDispatcher,
				&m->members,
				m->typeSize,
				subNode,
				(nestedDepth + 1));

			base += stride;
		}

		return loadStatusInfo;
	}

	///////////////////////////////////////////////////////////////////////////
	inline void JSONWriteHelper(
		FILE* fp,
		const unsigned char* data,
		const char* name,
		int typeID,
		ComplexType complexType,
		const VectorTypeDispatcherBase* vectorDispatcher,
		const std::vector<MemberData*>* members,
		size_t typeSize,
		AttribFlags flags = 0,
		unsigned int indent = 0)
	{
		assert(fp != nullptr);
		assert(data != nullptr);

		assert(indent < 20 && "Too many levels of embedded structs");

		for (unsigned int i = 0; i < indent; i++)
			fprintf(fp, "\t");

		//if (!(flags & TEXT_EXPORT_NO_NAMES))
		{
			if (name != nullptr && name[0] != '\0')
			{
				if (flags & TEXT_EXPORT_MINIMAL)
					fprintf(fp, "\"%s\":", name);
				else
					fprintf(fp, "\"%s\" : ", name);
			}
		}

		//flags |= m->attribFlags;

		if (complexType == ComplexType::Enum)
		{
			assert(m_enumDefs.find(typeID) != m_enumDefs.end());
			auto& e = m_enumDefs[typeID];

			auto val = *((int*)data);
			auto it = e.valueKeyMembers.find(val);

			if (it != e.valueKeyMembers.end())
				fprintf(fp, "\"%s\"", it->second.c_str());
			else
				fprintf(fp, "\"INVALID_ENUM\"");
		}
		else if (complexType == ComplexType::Struct)
		{
			int newIndent = indent;

			if (flags & TEXT_EXPORT_MINIMAL)
			{
				fprintf(fp, "{");
				newIndent = 0;
			}
			else if (flags & TEXT_EXPORT_SINGLE_LINE)
			{
				fprintf(fp, "{ ");
				newIndent = 0;
			}
			else
			{
				fprintf(fp, "\n");

				for (uint i = 0; i < indent; i++)
					fprintf(fp, "\t");

				fprintf(fp, "{\n");
				++newIndent;
			}

			assert(m_structDefs.find(typeID) != m_structDefs.end());
			auto& s = m_structDefs[typeID];

			assert(s.complexType == ComplexType::Struct);

			for (size_t i = 0; i < s.members.size(); i++)
			{
				auto& m = s.members[i];

				JSONWriteHelper(
					fp,
					&data[m->byteOffset],
					m->name.c_str(),
					m->typeID,
					m->complexType,
					m->vectorDispatcher,
					&m->members,
					m->typeSize,
					m->attribFlags | flags,
					newIndent);

				// don't add comma for last element
				if (i < (s.members.size() - 1))
				{
					if (flags & TEXT_EXPORT_MINIMAL)
						fprintf(fp, ",");
					else if (flags & TEXT_EXPORT_SINGLE_LINE)
						fprintf(fp, ", ");
					else
						fprintf(fp, ",\n");
				}
			}

			if (flags & TEXT_EXPORT_MINIMAL)
				fprintf(fp, "}");
			else if (flags & TEXT_EXPORT_SINGLE_LINE)
				fprintf(fp, " }");
			else
			{
				fprintf(fp, "\n");

				for (uint i = 0; i < indent; i++)
					fprintf(fp, "\t");

				fprintf(fp, "}");
			}
		}
		else if (complexType == ComplexType::Vector)
		{
			assert(vectorDispatcher != nullptr);

			const unsigned char* base = nullptr;
			size_t count = 0;
			size_t stride = typeSize;

			base = vectorDispatcher->base(data);
			count = vectorDispatcher->size(data);

			int newIndent = indent;

			if (flags & TEXT_EXPORT_MINIMAL)
			{
				fprintf(fp, "[");
				newIndent = 0;
			}
			else if (flags & TEXT_EXPORT_SINGLE_LINE)
			{
				fprintf(fp, "[ ");
				newIndent = 0;
			}
			else
			{
				fprintf(fp, "\n");

				for (uint i = 0; i < indent; i++)
					fprintf(fp, "\t");

				fprintf(fp, "[\n");
				++newIndent;
			}

			if (count > 0)
			{
				// pull out the info about the type inside the vector
				assert(members != nullptr);
				auto m = (*members)[0];
				assert(m != nullptr);

				for (size_t i = 0; i < count; i++)
				{
					JSONWriteHelper(
						fp,
						&base[m->byteOffset],
						"",
						m->typeID,
						m->complexType,
						m->vectorDispatcher,
						&m->members,
						m->typeSize,
						m->attribFlags | flags,
						newIndent);

					// don't add comma for last element
					if (i < (count - 1))
					{
						if (flags & TEXT_EXPORT_MINIMAL)
							fprintf(fp, ",");
						else if (flags & TEXT_EXPORT_SINGLE_LINE)
							fprintf(fp, ", ");
						else
							fprintf(fp, ",\n");
					}

					base += stride;
				}
			}

			if (flags & TEXT_EXPORT_MINIMAL)
				fprintf(fp, "]");
			else if (flags & TEXT_EXPORT_SINGLE_LINE)
				fprintf(fp, " ]");
			else
			{
				fprintf(fp, "\n");

				for (uint i = 0; i < indent; i++)
					fprintf(fp, "\t");

				fprintf(fp, "]");
			}
		}
		else if (IsPrimitive(typeID)) // primitive
			PrintPrimitive(fp, data, typeID);
		else
			assert(false && "Unknown type");
	}

public:
	///////////////////////////////////////////////////////////////////////////
	inline SerializerJSON() { }

	///////////////////////////////////////////////////////////////////////////
	SerializerJSON(const SerializerJSON& rhs) = delete;
	SerializerJSON& operator=(const SerializerJSON& rhs) = delete;

	///////////////////////////////////////////////////////////////////////////
	inline ~SerializerJSON() { }

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline LoadStatusInfo JSONLoad(T* data, const ParserJSON::Node* node, const char* name = "")
	{
		assert(data != nullptr);
		assert(name != nullptr);
		assert(node != nullptr);

		auto typeID = RTTI::Wrapper<T>::RTTI.TypeID;
		auto typeSize = sizeof(T);
		auto complexType = ComplexType::None;
		VectorTypeDispatcherBase* vectorDispatcher = nullptr;
		std::vector<MemberData*>* members = nullptr;

		if (m_enumDefs.find(typeID) != m_enumDefs.end()) // enum type
			complexType = ComplexType::Enum;
		else if (m_structDefs.find(typeID) != m_structDefs.end()) // struct or vector type
		{
			auto& s = m_structDefs[typeID];
			typeSize = s.typeSize;
			complexType = s.complexType;
			vectorDispatcher = s.vectorDispatcher;
			members = &s.members;
		}
		else if (IsPrimitive(typeID))
			complexType = ComplexType::None;
		else
			assert(false && "Unknown type for loading (is the type registered?)");

		return JSONLoadHelper((unsigned char*)data, name, typeID, complexType, vectorDispatcher, members, typeSize, node, 1);
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline void JSONWrite(FILE* fp, T* data, const char* name = "", AttribFlags flags = 0)
	{
		assert(fp != nullptr);
		assert(data != nullptr);
		//assert(name != nullptr);
		//assert(name[0] != '\0');

		auto typeID = RTTI::Wrapper<T>::RTTI.TypeID;
		auto typeSize = sizeof(T);
		auto complexType = ComplexType::None;
		VectorTypeDispatcherBase* vectorDispatcher = nullptr;
		std::vector<MemberData*>* members = nullptr;

		if (m_enumDefs.find(typeID) != m_enumDefs.end()) // enum type
			complexType = ComplexType::Enum;
		else if (m_structDefs.find(typeID) != m_structDefs.end()) // struct or vector type
		{
			auto& s = m_structDefs[typeID];
			typeSize = s.typeSize;
			complexType = s.complexType;
			vectorDispatcher = s.vectorDispatcher;
			members = &s.members;
			flags |= s.attribFlags;
		}
		else if (IsPrimitive(typeID))
			complexType = ComplexType::None;
		else
			assert(false && "Unknown type for writing");

		JSONWriteHelper(fp, (unsigned char*)data, name, typeID, complexType, vectorDispatcher, members, typeSize, flags);
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline bool JSONWrite(const char* filename, T* data, const char* name = "", AttribFlags flags = 0)
	{
		assert(filename != nullptr);
		assert(filename[0] != '\0');

		auto fp = fopen(filename, "a");

		if (fp == nullptr)
			return false;

		JSONWrite(fp, data, name, flags);

		fclose(fp);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline bool JSONWrite(const std::string filename, T* data, const char* name = "", AttribFlags flags = 0)
	{
		JSONWrite(filename.c_str(), data, name, flags);
	}
};

