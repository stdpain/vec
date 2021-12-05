#include <lua.hpp>
#include <cassert>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#define LOG_ERR_IF(st, L) \
	if ((st) != 0) { \
		const char* errmsg = lua_tostring((L), -1); \
		std::cout << errmsg << std::endl; \
	} \

int cxx_float_add(lua_State * L)
{
	int res = lua_gettop(L);
	// args 
	double arg1 = luaL_checknumber(L, 1);
	double arg2 = luaL_checknumber(L, 2);
	lua_pushnumber(L, arg1 + arg2);
	// return number
	return 1;
}

static luaL_Reg cuslib[] =
{
	{"cxx_cus_add", cxx_float_add},
	{NULL, NULL},
};


int load_cus_lib(lua_State* L)
{
	luaL_newlib(L, cuslib);
	return 1; //return one value
}

class CXXVec {
public:
	static const constexpr char* meta = "cxx_vec";
	CXXVec() {
		std::cout << "call cxx_vec_constructor" << std::endl; 
	}

	~CXXVec() { std::cout << "call cxx_vec_destructor" << std::endl; }

	int at(int i) {
		return _data[i];
	}
private:
	std::vector<int> _data = { 1,2,3,4,5 };
};

int new_cxx_vec(lua_State* L) {
	void* pVec = lua_newuserdata(L, sizeof(CXXVec*));
	*static_cast<CXXVec**>(pVec) = new CXXVec();
	luaL_getmetatable(L, CXXVec::meta);
	lua_setmetatable(L, -2);
	return 1;
}

int del_cxx_vec(lua_State* L) {
	CXXVec** pVec = (CXXVec**)luaL_checkudata(L, 1, CXXVec::meta);
	delete* pVec;
	return 0;
}

int at_cxx_vec(lua_State* L) {
	CXXVec** ppVec = (CXXVec**)luaL_checkudata(L, 1, CXXVec::meta);
	assert(ppVec);
	luaL_argcheck(L, ppVec != NULL, 1, "invalid user data");
	//std::cout << "to integer" << std::endl;
	int idx = (int)lua_tointeger(L, 2);
	CXXVec* pVec = *ppVec;
	lua_pushinteger(L, pVec->at(idx));
	return 1;
}

struct luaL_Reg cxx_vec_lib[] = {
	{"create", new_cxx_vec},
	{NULL, NULL},
};

static const struct luaL_Reg cxx_vec_reg_mf[] =
{
	{"at", at_cxx_vec},
	{"__gc", del_cxx_vec},
	{NULL, NULL},
};

int CXXVecLibOpen(lua_State* L)
{
	luaL_newlib(L, cxx_vec_lib);
	return 1;
}

int main(int argc, char* args)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	//luaL_loadfile(L, "");
	const char lua_script[] = R"(
		print("hello world");
		function add(a, b)
			return a + b;
		end
		function strcat(a, b)
			return a..b;
		end
		function get_array()
			local val = {1, nil, 2, "str"};
			return val;
		end
	)";
	const char* name_add_func = "add";
	const char* name_strcat_func = "strcat";
	const char* name_getarray_func = "get_array";
	int res;
	res = luaL_loadbuffer(L, lua_script, sizeof(lua_script) - 1, "load");
	assert(res == 0);
	res = lua_pcall(L, 0, 0, 0);
	assert(res == 0);

	{
		int a = 1;
		int b = 2;
		// call add(a, b);
		res = lua_getglobal(L, name_add_func);
		assert(res == LUA_TFUNCTION);
		res = lua_isfunction(L, -1);
		assert(res == 1);
		lua_pushinteger(L, a);
		lua_pushinteger(L, b);
		res = lua_pcall(L, 2, 1, 0);
		LOG_ERR_IF(res, L);
		assert(res == 0);
		res = lua_isinteger(L, -1);
		assert(res == 1);
		int64_t ret = lua_tointeger(L, -1);
		assert(ret == 3);
		res = lua_gettop(L);
		assert(res == 1);
		lua_settop(L, 0);
	}
	{
		std::string stra = "hello";
		std::string strb = "world";
		res = lua_getglobal(L, name_strcat_func);
		assert(res == LUA_TFUNCTION);
		res = lua_isfunction(L, -1);
		lua_pushlstring(L, stra.data(), stra.length());
		lua_pushlstring(L, strb.data(), strb.length());
		res = lua_pcall(L, 2, 1, 0);
		LOG_ERR_IF(res, L);
		assert(res == 0);
		res = lua_isstring(L, -1);
		assert(res == 1);
		size_t len = -1;
		const char* ret = lua_tolstring(L, -1, &len);
		std::string expect_str = stra + strb;
		assert(expect_str == std::string(ret, len));
		assert(lua_gettop(L) == 1);
		res = lua_getmetatable(L, -1);
		assert(res == 1);
		assert(lua_gettop(L) == 2);
		// https://www.lua.org/manual/5.3/manual.html#lua_next
		// 
		{
			lua_pushnil(L);  /* first key */
			while (lua_next(L, -2) != 0) {
				/* uses 'key' (at index -2) and 'value' (at index -1) */
				printf("%s - %s\n",
					lua_typename(L, lua_type(L, -2)),
					lua_typename(L, lua_type(L, -1)));
				/* removes 'value'; keeps 'key' for next iteration */
				lua_pop(L, 1);
			}
		}
		assert(lua_gettop(L) == 2);
		lua_settop(L, 0);
	}
	{
		res = lua_getglobal(L, name_getarray_func);
		assert(res == LUA_TFUNCTION);
		res = lua_isfunction(L, -1);
		assert(res == 1);
		res = lua_pcall(L, 0, 1, 0);
		LOG_ERR_IF(res, L);
		assert(res == 0);
		res = lua_istable(L, -1);
		assert(res == 1);
		int len = luaL_len(L, -1);
		{
			//lua_objlen(L, -1);
			for (int i = 1; i <= len; i++)
			{
				lua_pushnumber(L, i);
				lua_gettable(L, -2);
				printf("array at:%d - %s\n", i,
					lua_typename(L, lua_type(L, -1)));
				lua_pop(L, 1);
			}
		}
		{
			lua_pushnil(L);  /* first key */
			while (lua_next(L, -2) != 0) {
				/* uses 'key' (at index -2) and 'value' (at index -1) */
				printf("%s - %s\n",
					lua_typename(L, lua_type(L, -2)),
					lua_typename(L, lua_type(L, -1)));
				/* removes 'value'; keeps 'key' for next iteration */
				lua_pop(L, 1);
			}
		}
	}
	{
		lua_register(L, "cxx_add", cxx_float_add);
		luaL_dostring(L, "print(cxx_add(1, 2))");
		luaL_requiref(L, "cuslib", load_cus_lib, 1);
		res = luaL_dostring(L, "print(cuslib.cxx_cus_add(1, 2))");
		LOG_ERR_IF(res, L);
		lua_settop(L, 0);
	}
	{
		luaL_requiref(L, "CXXVec", CXXVecLibOpen, 1);
		luaL_newmetatable(L, CXXVec::meta);
		lua_pushvalue(L, -1);
		lua_setfield(L, -2, "__index");
		luaL_setfuncs(L, cxx_vec_reg_mf, 0);
		lua_pop(L, 1);
		res = luaL_dostring(L, R"(
			local stu = CXXVec.create();
			print(stu:at(2));
		)");
		LOG_ERR_IF(res, L);
	}


	lua_close(L);
	return 0;
}
