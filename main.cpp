

#include <map>
#include <vector>
#include <iostream>

#include "lua.hpp"
#include "TLua.hpp"

void test_callback(int iv, int iv2, std::vector<int> ivv, std::map<std::string, int> im)
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
	TLua::DoFile("Script/Lua/init.lua");
	TLua::DoFile("test.lua");

	using int_vector = std::vector<int>;

	int_vector iv;
	iv.push_back(100);
	iv.push_back(200);
	iv.push_back(300);
	iv.push_back(400);

	std::map<std::string, int> im;
	im["eleven"] = 7;
	im["six"] = 6;
	im["four"] = 4;

	TLua::RegisterCallback("test_callback", test_callback);
	TLua::RegisterCallback("test_callback2", test_callback2);

	std::string value = TLua::Call<std::string>("say_hello_to", "dennis", 1024, iv, im);
	int riv = TLua::Call<int>("get_int_value");
	double rdv = TLua::Call<double>("get_double_value");
	int_vector ivv = TLua::Call<int_vector>("get_int_vector");

	using int_map = std::map<std::string, int>;
	int_map mv = TLua::Call<int_map>("get_int_map");

	std::cout << value << "---" << riv << "---" << rdv << "---" << rdv << std::endl;
}

int main(int argc, const char** argv)
{

	test_lua();

	std::cout << "hello world!" << std::endl;

	return 0;
}