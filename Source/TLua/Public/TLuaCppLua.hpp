
#pragma once

#include "Lua/lua.hpp"

namespace TLua
{
	class CppLuaType
	{
	public:
		CppLuaType(int InIndex) : Index(InIndex)
		{
		}

		operator int() const
		{
			return Index;
		}

	private:
		int Index;
	};

	void RegisterCppLua();
}
