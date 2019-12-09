// Copyright (C)2019 D. R. Commander
//
// This library is free software and may be redistributed and/or modified under
// the terms of the wxWindows Library License, Version 3.1 or (at your option)
// any later version.  The full license is in the LICENSE.txt file included
// with this distribution.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// wxWindows Library License for more details.

#ifndef __VIRTUALPBUFFER_H__
#define __VIRTUALPBUFFER_H__

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>
#include <EGL/egl.h>
#include <vulkan/vulkan.h>


namespace vglserver
{
	class VirtualPbuffer
	{
		public:

			VirtualPbuffer(VGLFBConfig config, const int *glxAttribs);
			~VirtualPbuffer(void);
			GLXPbuffer getGLXPbuffer(void) { return glxpb; }

		private:

			checkEGLErrors(void);

			GLXPbuffer glxpb;
			EGLSurface eglpb;
			// 0 = front left, 1 = back left, 2 = front right, 3 = back right
			VkImage cimage[4], dimage;
			GLuint fbo, rboc[4], rbod;
			int width, height;
			VkInstance instance;
			VkDevice device;
};

#endif  // __VIRTUALPBUFFER_H__
