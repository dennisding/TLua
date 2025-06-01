
#pragma once

#include "TLuaImp.hpp"

namespace TLua
{
	class Function
	{
		Function(const Function& Fun) = delete;
		Function(const Function&& Fun) = delete;
		Function& operator=(const Function& Fun) = delete;
		Function& operator=(const Function&& Fun) = delete;
	public:
		Function()
		{
			Key = this;
		}

		virtual ~Function()
		{
			Release();
		}

		void FromLua(lua_State* State, int Index)
		{

		}

		void ToLua(lua_State* State)
		{
		}

	private:
		void Release()
		{

		}

	private:
		void* Key;
	};
}