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

#include "faker.h"
#include <X11/Xmd.h>
#include <GL/glxproto.h>
#include "faker-sym.h"


typedef struct _VGLFBConfig *VGLFBConfig;


void CheckEGLErrors(Display *dpy, unsigned short minorCode);

GLXContext VGLCreateContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs);

GLXPbuffer VGLCreatePbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs);

int VGLGetFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
	int *value);

Bool VGLQueryExtension(Display *dpy, int *majorOpcode, int *eventBase,
	int *errorBase);


static INLINE void VGLDestroyContext(Display *dpy, GLXContext ctx)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		_eglDestroyContext((EGLDisplay)DPY3D, (EGLContext)ctx);
		CheckEGLErrors(dpy, X_GLXDestroyContext);
	}
	else
		_glXDestroyContext(DPY3D, ctx);
}


static INLINE void VGLDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		_eglDestroySurface((EGLDisplay)DPY3D, (EGLSurface)pbuf);
		CheckEGLErrors(dpy, X_GLXDestroyPbuffer);
	}
	else _glXDestroyPbuffer(DPY3D, pbuf);
}


static INLINE GLXContext VGLGetCurrentContext(void)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		return (GLXContext)_eglGetCurrentContext();
	}
	else
		return _glXGetCurrentContext();
}


static INLINE GLXDrawable VGLGetCurrentDrawable(void)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		return (GLXDrawable)_eglGetCurrentSurface(EGL_DRAW);
	}
	else
		return _glXGetCurrentDrawable();
}


static INLINE GLXDrawable VGLGetCurrentReadDrawable(void)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		return (GLXDrawable)_eglGetCurrentSurface(EGL_READ);
	}
	else
		return _glXGetCurrentReadDrawable();
}


static INLINE Bool VGLIsDirect(GLXContext ctx)
{
	if(fconfig.egl)
		return True;
	else
		return _glXIsDirect(DPY3D, ctx);
}


static INLINE Bool VGLMakeCurrent(Display *dpy, GLXDrawable draw,
	GLXDrawable read, GLXContext ctx)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");
		EGLBoolean ret = (Bool)_eglMakeCurrent((EGLDisplay)DPY3D, (EGLSurface)draw,
			(EGLSurface)read, (EGLContext)ctx);
		CheckEGLErrors(dpy, X_GLXMakeContextCurrent);
		return ret;
	}
	else
		return _glXMakeContextCurrent(DPY3D, draw, read, ctx);
}

#endif  // __VGLWRAP_H__
