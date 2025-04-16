

#include <iostream>

#include "lua.hpp"
#include "TLua.hpp"

void test_callback(int iv, int iv2)
{
	std::cout << "c++ callback " << iv << " ----- " << iv2 << std::endl;
}

int test_callback2()
{
	std::cout << "c++ callback" << std::endl;

	return 1;
}

void test_lua()
{
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