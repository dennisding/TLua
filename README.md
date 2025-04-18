
# 1. how to use it.  
        1. just add the TLua.hpp, TLua.cpp, TLuaArg.hpp and lua source code to you project.  
        2. include the TLua.hpp in you source.  

# 2. call lua fun in c++  
```cpp
    TLua::DoFile(file_name);  
    TLua::DoString("print("hello world")");  
    ReturnType result = TLua::Call<ReturnType>(lua_fun_name, arg1, arg2, arg3);  
```
   
# 3. call c++ fun in lua  
## 1. register c++ function to lua
```cpp
        std::vector<int> cpp_fun(int iv, std::map<std::string, int> &mv)  
        TLua::RegisterCallback("_cpp_fun_name", cpp_fun);
```
## 2. call c++ function in lua  
```cpp
        local result = _cpp._cpp_fun_name(1024, {['one']=1, ['two']=2, ['three']=3})  
```
# 4. bind c++ object to lua table  
```cpp
    struct Test
    {
        Test()
        {
            TLua::Bind(this, "Test");
        }
        ~Test()
        {
            TLua::Unbind(this);
        }

        int test()
        {
            TLua::SetAttr(this, "age", 100);
            TLua::GetAttr(this, "name", name_ )
            std::stirng result = TLua::CallMethod<std::string>(this, "show_info", name_);
        }
        std::string name_;
    }
```
## 1. add a vtable to the lua code
```cpp
          TLua::VTable<Test>("Test") // "Test" is a global function you can create the instance table
             .AddAttr(&Test::name_)
             .AddAttr(&Test::test)
           ;
```
## 2. bind the c++ object to lua object
```cpp
        TLua::Bind(this, "Test");
```
## 3. unbind the c++ object when c++ object is deleted
```cpp        
        TLua::Unbind(this);
```
## 4. access the lua attributes
```cpp
        TLua::SetAttr(this, name, value)
        TLua::GetAttr(this, name, return_value)
```
## 5. call the lua object method
```cpp
        ReturnType result = TLua::CallMethod<ReturnType>(this, name, args...);
```
## 6. call the cpp object method
```lua
        local result = self._vtable[method_name](args...)
```
## 7. access the cpp attributes
```lua
       local getter, setter = table.unpack(self._vtable[attr_name])
       local value = getter(self._cobj)
       setter(self._cobj, new_value)
```
