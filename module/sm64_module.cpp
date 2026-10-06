// gmcl_sm64: thin GMod binary module around libsm64. UNTESTED SKELETON - see README.
#include "GarrysMod/Lua/Interface.h"
#include "libsm64.h"
#include <vector>
#include <cstdint>

using namespace GarrysMod::Lua;

#ifdef _WIN32
#define SM64_EXPORT extern "C" __declspec(dllexport)
#else
#define SM64_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Header-tolerant helpers: work whether or not this gmod-module-base version has lua_State::luabase / SetState.
template <class S> static auto GetBase(S* s, int) -> decltype(s->luabase) { return s->luabase; }
template <class B, class S> static auto SetStateIfAny(B* b, S* s, int) -> decltype(b->SetState(s), void()) { b->SetState(s); }
template <class B, class S> static void SetStateIfAny(B*, S*, long) {}

#define LUA_FN(name) \
	static int name##_impl(ILuaBase* LUA); \
	static int name(lua_State* state) { ILuaBase* b = GetBase(state, 0); SetStateIfAny(b, state, 0); return name##_impl(b); } \
	static int name##_impl(ILuaBase* LUA)

static uint8_t g_texture[4 * SM64_TEXTURE_WIDTH * SM64_TEXTURE_HEIGHT];
static float g_pos[9 * SM64_GEO_MAX_TRIANGLES];
static float g_nrm[9 * SM64_GEO_MAX_TRIANGLES];
static float g_col[9 * SM64_GEO_MAX_TRIANGLES];
static float g_uv [6 * SM64_GEO_MAX_TRIANGLES];
static SM64MarioGeometryBuffers g_geo;
static bool g_init = false;

static void SetNum(ILuaBase* LUA, const char* k, double v) { LUA->PushNumber(v); LUA->SetField(-2, k); }

static void PushArray(ILuaBase* LUA, const float* a, int count)
{
	LUA->CreateTable();
	for (int i = 0; i < count; i++) { LUA->PushNumber(i + 1); LUA->PushNumber(a[i]); LUA->SetTable(-3); }
}

// libsm64.Init(romString) -> ok, err
LUA_FN(L_Init)
{
	LUA->CheckType(1, 4); // string
	unsigned int len = 0;
	const char* rom = LUA->GetString(1, &len);
	if (len != 8388608) { LUA->PushBool(false); LUA->PushString("ROM must be the 8 MB US .z64"); return 2; }
	if (!g_init) {
		std::vector<uint8_t> copy(rom, rom + len);
		sm64_global_init(copy.data(), g_texture);
		g_init = true;
	}
	LUA->PushBool(true);
	return 1;
}

// libsm64.GetTexture() -> raw RGBA string (704x64)
LUA_FN(L_GetTexture)
{
	LUA->PushString((const char*)g_texture, sizeof(g_texture));
	return 1;
}

// libsm64.LoadSurfaces(flatIntTable, numTris)  (9 ints per triangle, SM64 space)
LUA_FN(L_LoadSurfaces)
{
	LUA->CheckType(1, 5); // table
	int n = (int)LUA->CheckNumber(2);
	std::vector<SM64Surface> surfs(n > 0 ? n : 0);
	for (int t = 0; t < n; t++) {
		surfs[t].type = 0; surfs[t].force = 0; surfs[t].terrain = 0;
		for (int k = 0; k < 9; k++) {
			LUA->PushNumber(t * 9 + k + 1);
			LUA->GetTable(1);
			surfs[t].vertices[k / 3][k % 3] = (int16_t)LUA->GetNumber(-1);
			LUA->Pop();
		}
	}
	sm64_static_surfaces_load(surfs.data(), (uint32_t)n);
	return 0;
}

// libsm64.MarioCreate(x,y,z) -> id (<0 on failure)
LUA_FN(L_MarioCreate)
{
	LUA->PushNumber(sm64_mario_create((float)LUA->CheckNumber(1), (float)LUA->CheckNumber(2), (float)LUA->CheckNumber(3)));
	return 1;
}

// libsm64.MarioTick(id, camLookX, camLookZ, stickX, stickY, a, b, z) -> stateTable
LUA_FN(L_MarioTick)
{
	int id = (int)LUA->CheckNumber(1);
	SM64MarioInputs in{};
	in.camLookX = (float)LUA->CheckNumber(2);
	in.camLookZ = (float)LUA->CheckNumber(3);
	in.stickX   = (float)LUA->CheckNumber(4);
	in.stickY   = (float)LUA->CheckNumber(5);
	in.buttonA  = LUA->GetBool(6);
	in.buttonB  = LUA->GetBool(7);
	in.buttonZ  = LUA->GetBool(8);
	SM64MarioState st{};
	sm64_mario_tick(id, &in, &st, &g_geo);

	LUA->CreateTable();
	SetNum(LUA, "x", st.position[0]); SetNum(LUA, "y", st.position[1]); SetNum(LUA, "z", st.position[2]);
	SetNum(LUA, "vx", st.velocity[0]); SetNum(LUA, "vy", st.velocity[1]); SetNum(LUA, "vz", st.velocity[2]);
	SetNum(LUA, "face", st.faceAngle); SetNum(LUA, "health", st.health);
	SetNum(LUA, "action", st.action); SetNum(LUA, "flags", st.flags);
	return 1;
}

// libsm64.GetGeometry() -> numTris, posTable, colTable, uvTable   (flat arrays)
LUA_FN(L_GetGeometry)
{
	int n = g_geo.numTrianglesUsed;
	LUA->PushNumber(n);
	PushArray(LUA, g_pos, n * 9);
	PushArray(LUA, g_col, n * 9);
	PushArray(LUA, g_uv,  n * 6);
	return 4;
}

LUA_FN(L_MarioDelete) { sm64_mario_delete((int)LUA->CheckNumber(1)); return 0; }

#define REG(name, fn) LUA->PushCFunction(fn); LUA->SetField(-2, name);

SM64_EXPORT int gmod13_open(lua_State* state)
{
	ILuaBase* LUA = GetBase(state, 0);
	SetStateIfAny(LUA, state, 0);

	g_geo.position = g_pos; g_geo.normal = g_nrm; g_geo.color = g_col; g_geo.uv = g_uv;
	g_geo.numTrianglesUsed = 0;

	LUA->PushSpecial(SPECIAL_GLOB);
	LUA->CreateTable();
	REG("Init", L_Init) REG("GetTexture", L_GetTexture) REG("LoadSurfaces", L_LoadSurfaces)
	REG("MarioCreate", L_MarioCreate) REG("MarioTick", L_MarioTick)
	REG("GetGeometry", L_GetGeometry) REG("MarioDelete", L_MarioDelete)
	LUA->SetField(-2, "libsm64");
	LUA->Pop();
	return 0;
}

SM64_EXPORT int gmod13_close(lua_State* state)
{
	(void)state;
	if (g_init) { sm64_global_terminate(); g_init = false; }
	return 0;
}
