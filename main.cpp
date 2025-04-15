

#include <iostream>

#include "lua.hpp"
#include "TLua.hpp"

void test_callback(int iv)
{
	std::cout << "c++ callback " << iv << std::endl;
}

int test_callback2()
{
	std::cout << "c++ callback" << std::endl;

	return 1;
}

void test_lua()
{
	//lua_State* lua = luaL_newstate();
	//luaL_openlibs(lua);

	//// call lua file
	//if (luaL_dofile(lua, "test.lua") != 0) {
	//	std::cout << "error" << std::endl;
	//}

	//// call lua fun
	//lua_getglobal(lua, "say_hello_to");
	//lua_pushstring(lua, "dennis");
	//lua_call(lua, 1, 0);

	TLua::Init();
	TLua::DoFile("test.lua");

	TLua::RegisterCallback("test_callback", test_callback);
	TLua::RegisterCallback("test_callback2", test_callback2);

	std::string value = TLua::Call<std::string>("say_hello_to", "dennis", 1024);
	std::cout << value << std::endl;
}

int main(int argc, const char** argv)
{

	test_lua();

	std::cout << "hello world!" << std::endl;

	return 0;
}