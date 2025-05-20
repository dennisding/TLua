
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
			: Property(InProperty), AnsiName(TCHAR_TO_ANSI(*InProperty->GetName()))
		{
		}

		virtual void SetLuaField(lua_State* State, int Index, const void* Container) override
		{
			SetTableByName(State, Index, AnsiName.c_str(),
				(ValueType)PropertyInfo<PropertyType>::GetValue(Container, Property));
		}

		virtual void GetLuaField(lua_State* State, int Index, void* Container) override
		{
			auto Value = GetTableByName<ValueType>(State, Index, 
				AnsiName.c_str());
							//(const char*)*Property->GetNameCPP());
			
			PropertyInfo<PropertyType>::SetValue(Container, Property, Value);
		}

	private:
		PropertyType* Property;
		std::string AnsiName;
	};

	class StructPropertyType : public PropertyBase
	{
	public:
		StructPropertyType(FStructProperty* InProperty) : Property(InProperty)
		{ 
			Properties = UStructMgr::Get(Property->Struct);
		}

		virtual void SetLuaField(lua_State* State, int Index, const void* Container) override
		{
			const void* ValuePtr = Property->ContainerPtrToValuePtr<const void*>(Container);

			for (PropertyBase* ChildProperty : *Properties) {
				ChildProperty->SetLuaField(State, Index, ValuePtr);
			}
		}

		virtual void GetLuaField(lua_State* State, int Index, void* Container) override
		{
			void* ValuePtr = Property->ContainerPtrToValuePtr<void*>(Container);
			for (PropertyBase* ChildProperty : *Properties) {
				ChildProperty->GetLuaField(State, Index, ValuePtr);
			}
		}

	private:
		PropertyArray* Properties;
		FStructProperty* Property;
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

		void Visit(FStructProperty* Property)
		{
			Properties->Add(new StructPropertyType(Property));
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