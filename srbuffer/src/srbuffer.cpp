#include <cstring>
#define LIB_NAME "srbuffer"
#define MODULE_NAME "srbuffer"

#include <cstdint>
#include <dmsdk/sdk.h>
#include <stdint.h>

#if defined(__BYTE_ORDER__)
/* Nothing to do on bigendian systems. */
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SR_BIG_TO_HOST(X, N) ;
#endif /* __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__ */

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)

/* For GCC and Clang, have builtins for byte swapping. */
#if defined(__GNUC__) && defined(__GNUC_PREREQ)
#if __GNUC_PREREQ(4, 3)
#define have_bswap
#endif
#endif

#if defined(__clang__) && defined(__has_builtin)
#if __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64) &&    \
    __has_builtin(__builtin_bswap16)
#define have_bswap
#endif
#endif
#endif
#endif /* __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ */

#if defined(have_bswap)
#define SR_BIG_TO_HOST(N)                                                      \
  switch (sizeof(N)) {                                                         \
  case 1:                                                                      \
    N = N;                                                                     \
  case 2:                                                                      \
    N = __builtin_bswap16(N);                                                  \
  case 4:                                                                      \
    N = __builtin_bswap32(N);                                                  \
  case 8:                                                                      \
    N = (__builtin_bswap64(N));                                                \
  }
#endif

#define SER_STATUS_FREED 0;
#define SER_STATUS_ACTIVE 1;

enum DATA_TYPES {
  SER_DATA_UINT8,
  SER_DATA_UINT16,
  SER_DATA_UINT32,
  SER_DATA_UINT64,
  SER_DATA_INT8,
  SER_DATA_INT16,
  SER_DATA_INT32,
  SER_DATA_INT64,
  SER_DATA_FLOAT32,
  SER_DATA_DOUBLE,
  SER_DATA_STRING,
};

typedef struct {
  uint8_t status;
  uint32_t size;
  uint32_t current_index;
  uint8_t *buffer;
} B_Buffer;

static bool is_little_endian = true;
#ifndef SR_BIG_TO_HOST
#define SR_BIG_TO_HOST(N)                                                      \
  if (is_little_endian) {                                                      \
    switch (sizeof(N)) {                                                       \
    case 1:
N = N;
break;
case 2:
N = (N << 8) | ((N >> 8) & 0xFF);
break;
case 4:
N = (N >> 24) | ((N & 0x00FF0000) >> 8) | ((N & 0x0000FF00) << 8) | (N << 24);
break;
case 8:
N = (N >> 56) | ((N & 0x00FF000000000000) >> 40) |
    ((N & 0x0000FF0000000000) >> 24) | ((N & 0x000000FF00000000) >> 8) |
    ((N & 0x00000000FF000000) << 8) | ((N & 0x0000000000FF0000) << 24) |
    ((N & 0x000000000000FF00) << 40) | (N << 56);
}
}
#endif // !SR_BIG_TO_HOST

#define SER_CHECK_DATA(__L, __n, s)                                            \
  DM_LUA_STACK_CHECK(__L, __n);                                                \
  B_Buffer *serializer;                                                        \
  const char *__str = luaL_checkstring(L, 1);                                  \
  B_Buffer sr;                                                                 \
  serializer = &sr;                                                            \
  sr.size = lua_strlen(L, 1);                                                  \
  sr.buffer = (uint8_t *)__str;                                                \
  sr.current_index = 0;                                                        \
  int offset = luaL_checkint(__L, 2);                                          \
  if (offset >= 0) {                                                           \
    serializer->current_index = offset;                                        \
  }                                                                            \
  if (serializer->size < serializer->current_index + s) {                      \
    printf("error");                                                           \
    return DM_LUA_ERROR(                                                       \
        "serializer buffer out of bounds size: %d, index %d, writesize %d",    \
        serializer->size, serializer->current_index, s);                       \
  }

#define READ_BUFFER(T, B)                                                      \
  ({                                                                           \
    T __Val = ((T *)(B->buffer + B->current_index))[0];                        \
    B->current_index += sizeof(T);                                             \
    __Val;                                                                     \
  });

//SR_BIG_TO_HOST(__Val);                                                     \

#define WRITE_BUFFER(T, B, N)                                                  \
  ({                                                                           \
    T __Val = ((T *)(B->buffer + B->current_index))[0] = N;                    \
    B->current_index += sizeof(T);                                             \
    SR_BIG_TO_HOST(__Val);                                                     \
    __Val;                                                                     \
  });

static int Create(lua_State *L) {
  DM_LUA_STACK_CHECK(L, 1);
  size_t size = luaL_checkinteger(L, 1);
  char *str = (char *)malloc(size);
  lua_pushlstring(L, str, size);
  free(str);
  return 1;
}

static int Expand(lua_State *L) {
  DM_LUA_STACK_CHECK(L, 1);
  size_t size = luaL_checkinteger(L, 2);
  size_t original_size = lua_strlen(L, 1);
  char *str = (char *)malloc(size + original_size);
  memcpy(str, luaL_checkstring(L, 1), original_size);
  lua_pushlstring(L, str, size);
  free(str);
  return 1;
}

static int ReadUint8(lua_State *L) {
  SER_CHECK_DATA(L, 2, 1);
  uint8_t num = serializer->buffer[serializer->current_index++];
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadUint16(lua_State *L) {
  SER_CHECK_DATA(L, 2, 2)
  uint16_t num = READ_BUFFER(uint16_t, serializer);
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadUint32(lua_State *L) {
  SER_CHECK_DATA(L, 2, 4);
  uint32_t num = READ_BUFFER(uint32_t, serializer);
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadUint64(lua_State *L) {
  SER_CHECK_DATA(L, 2, 8)
  uint64_t num = READ_BUFFER(uint64_t, serializer);
  lua_pushnumber(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadFloat32(lua_State *L) {
  SER_CHECK_DATA(L, 2, 4);
  float num = READ_BUFFER(float, serializer);
  lua_pushnumber(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadFloat64(lua_State *L) {
  SER_CHECK_DATA(L, 2, 4);
  double num = READ_BUFFER(double, serializer);
  lua_pushnumber(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadInt8(lua_State *L) {
  SER_CHECK_DATA(L, 2, 1)
  int8_t num = serializer->buffer[serializer->current_index++];
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadInt16(lua_State *L) {
  SER_CHECK_DATA(L, 2, 2)
  int16_t num = READ_BUFFER(typeof(num), serializer);
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadInt32(lua_State *L) {
  SER_CHECK_DATA(L, 2, 4)
  int32_t num = READ_BUFFER(typeof(num), serializer);
  lua_pushinteger(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadInt64(lua_State *L) {
  SER_CHECK_DATA(L, 2, 8)
  int64_t num = READ_BUFFER(typeof(num), serializer);
  lua_pushnumber(L, num);
  lua_pushinteger(L, sizeof(num));
  return 2;
}

static int ReadCstring(lua_State *L) {
  SER_CHECK_DATA(L, 3, 1)
  uint32_t len = luaL_checkint(L, 3);
  if (serializer->size - serializer->current_index < len) {
    DM_LUA_ERROR("buffer out of bounds while reading string of size %d in sr "
                 "of size: %d",
                 len, serializer->size);
  }
  lua_pushlstring(
      L, (const char *)(serializer->buffer + serializer->current_index), len);
  serializer->current_index += len;
  lua_pushinteger(L, sizeof(len) + len);
  return 2;
}

/* Write functions */
static int WriteUint8(lua_State *L) {
  SER_CHECK_DATA(L, 1, 1);
  uint8_t num = luaL_checkint(L, 3);
  serializer->buffer[serializer->current_index++] = num;
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteUint16(lua_State *L) {
  SER_CHECK_DATA(L, 1, 2)
  uint16_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(uint16_t, serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteUint32(lua_State *L) {
  SER_CHECK_DATA(L, 1, 4)
  uint32_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(uint32_t, serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteUint64(lua_State *L) {
  SER_CHECK_DATA(L, 1, 8)
  uint64_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteFloat32(lua_State *L) {
  SER_CHECK_DATA(L, 1, 4)
  float num = luaL_checknumber(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteFloat64(lua_State *L) {
  SER_CHECK_DATA(L, 1, 4)
  double num = luaL_checknumber(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteInt8(lua_State *L) {
  SER_CHECK_DATA(L, 1, 1);
  int8_t num = luaL_checkinteger(L, 3);
  serializer->buffer[serializer->current_index++] = num;
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteInt16(lua_State *L) {
  SER_CHECK_DATA(L, 1, 2)
  int16_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteInt64(lua_State *L) {
  SER_CHECK_DATA(L, 1, 2)
  int64_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteInt32(lua_State *L) {
  SER_CHECK_DATA(L, 1, 4)
  int32_t num = luaL_checkint(L, 3);
  WRITE_BUFFER(typeof(num), serializer, num);
  lua_pushinteger(L, sizeof(num));
  return 1;
}

static int WriteSimpleArray(lua_State *L) {
  SER_CHECK_DATA(L, 1, 1);

  if (!lua_istable(L, 3)) {
    DM_LUA_ERROR("expected array");
  }

  int type = luaL_checkinteger(L, 4);
  lua_pushnil(L);

  while (lua_next(L, -2) != 0) {
    switch (type) {
    case SER_DATA_UINT8: {
      uint8_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_UINT16: {
      uint16_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_UINT32: {
      uint32_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_UINT64: {
      uint64_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_INT8: {
      int8_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_INT16: {
      int16_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_INT32: {
      int32_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    case SER_DATA_INT64: {
      int64_t num = lua_tonumber(L, -1);
      WRITE_BUFFER(typeof(num), serializer, num);
    } break;
    }
    lua_pop(L, 1);
  }

  lua_pushinteger(L, serializer->current_index - offset);

  return 1;
}

static int WriteCstring(lua_State *L) {
  SER_CHECK_DATA(L, 1, 4)
  const char *str = luaL_checkstring(L, 3);
  uint32_t len = lua_strlen(L, 3);
  if (serializer->size < serializer->current_index + len - 1) {
    DM_LUA_ERROR("Serializer out of bounds for string");
    return 0;
  }
  memcpy(serializer->buffer + serializer->current_index, str, len);
  serializer->current_index += len;
  lua_pushinteger(L, 4 + len);
  return 1;
}

static int getBytes(lua_State *L) {
  SER_CHECK_DATA(L, 1, 0);
  lua_pushlstring(L, (const char *)serializer->buffer,
                  serializer->current_index);
  return 1;
}

static int reset(lua_State *L) {
  SER_CHECK_DATA(L, 0, 0);
  serializer->current_index = 0;
  return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] = {
    {"create", Create},
    {"readuint8", ReadUint8},
    {"readuint16", ReadUint16},
    {"readuint32", ReadUint32},
    {"readuint64", ReadUint64},
    {"readfloat32", ReadFloat32},
    {"readfloat64", ReadFloat64},
    {"readint8", ReadInt8},
    {"readint16", ReadInt16},
    {"readint32", ReadInt32},
    {"readint64", ReadInt64},
    {"readcstring", ReadCstring},
    {"writeuint8", WriteUint8},
    {"writeuint16", WriteUint16},
    {"writeuint32", WriteUint32},
    {"writeuint64", WriteUint64},
    {"writefloat32", WriteFloat32},
    {"writefloat64", WriteFloat64},
    {"writeint8", WriteInt8},
    {"writeint16", WriteInt16},
    {"writeint64", WriteInt64},
    {"writeint32", WriteInt32},
    {"writecstring", WriteCstring},
    {"writesimplearray", WriteSimpleArray},
    {"reset", reset},
    {0, 0}};

static void LuaInit(lua_State *L) {
  int top = lua_gettop(L);

  // Register lua names
  luaL_register(L, MODULE_NAME, Module_methods);

  lua_pop(L, 1);
  assert(top == lua_gettop(L));
}

dmExtension::Result InitializeMyExtension(dmExtension::Params *params) {
  int i = 1;
  is_little_endian = (*((char *)&i)) != 0;
  // Init Lua
  LuaInit(params->m_L);
  printf("Registered %s Extension\n", MODULE_NAME);
  return dmExtension::RESULT_OK;
}

DM_DECLARE_EXTENSION(srbuffer, LIB_NAME, 0, 0, InitializeMyExtension, 0, 0, 0);
