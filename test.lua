
print("hello world!")

_cpp_log(999, "this is a log from lua!!!")

function say_hello_to(name, value, iv, im)
    print('hello unreal!!!!'..name, value, iv)

    for k, v in ipairs(iv) do
        print('key, value', k, v)
    end

    for k, v in pairs(im) do
        print('key, value', k, v)
    end

    local t = {111, 222, 333, 444}
    local t2 = {}
    t2['hello'] = 1
    t2['word'] = 2

    _cpp.test_callback(100, 200, t, t2)
    print(_cpp.test_callback2())
    return value + 1
end

function get_int_value()
    return 1
end

function get_double_value()
    return 3.14
end

function get_int_vector()
    return {10, 11, 12, 13, 14}
end

function get_int_map()
    local m = {}
    m["one"] = 1
    m["two"] = 2
    m["three"] = 3
    return m
end


function Test()
    local t = {}
    t.age = 3111
    t.name = 'dennis'
    t.set_info = function(self, age, name)
        print("new age and name", age, name)
        self.age = age
        self.name = name
    end

    t.show_info = function(self)
        print('info, age:', self.age, "  name:", self.name)
    end

    t.get_info = function(self)
        return self.age
    end

    -- return a new instance
    return t
end

-- function _register_callback(name, processor, cfun)
--     print('register a callback', name, processor, cfun)
--     _cpp = _cpp or {}
--     local function _imp(...)
--         _cpp_callback(processor, cfun, ...)
--     end
--     _cpp[name] = _imp
-- end
