/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/***************************************************************************/
/*
 * pieState.h
 *
 * render State controlr all pumpkin image library functions.
 *
 */
/***************************************************************************/

#ifndef _piestate_h
#define _piestate_h

/***************************************************************************/

#include <string>
#include <vector>

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/framework/opengl.h"
#include "lib/ivis_opengl/gfx_api.h"
#include <glm/gtc/type_ptr.hpp>
#include "piedef.h"

struct iIMDShape;

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

struct RENDER_STATE
{
	bool				fogEnabled;
	bool				fog;
	PIELIGHT			fogColour;
	float				fogBegin;
	float				fogEnd;
	SDWORD				texPage;
	REND_MODE			rendMode;
};

void rendStatesRendModeHack();  // Sets rendStates.rendMode = REND_ALPHA; (Added during merge, since the renderStates is now static.)

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void pie_SetDefaultStates();//Sets all states
//fog available
void pie_EnableFog(bool val);
bool pie_GetFogEnabled();
//fog currently on
void pie_SetFogStatus(bool val);
bool pie_GetFogStatus();
void pie_SetFogColour(PIELIGHT colour);
PIELIGHT pie_GetFogColour() WZ_DECL_PURE;
void pie_UpdateFogDistance(float begin, float end);
//render states
RENDER_STATE getCurrentRenderState();

int pie_GetMaxAntialiasing();

enum SHADER_VERSION
{
	VERSION_120,
	VERSION_130,
	VERSION_140,
	VERSION_150_CORE,
	VERSION_330_CORE,
	VERSION_400_CORE,
	VERSION_410_CORE,
	VERSION_FIXED_IN_FILE,
	VERSION_AUTODETECT_FROM_LEVEL_LOAD
};
bool pie_LoadShaders();
void pie_FreeShaders();
SHADER_MODE pie_LoadShader(SHADER_VERSION vertex_version, SHADER_VERSION fragment_version, const char *programName, const std::string &vertexPath, const std::string &fragmentPath,
	const std::vector<std::string> &);
inline SHADER_MODE pie_LoadShader(SHADER_VERSION version, const char *programName, const std::string &vertexPath, const std::string &fragmentPath,
						   const std::vector<std::string> &uniformNames)
{
	return pie_LoadShader(version, version, programName, vertexPath, fragmentPath, uniformNames);
}

namespace pie_internal
{
	struct SHADER_PROGRAM
	{
		GLuint program = 0;

		// Uniforms
		std::vector<GLint> locations;

		// Attributes
		GLint locVertex = 0;
		GLint locNormal = 0;
		GLint locTexCoord = 0;
		GLint locColor = 0;
	};

	extern std::vector<SHADER_PROGRAM> shaderProgram;
	extern gfx_api::buffer* rectBuffer;

	/**
	 * setUniforms is an overloaded wrapper around glUniform* functions
	 * accepting glm structures.
	 * Please do not use directly, use pie_ActivateShader below.
	 */

	inline void setUniforms(GLint location, const ::glm::vec4 &v)
	{
		glUniform4f(location, v.x, v.y, v.z, v.w);
	}

	inline void setUniforms(GLint location, const ::glm::mat4 &m)
	{
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(m));
	}

	inline void setUniforms(GLint location, const Vector2i &v)
	{
		glUniform2i(location, v.x, v.y);
	}

	inline void setUniforms(GLint location, const Vector2f &v)
	{
		glUniform2f(location, v.x, v.y);
	}

	inline void setUniforms(GLint location, const int32_t &v)
	{
		glUniform1i(location, v);
	}

	inline void setUniforms(GLint location, const float &v)
	{
		glUniform1f(location, v);
	}

	/**
	 * uniformSetter is a variadic function object.
	 * It's recursively expanded so that uniformSetter(array, arg0, arg1...);
	 * will yield the following code:
	 * {
	 *     setUniforms(arr[0], arg0);
	 *     setUniforms(arr[1], arg1);
	 *     setUniforms(arr[2], arg2);
	 *     ...
	 *     setUniforms(arr[n], argn);
	 * }
	 */
	template<typename...T>
	struct uniformSetter
	{
		void operator()(const std::vector<GLint> &locations, T...) const;
	};

	template<>
	struct uniformSetter<>
	{
		void operator()(const std::vector<GLint> &) const {}
	};

	template<typename T, typename...Args>
	struct uniformSetter<T, Args...>
	{
		void operator()(const std::vector<GLint> &locations, const T& value, const Args&...args) const
		{
			constexpr int N = sizeof...(Args) + 1;
			setUniforms(locations[locations.size() - N], value);
			uniformSetter<Args...>()(locations, args...);
		}
	};
}

void pie_SetShaderStretchDepth(float stretch);
float pie_GetShaderTime();
float pie_GetShaderStretchDepth();
void pie_SetShaderTime(uint32_t shaderTime);
void pie_SetShaderEcmEffect(bool value);
int pie_GetShaderEcmEffect();

static inline glm::vec4 pal_PIELIGHTtoVec4(PIELIGHT rgba)
{
	return (1 / 255.0f) * glm::vec4{
		rgba.byte.r,
		rgba.byte.g,
		rgba.byte.b,
		rgba.byte.a
	};
}

#endif // _pieState_h
