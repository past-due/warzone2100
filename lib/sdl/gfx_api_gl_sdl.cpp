/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2019  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "gfx_api_gl_sdl.h"
#include <SDL_opengl.h>

sdl_OpenGL_Impl::sdl_OpenGL_Impl(SDL_Window* _window)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
}

GLADloadproc sdl_OpenGL_Impl::getGLGetProcAddress()
{
	return SDL_GL_GetProcAddress;
}

bool sdl_OpenGL_Impl::createGLContext()
{
	SDL_GLContext WZglcontext = SDL_GL_CreateContext(window);
	if (!WZglcontext)
	{
		// Failed to create the default (Core Profile) context
		std::string originalGLContextError(SDL_GetError());
		debug(LOG_3D, "Cannot create an OpenGL Core context [%s]; falling back to compatibility context", originalGLContextError.c_str());

		// Fall back to forward-compatible OpenGL 2.1
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		WZglcontext = SDL_GL_CreateContext(window);
		if (!WZglcontext)
		{
			debug(LOG_ERROR, "First attempt - Failed to create an OpenGL Core context! [%s]", originalGLContextError.c_str());
			debug(LOG_ERROR, "Second attempt - Failed to create an OpenGL 2.1 Compatibility context! [%s]", SDL_GetError());
		}
		return false;
	}

	int value = 0;
	if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == -1 || value == 0)
	{
		debug(LOG_FATAL, "OpenGL initialization did not give double buffering!");
		debug(LOG_FATAL, "Double buffering is required for this game!");
		return false;
	}

	int windowWidth, windowHeight = 0;
	SDL_GetWindowSize(window, &windowWidth, &windowHeight);
	debug(LOG_WZ, "Logical Window Size: %d x %d", windowWidth, windowHeight);

	return true;
}

void sdl_OpenGL_Impl::swapWindow()
{
	SDL_GL_SwapWindow(window);
}

void sdl_OpenGL_Impl::getDrawableSize(int* w, int* h)
{
	SDL_GL_GetDrawableSize(window, w, h);
}
