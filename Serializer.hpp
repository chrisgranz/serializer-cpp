
#pragma once

#include <type_traits>
#include <vector>
#include <string>
#include <utility>
#include <map>
#include <unordered_map>

#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstddef>

namespace RTTI {

///////////////////////////////////////////////////////////////////////////////
template <typename T> struct TypeCounter { static int NextTypeID; };
template <typename T> int TypeCounter<T>::NextTypeID(0);

///////////////////////////////////////////////////////////////////////////////
/// Helper class which stores type info and increments global counters.
///////////////////////////////////////////////////////////////////////////////
struct TypeInfo
{
	const int TypeID; //< unique type ID
	inline TypeInfo(int& counter) : TypeID(counter) { ++counter; }
};

///////////////////////////////////////////////////////////////////////////////
template < typename D, typename B >
struct Base : public B
{
	static const TypeInfo RTTI;
	Base() : B(RTTI.TypeID) { }
	virtual ~Base() { }
};

template < typename D, typename B >
const TypeInfo Base< D, B >::RTTI(TypeCounter<B>::NextTypeID);

///////////////////////////////////////////////////////////////////////////////
/// This is a simple type wrapper which allows for passing and storing type
/// ID from template functions to non-template functions or store in generic
/// data structures.
///////////////////////////////////////////////////////////////////////////////
struct WrapperBase
{
	const int TypeID;
	inline explicit WrapperBase(int typeID) : TypeID(typeID) { }
	inline virtual ~WrapperBase() { }
};

template <typename T> class Wrapper final
	: public Base< Wrapper<T>, WrapperBase >
{ };

} // namespace RTTI

///////////////////////////////////////////////////////////////////////////////
class Serializer
{
protected:
	///////////////////////////////////////////////////////////////////////////
	// Helper template and base type when dealing with vector member types
	///////////////////////////////////////////////////////////////////////////
	struct VectorTypeDispatcherBase
	{
		inline virtual ~VectorTypeDispatcherBase() { }
		virtual size_t size(const void* obj) const = 0;
		virtual const unsigned char* base(const void* obj) const = 0;
		virtual unsigned char* base(void* obj) const = 0;
		virtual void reserve(void* obj, size_t s) const = 0;
		virtual void resize(void* obj, size_t s) const = 0;
	};

	///////////////////////////////////////////////////////////////////////////
	template<typename T>
	struct VectorTypeDispatcher : public VectorTypeDispatcherBase
	{
		inline virtual ~VectorTypeDispatcher() { }

		inline virtual size_t size(const void* obj) const
		{
			assert(obj != nullptr);
			return static_cast<const std::vector<T>*>(obj)->size();
		}

		inline virtual const unsigned char* base(const void* obj) const
		{
			assert(obj != nullptr);
			return (const unsigned char*)&(*(static_cast<const std::vector<T>*>(obj)))[0];
		}

		inline virtual unsigned char* base(void* obj) const
		{
			assert(obj != nullptr);
			return (unsigned char*)&(*(static_cast<std::vector<T>*>(obj)))[0];
		}

		inline virtual void reserve(void* obj, size_t s) const
		{
			assert(obj != nullptr);
			static_cast<std::vector<T>*>(obj)->reserve(s);
		}

		inline virtual void resize(void* obj, size_t s) const
		{
			assert(obj != nullptr);
			static_cast<std::vector<T>*>(obj)->resize(s);
		}
	};

	///////////////////////////////////////////////////////////////////////////
	// enum definition data
	///////////////////////////////////////////////////////////////////////////
	struct EnumDefData
	{
		std::string name;
		int typeID;
		std::unordered_map<std::string, int> nameKeyMembers;  // map of defined enum name-value pairs
		std::unordered_map<int, std::string> valueKeyMembers; // map of defined enum value-name pairs (so find by value)
	};

public:
	///////////////////////////////////////////////////////////////////////////
	static const unsigned int MAX_NESTED_DEPTH = 25;

	///////////////////////////////////////////////////////////////////////////
	enum class ComplexType
	{
		None = 0,
		Enum,
		Struct,
		Vector,
	};

	///////////////////////////////////////////////////////////////////////////
	static const uint TEXT_EXPORT_NO_NAMES = (1 << 0);
	static const uint TEXT_EXPORT_SINGLE_LINE = (1 << 1);
	static const uint TEXT_EXPORT_MINIMAL = (1 << 2);

	using AttribFlags = unsigned int;

	///////////////////////////////////////////////////////////////////////////
	// Data for one struct or vector member
	///////////////////////////////////////////////////////////////////////////
	struct MemberData
	{
		std::string name;  // name for loading and writing

		size_t byteOffset; // offset inside the structure in bytes
		int typeID;        // member type from RTTI::Wrapper<T>::RTTI.TypeID
		size_t typeSize;   // size in bytes of the member

		ComplexType complexType;                    // ComplexType::None if this is a primitive member
		std::vector<MemberData*> members;           // data for sub-members if this is not a primitive member
		VectorTypeDispatcherBase* vectorDispatcher; // only used for vectors

		AttribFlags attribFlags;                    // attributes for this member

		///////////////////////////////////////////////////////////////////////
		inline MemberData()
			:
			name("NO_NAME"),
			byteOffset(0),
			typeID(-1),
			typeSize(0),
			complexType(ComplexType::None),
			vectorDispatcher(nullptr),
			attribFlags(0)
		{ }

		///////////////////////////////////////////////////////////////////////
		inline MemberData(
			const char* name,
			size_t byteOffset,
			int typeID,
			size_t typeSize,
			ComplexType complexType,
			VectorTypeDispatcherBase* vectorDispatcher,
			AttribFlags attribFlags)
			:
			name(name),
			byteOffset(byteOffset),
			typeID(typeID),
			typeSize(typeSize),
			complexType(complexType),
			vectorDispatcher(vectorDispatcher),
			attribFlags(attribFlags)
		{ }

		///////////////////////////////////////////////////////////////////////
		inline MemberData(const MemberData& rhs)
			:
			name(rhs.name),
			byteOffset(rhs.byteOffset),
			typeID(rhs.typeID),
			typeSize(rhs.typeSize),
			complexType(rhs.complexType),
			vectorDispatcher(rhs.vectorDispatcher),
			attribFlags(rhs.attribFlags)
		{
			for (auto& m : rhs.members)
			{
				assert(m != nullptr);
				members.push_back(new MemberData(*m));
			}
		}

		///////////////////////////////////////////////////////////////////////
		inline MemberData& operator=(const MemberData& rhs)
		{
			name = rhs.name;
			byteOffset = rhs.byteOffset;
			typeID = rhs.typeID;
			typeSize = rhs.typeSize;
			complexType = rhs.complexType;
			vectorDispatcher = rhs.vectorDispatcher;
			attribFlags = rhs.attribFlags;

			for (auto& m : rhs.members)
			{
				assert(m != nullptr);
				members.push_back(new MemberData(*m));
			}

			return *this;
		}

		///////////////////////////////////////////////////////////////////////
		inline ~MemberData()
		{
			for (auto& m : members)
			{
				assert(m != nullptr);
				delete m;
			}
		}
	};

	///////////////////////////////////////////////////////////////////////////
	// Status information used during/after loading
	///////////////////////////////////////////////////////////////////////////
	enum class LoadStatus
	{
		NotYetLoaded = 0,
		Loaded,
		Missing,
		BadFormat,
		MaxNestDepthExceeded
	};

	///////////////////////////////////////////////////////////////////////////
	class LoadStatusInfo
	{
	public:
		LoadStatus m_loadStatus;
		LoadStatusInfo* m_subInfo;
		size_t m_subInfoSize;

		inline LoadStatusInfo(LoadStatus loadStatus)
			:
			m_loadStatus(loadStatus),
			m_subInfo(nullptr),
			m_subInfoSize(0)
		{ }

		inline LoadStatusInfo()
			:
			m_loadStatus(LoadStatus::NotYetLoaded),
			m_subInfo(nullptr),
			m_subInfoSize(0)
		{ }

		LoadStatusInfo(const LoadStatusInfo& rhs) = delete;
		LoadStatusInfo& operator=(const LoadStatusInfo& rhs) = delete;

		inline LoadStatusInfo(LoadStatusInfo&& rhs)
			:
			m_loadStatus(rhs.m_loadStatus),
			m_subInfo(rhs.m_subInfo),
			m_subInfoSize(rhs.m_subInfoSize)
		{
			rhs.m_loadStatus = LoadStatus::NotYetLoaded;
			rhs.m_subInfo = nullptr;
			rhs.m_subInfoSize = 0;
		}

		inline LoadStatusInfo& operator=(LoadStatusInfo&& rhs)
		{
			m_loadStatus = rhs.m_loadStatus;
			m_subInfo = rhs.m_subInfo;
			m_subInfoSize = rhs.m_subInfoSize;
			rhs.m_loadStatus = LoadStatus::NotYetLoaded;
			rhs.m_subInfo = nullptr;
			rhs.m_subInfoSize = 0;
			return *this;
		}

		inline ~LoadStatusInfo()
		{
			if (m_subInfo != nullptr)
				delete[] m_subInfo;
		}

		inline LoadStatus Status() const { return m_loadStatus; }

		inline const LoadStatusInfo& SubInfo(size_t i) const
		{
			assert(i < m_subInfoSize);
			return m_subInfo[i];
		}
	};

protected:
	///////////////////////////////////////////////////////////////////////////
	std::unordered_map<int, MemberData> m_structDefs; // table of defined structures
	std::unordered_map<int, EnumDefData> m_enumDefs;  // table of defined enums

	std::vector<VectorTypeDispatcherBase*> m_vectorDispatchers;

protected:
	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	struct ComplexTypeHelper
	{
		///////////////////////////////////////////////////////////////////////
		static inline bool BuildMember(Serializer& sds, MemberData& m, const char* name, size_t offset, AttribFlags flags)
		{
			assert(name != nullptr);
			assert(name[0] != '\0');

			auto id = RTTI::Wrapper<T>::RTTI.TypeID;

			m.name = name;
			m.byteOffset = offset;
			m.typeID = id;
			m.typeSize = sizeof(T);
			m.vectorDispatcher = nullptr;
			m.attribFlags = flags;

			// NOTE: we treat strings as primitives, so exclude them here
			if (std::is_class<T>::value && id != RTTI::Wrapper<std::string>::RTTI.TypeID)
			{
				assert(sds.m_structDefs.find(id) != sds.m_structDefs.end());
				m.complexType = ComplexType::Struct;
				m.attribFlags |= sds.m_structDefs[id].attribFlags;

				for (auto& subm : sds.m_structDefs[id].members)
				{
					assert(subm != nullptr);
					m.members.push_back(new MemberData(*subm));
				}

				return true;
			}

			if (std::is_enum<T>::value)
			{
				assert(sds.m_enumDefs.find(id) != sds.m_enumDefs.end());
				m.complexType = ComplexType::Enum;
				//m.attribFlags |= sds.m_enumDefs[childID].attribFlags;
				return true;
			}

			// otherwise it is a primitive child member
			m.complexType = ComplexType::None;
			return true;
		}

		///////////////////////////////////////////////////////////////////////
		static inline bool BuildChildMember(Serializer& sds, MemberData& parent, const char* name, size_t offset, AttribFlags flags)
		{
			assert(offset < parent.typeSize
				&& "Byte offset into data structure is beyond the end of known size--data corruption likely!");
#ifndef NDEBUG
			// check that the member doesn't already exist
			assert(name != nullptr);
			assert(name[0] != '\0');

			for (auto m : parent.members)
			{
				assert(m != nullptr);

				if (m->name.compare(name) == 0)
				{
					assert(false && "Struct member with given name already exists in registry");
					return false;
				}
			}
#endif
			parent.members.push_back(new MemberData);
			auto m = parent.members.back();
			return BuildMember(sds, *m, name, offset, flags);
		}
	};

	///////////////////////////////////////////////////////////////////////////
	/// Specialized for vector<T> types 
	///////////////////////////////////////////////////////////////////////////
	template <typename ElementT>
	struct ComplexTypeHelper< std::vector<ElementT> >
	{
		///////////////////////////////////////////////////////////////////////
		static inline bool BuildMember(Serializer& sds, MemberData& m, const char* name, size_t offset, AttribFlags flags)
		{
			assert(name != nullptr);
			assert(name[0] != '\0');

			auto id = RTTI::Wrapper<ElementT>::RTTI.TypeID;

			// FIXME: we really don't need to add a vector dispatcher object for the same vector<T> each time
			sds.m_vectorDispatchers.push_back(new VectorTypeDispatcher<ElementT>);

			m.name = name;
			m.byteOffset = offset;
			m.typeID = id;
			m.typeSize = sizeof(ElementT);
			m.complexType = ComplexType::Vector;
			m.vectorDispatcher = sds.m_vectorDispatchers.back();
			m.attribFlags = flags;

			/*
			if (sds.m_structDefs.find(id) != sds.m_structDefs.end())
			{
				m.attribFlags |= sds.m_structDefs[id].attribFlags;

				for (auto& subm : sds.m_structDefs[id].members)
				{
					assert(subm != nullptr);
					m.members.push_back(HALCYON_NEW MemberData(*subm));
				}
			}
			*/

			return ComplexTypeHelper< ElementT >::BuildChildMember(sds, m, "vector<T>_subtype", 0, flags);
		}

		///////////////////////////////////////////////////////////////////////
		static inline bool BuildChildMember(Serializer& sds, MemberData& parent, const char* name, size_t offset, AttribFlags flags)
		{
			assert(offset < parent.typeSize
				&& "Byte offset into data structure is beyond the end of known size--data corruption likely!");

			parent.members.push_back(new MemberData);
			auto m = parent.members.back();
			return BuildMember(sds, *m, name, offset, flags);
		}
	};

	///////////////////////////////////////////////////////////////////////////
	inline bool IsPrimitive(int typeID)
	{
		if (typeID == RTTI::Wrapper<char>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<unsigned char>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<int16_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<uint16_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<int32_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<uint32_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<int64_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<uint64_t>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<float>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<double>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<bool>::RTTI.TypeID
			|| typeID == RTTI::Wrapper<std::string>::RTTI.TypeID)
			return true;

		return false;
	}

	///////////////////////////////////////////////////////////////////////////
	inline void PrintPrimitive(FILE* fp, const unsigned char* data, int typeID)
	{
		assert(fp != nullptr);
		assert(data != nullptr);

		if (typeID == RTTI::Wrapper<bool>::RTTI.TypeID)
			fprintf(fp, "'%s'", *((const bool*)data) ? "true" : "false");
		else if (typeID == RTTI::Wrapper<char>::RTTI.TypeID)
			fprintf(fp, "'%c'", *((const char*)data));
		else if (typeID == RTTI::Wrapper<unsigned char>::RTTI.TypeID)
			fprintf(fp, "'%c'", *((const unsigned char*)data));
		else if (typeID == RTTI::Wrapper<int16_t>::RTTI.TypeID)
			fprintf(fp, "%d", *((int16_t*)data));
		else if (typeID == RTTI::Wrapper<uint16_t>::RTTI.TypeID)
			fprintf(fp, "%u", *((const uint16_t*)data));
		else if (typeID == RTTI::Wrapper<int32_t>::RTTI.TypeID)
			fprintf(fp, "%d", *((const int32_t*)data));
		else if (typeID == RTTI::Wrapper<uint32_t>::RTTI.TypeID)
			fprintf(fp, "%u", *((const uint32_t*)data));
		else if (typeID == RTTI::Wrapper<int64_t>::RTTI.TypeID)
			fprintf(fp, "%ld", (long int)*((const int64_t*)data));
		else if (typeID == RTTI::Wrapper<uint64_t>::RTTI.TypeID)
			fprintf(fp, "%lu", (long unsigned)*((const uint64_t*)data));
		else if (typeID == RTTI::Wrapper<float>::RTTI.TypeID)
			fprintf(fp, "%f", *((const float*)data));
		else if (typeID == RTTI::Wrapper<double>::RTTI.TypeID)
			fprintf(fp, "%f", *((const double*)data));
		else if (typeID == RTTI::Wrapper<std::string>::RTTI.TypeID)
			fprintf(fp, "\"%s\"", ((const std::string*)data)->c_str());
		else
			assert(false && "Unknown primitive type");
	}

public:
	///////////////////////////////////////////////////////////////////////////
	inline Serializer() { }

	///////////////////////////////////////////////////////////////////////////
	Serializer(const Serializer& rhs) = delete;
	Serializer& operator=(const Serializer& rhs) = delete;

	///////////////////////////////////////////////////////////////////////////
	inline ~Serializer()
	{
		Clear();
	}

	///////////////////////////////////////////////////////////////////////////
	inline void Clear()
	{
		for (auto& vd : m_vectorDispatchers)
			delete vd;

		m_vectorDispatchers.clear();
		m_enumDefs.clear();
		m_structDefs.clear();
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline int RegisterType(const char* name, AttribFlags flags = 0)
	{
		static_assert(std::is_class<T>::value == true
			|| std::is_enum<T>::value == true,
			"Type should be a class or enum type (not a primitive)");
		static_assert(std::is_enum<T>::value == false
			|| sizeof(T) == sizeof(int),
			"Non-integer enum types are not supported");
		static_assert(std::is_pointer<T>::value == false,
			"Type should not be a pointer type");
		//static_assert(std::is_standard_layout<T>::value == true
		//	&& std::is_trivial<StructT>::value == true,
		//	"Only POD types are supported");

		assert(name != nullptr);
		assert(name[0] != '\0');

		int id = RTTI::Wrapper<T>::RTTI.TypeID;

		if (std::is_enum<T>::value == true)
		{
			assert(m_enumDefs.find(id) == m_enumDefs.end()
				&& "An enum type with the given name has already been added");

			auto& s = m_enumDefs[id];
			s.name = name;
			s.typeID = id;

			return id;
		}

		// otherwise it is struct of vector type
		assert(m_structDefs.find(id) == m_structDefs.end()
			&& "A type with the given name has already been added");

		auto& s = m_structDefs[id];
		ComplexTypeHelper< T >::BuildMember(*this, s, name, 0, flags);
		return id;
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline void UnregisterType()
	{
		static_assert(std::is_class<T>::value == true
			|| std::is_enum<T>::value == true,
			"Type should be a class or enum type (not a primitive)");
		static_assert(std::is_pointer<T>::value == false,
			"T should not be a pointer type");

		int id = RTTI::Wrapper<T>::RTTI.TypeID;

		// handle enums
		if (std::is_enum<T>::value == true)
		{
			assert(m_enumDefs.find(id) != m_enumDefs.end());
			m_enumDefs.erase(id);
			return;
		}

		// otherwise it is a struct or vector type
		assert(m_structDefs.find(id) != m_structDefs.end());
		m_structDefs.erase(id);
	}

	///////////////////////////////////////////////////////////////////////////
	inline void UnregisterAllTypes()
	{
		m_enumDefs.clear();
		m_structDefs.clear();

		for (auto& vd : m_vectorDispatchers)
			delete vd;

		m_vectorDispatchers.clear();
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename EnumT>
	inline bool RegisterTypeMember(const char* name, EnumT value, AttribFlags flags = 0)
	{
		static_assert(std::is_enum<EnumT>::value == true,
			"Type should be an enum type");
		static_assert(sizeof(EnumT) == sizeof(int),
			"Non-integer enum types are not supported");

		int id = RTTI::Wrapper<EnumT>::RTTI.TypeID;
		assert(m_enumDefs.find(id) != m_enumDefs.end());

		assert(name != nullptr);
		assert(name[0] != '\0');

		auto it = m_enumDefs.find(id);

		if (it == m_enumDefs.end()) // couldn't find the enum
		{
			assert(false && "Couldn't find enum to add member to");
			return false;
		}

		auto& e = m_enumDefs[id];
		e.nameKeyMembers.insert(std::pair<std::string, int>(name, (int)value));
		e.valueKeyMembers.insert(std::pair<int, std::string>((int)value, name));

		return true;
	}

	///////////////////////////////////////////////////////////////////////////
	template <typename ParentStructT, typename T>
	inline bool RegisterTypeMember(const char* name, size_t offset, AttribFlags flags = 0)
	{
		static_assert(std::is_class<ParentStructT>::value == true,
			"Parent type should be a struct type");
		auto parentID = RTTI::Wrapper<ParentStructT>::RTTI.TypeID;
		assert(m_structDefs.find(parentID) != m_structDefs.end());
		auto& parent = m_structDefs[parentID];
		return ComplexTypeHelper< T >::BuildChildMember(*this, parent, name, offset, flags);
	}
};

///////////////////////////////////////////////////////////////////////////////
// convenience macros for registering new types
///////////////////////////////////////////////////////////////////////////////
#define SERIALIZER_REGISTER_TYPE(collection, structtype, flags) \
	collection.RegisterType< structtype >(#structtype, flags)
#define SERIALIZER_REGISTER_ENUM_TYPE_MEMBER(collection, structtype, membername, flags) \
	collection.RegisterTypeMember< structtype >(#membername, structtype::membername, flags)
#define SERIALIZER_REGISTER_TYPE_MEMBER(collection, structtype, membername, flags) \
	collection.RegisterTypeMember< structtype, decltype(structtype::membername) >(#membername, offsetof(structtype, membername), flags)

