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

sdl_OpenGL_Impl::sdl_OpenGL_Impl(SDL_Window* _window, bool _useOpenGLES)
{
	ASSERT(_window != nullptr, "Invalid SDL_Window*");
	window = _window;
	useOpenglES = _useOpenGLES;
	if (useOpenglES)
	{
		contextRequest = OpenGLES30;
	}
}

GLADloadproc sdl_OpenGL_Impl::getGLGetProcAddress()
{
	return SDL_GL_GetProcAddress;
}

bool sdl_OpenGL_Impl::configureNextOpenGLContextRequest()
{
	contextRequest = GLContextRequests(contextRequest + 1);
	switch (contextRequest)
	{
		case OpenGLCore_HighestAvailable:
		case OpenGL21Compat:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			return true;
		case OpenGLES30:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			return true;
		case OpenGLES20:
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			return true;
		case MAX_CONTEXT_REQUESTS:
			return false;
	}
	return false;
}

std::string sdl_OpenGL_Impl::to_string(const GLContextRequests& request) const\
{
	switch (contextRequest)
	{
		case OpenGLCore_HighestAvailable:
			return "OpenGL Core";
		case OpenGL21Compat:
			return "OpenGL 2.1 Compatibility";
		case OpenGLES30:
			return "OpenGL ES 3.0";
		case OpenGLES20:
			return "OpenGL ES 2.0";
		case MAX_CONTEXT_REQUESTS:
			return "";
	}
	return "";
}

bool sdl_OpenGL_Impl::isOpenGLES()
{
	return contextRequest >= OpenGLES30;
}

bool sdl_OpenGL_Impl::createGLContext()
{
	SDL_GLContext WZglcontext = SDL_GL_CreateContext(window);
	std::string glContextErrors;
	while (!WZglcontext)
	{
		glContextErrors += "Failed to create an " + to_string(contextRequest) + " context! [" + std::string(SDL_GetError()) + "]\n";
		if (!configureNextOpenGLContextRequest())
		{
			// No more context requests to try
			debug_multiline(LOG_ERROR, glContextErrors);
			return false;
		}
		WZglcontext = SDL_GL_CreateContext(window);
	}
	if (!glContextErrors.empty())
	{
		// Although context creation eventually succeeded, log the attempts that failed
		debug_multiline(LOG_3D, glContextErrors);
	}
	debug(LOG_3D, "Requested %s context", to_string(contextRequest).c_str());

	int value = 0;
	if (SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &value) == 0)
	{
		if (value == 0)
		{
			debug(LOG_FATAL, "OpenGL initialization did not give double buffering! (%d)", value);
			debug(LOG_FATAL, "Double buffering is required for this game!");
			return false;
		}
	}
	else
	{
		// SDL_GL_GetAttribute failed for SDL_GL_DOUBLEBUFFER
		// For desktop OpenGL, treat this as a fatal error
		code_part log_type = LOG_FATAL;
		if (isOpenGLES())
		{
			// For OpenGL ES (EGL?), log this as an error and let execution continue
			log_type = LOG_ERROR;
		}
		debug(log_type, "SDL_GL_GetAttribute failed to get value for SDL_GL_DOUBLEBUFFER (%s)", SDL_GetError());
		debug(log_type, "Double buffering is required for this game - if it isn't actually enabled, things will fail!");
		if (log_type == LOG_FATAL)
		{
			return false;
		}
	}

	if (SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &value) != 0)
	{
		debug(LOG_3D, "Failed to get value for SDL_GL_DEPTH_SIZE (%s)", SDL_GetError());
	}
	debug(LOG_3D, "Current value for SDL_GL_DEPTH_SIZE: (%d)", value);
	
	if (SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &value) != 0)
	{
		debug(LOG_3D, "Failed to get value for SDL_GL_STENCIL_SIZE (%s)", SDL_GetError());
	}
	debug(LOG_3D, "Current value for SDL_GL_STENCIL_SIZE: (%d)", value);

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
