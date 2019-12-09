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

// These functions wrap similar EGL and GLX functions in order to provide a
// common interface for VirtualGL.

#ifndef __VGLWRAP_H__
#define __VGLWRAP_H__

#define GL_GLEXT_PROTOTYPES
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>


typedef struct _VGLFBConfig *VGLFBConfig;

typedef struct _VGLPbuffer
{
	union
	{
		GLXPbuffer glx;
		EGLSurface egl;
	} pb;
	// 0 = front left, 1 = back left, 2 = front right, 3 = back right
	GLuint fbo, rboc[4], rbod;
} *VGLPbuffer;


void CheckEGLErrors(Display *dpy, unsigned short minorCode);

GLXContext VGLCreateContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs);

VGLPbuffer VGLCreatePbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs);

void VGLDestroyContext(Display *dpy, GLXContext ctx);

void VGLDestroyPbuffer(Display *dpy, VGLPbuffer pbuf);

GLXContext VGLGetCurrentContext(void);

GLXDrawable VGLGetCurrentDrawable(void);

GLXDrawable VGLGetCurrentReadDrawable(void);

int VGLGetFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
	int *value);

Bool VGLIsDirect(GLXContext ctx);

Bool VGLMakeCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read,
	GLXContext ctx);

Bool VGLQueryExtension(Display *dpy, int *majorOpcode, int *eventBase,
	int *errorBase);

#endif  // __VGLWRAP_H__
