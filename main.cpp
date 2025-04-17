

#include <map>
#include <vector>
#include <iostream>

#include "lua.hpp"
#include "TLua.hpp"

class Test
{
public:
	Test() : age(100)
	{
		TLua::Bind(this, "Test");
	}

	void test()
	{
		int new_age = TLua::CallMethod<int>(this, "get_info");

		// age = TLua::GetAttr<int>(this, "age") ?? can we achive this?
		TLua::SetAttr(this, "age", 13);
		TLua::GetAttr(this, "age", age);
		TLua::SetAttr(this, "name", "new_name");
		TLua::GetAttr(this, "name", name);
	}

	virtual ~Test()
	{
		TLua::Unbind(this);
	}

public:
	int age;
	std::string name;
};

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

void test_bind()
{
	// vtable
	TLua::VTable<Test>("Test")
		.AddAttr("age",  &Test::age)
		.AddAttr("name", &Test::name)
		.AddAttr("test", &Test::test)
		;

	Test* test = new Test();
	TLua::CallMethod(test, "show_info");
	test->test();
	delete test;
}

int main(int argc, const char** argv)
{

	test_lua();
	test_bind();

	std::cout << "hello world!" << std::endl;

	return 0;
}