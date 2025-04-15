
print("hello world!")

function say_hello_to(name, value)
    print('hello unreal!!!!'..name, value)

    _cpp.test_callback(100)
    print(_cpp.test_callback2())
    return value + 1
end

function _register_callback(name, processor, cfun)
    print('register a callback', name, processor, cfun)
    _cpp = _cpp or {}
    local function _imp(...)
--        _cpp_callback(cfun, ...)
        _cpp_callback(processor, cfun, ...)
    end
    _cpp[name] = _imp
end
