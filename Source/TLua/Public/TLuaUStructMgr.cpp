
#include "TLuaUStructMgr.hpp"
#include "TLuaProperty.hpp"
#include "TLuaTypes.hpp"

namespace TLua
{
	TMap<UStruct*, PropertyArray> UStructMgr::StructInfos;

	template <typename PropertyType, typename ValueType>
	class TPropertyType : public PropertyBase
	{
	public:
		TPropertyType(PropertyType* InProperty)
			: Property(InProperty)
		{
		}

		//virtual void PushValue(lua_State* State, const void* Container) override
		//{
		//	TLua::PushValue<ValueType>(State,
		//		(ValueType)PropertyInfo<PropertyType>::GetValue(Container, Property));
		//}

		virtual void SetField(lua_State* State, int Index, const void* Container) override
		{
//			FString Name = Property->GetNameCPP();
			SetTableByName(State, Index, (const char*)*Property->GetNameCPP() ,
				(ValueType)PropertyInfo<PropertyType>::GetValue(Container, Property));
		}

		virtual void GetField(lua_State* State, int Index, void* Container) override
		{
//			PropertyInfo<PropertyType>::GetValue()
			auto Value = GetTableByName<ValueType>(State, Index, 
							(const char*)*Property->GetNameCPP());
			
			Property->SetPropertyValue_InContainer(Container, Value);
		}

	private:
		PropertyType* Property;
	};

	class PropertyVisitor
	{
	public:
		PropertyVisitor(PropertyArray* InProperties) : Properties(InProperties)
		{

		}
		template <typename PropertyType>
		void Visit(PropertyType* Property, void* = nullptr)
		{
			using ValueType = PropertyInfo<PropertyType>::ValueType;
			Properties->Add(new TPropertyType<PropertyType, ValueType>(Property));
		}

		template <typename ValueType>
		void Visit(FObjectProperty* Property, ValueType* NullValue = nullptr)
		{
			Properties->Add(new TPropertyType<FObjectProperty, ValueType*>(Property));
		}
	private:
		PropertyArray* Properties;
	};

	PropertyArray* UStructMgr::Get(UStruct* Struct)
	{
		PropertyArray* Properties = StructInfos.Find(Struct);
		if (Properties) {
			return Properties;
		}

		// create the properties
		Properties = &StructInfos.Add(Struct);

		// dispatch the properties

		for (TFieldIterator<FProperty> It(Struct); It; ++It) {
			FProperty* Property = *It;

			PropertyVisitor Visitor(Properties);
			DispatchProperty(Property, Visitor);
		}

		return Properties;
	}
}