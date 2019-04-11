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

#include "vglwrap.h"
#include "ContextHash.h"
#include "GLXDrawableHash.h"
#include "PixmapHash.h"
#include "WindowHash.h"
#include "glxvisual.h"
#include <X11/Xmd.h>
#include <GL/glxproto.h>

#ifndef X_GLXCreateContextAttribsARB
#define X_GLXCreateContextAttribsARB  34
#endif

using namespace vglserver;


void CheckEGLErrors(Display *dpy, CARD16 minorCode)
{
	// NOTE: The error codes that throw an exception should never occur except
	// due to a bug in VirtualGL.
	switch(_eglGetError())
	{
		case EGL_SUCCESS:
			return;
		case EGL_NOT_INITIALIZED:
			THROW("EGL_NOT_INITIALIZED");
		case EGL_BAD_ACCESS:
			vglfaker::sendGLXError(dpy, minorCode, BadAccess, true);
			break;
		case EGL_BAD_ALLOC:
			vglfaker::sendGLXError(dpy, minorCode, BadAlloc, true);
			break;
		case EGL_BAD_ATTRIBUTE:
			THROW("EGL_BAD_ATTRIBUTE");
		case EGL_BAD_CONTEXT:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadContext, false);
			break;
		case EGL_BAD_CONFIG:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadFBConfig, false);
			break;
		case EGL_BAD_CURRENT_SURFACE:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadCurrentDrawable, false);
			break;
		case EGL_BAD_DISPLAY:
			THROW("EGL_BAD_DISPLAY");
		case EGL_BAD_MATCH:
			vglfaker::sendGLXError(dpy, minorCode, BadMatch, true);
			break;
		case EGL_BAD_SURFACE:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadDrawable, false);
			break;
		case EGL_BAD_PARAMETER:
			vglfaker::sendGLXError(dpy, minorCode, BadValue, true);
			break;
		case EGL_BAD_NATIVE_PIXMAP:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadPixmap, false);
			break;
		case EGL_BAD_NATIVE_WINDOW:
			vglfaker::sendGLXError(dpy, minorCode, GLXBadWindow, false);
			break;
		case EGL_CONTEXT_LOST:
			THROW("EGL_CONTEXT_LOST");
	}
}


int CheckEGLErrors_NonFatal(void)
{
	// NOTE: The error codes that throw an exception should never occur except
	// due to a bug in VirtualGL.
	switch(_eglGetError())
	{
		case EGL_SUCCESS:
			return Success;
		case EGL_NOT_INITIALIZED:
			THROW("EGL_NOT_INITIALIZED");
		case EGL_BAD_ATTRIBUTE:
			return GLX_BAD_ATTRIBUTE;
		case EGL_BAD_CONFIG:
			THROW("EGL_BAD_CONFIG");
		case EGL_BAD_DISPLAY:
			THROW("EGL_BAD_DISPLAY");
		default:
			THROW("EGL error");
	}
}


GLXContext VGLCreateContext(Display *dpy, VGLFBConfig config, GLXContext share,
	Bool direct, const int *glxAttribs)
{
	if(fconfig.egl)
	{
		if(!direct) return NULL;

		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");

		int eglAttribs[256], egli = 0;
		for(int i = 0; i < 256; i++) eglAttribs[i] = EGL_NONE;

		if(glxAttribs && glxAttribs[0] != None)
		{
			for(int glxi = 0; glxAttribs[glxi] && egli < 256; glxi += 2)
			{
				switch(glxAttribs[glxi])
				{
					case GLX_CONTEXT_MAJOR_VERSION_ARB:
						eglAttribs[egli++] = EGL_CONTEXT_MAJOR_VERSION;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
					case GLX_CONTEXT_MINOR_VERSION_ARB:
						eglAttribs[egli++] = EGL_CONTEXT_MINOR_VERSION;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
					case GLX_CONTEXT_FLAGS_ARB:
					{
						int flags = glxAttribs[++glxi];
						if(flags & GLX_CONTEXT_DEBUG_BIT_ARB)
						{
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_DEBUG;
							eglAttribs[egli++] = EGL_TRUE;
						}
						if(flags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
						{
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE;
							eglAttribs[egli++] = EGL_TRUE;
						}
						if(flags & GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB)
						{
							eglAttribs[egli++] = EGL_CONTEXT_OPENGL_ROBUST_ACCESS;
							eglAttribs[egli++] = EGL_TRUE;
						}
						break;
					}
					case GLX_CONTEXT_PROFILE_MASK_ARB:
						// The mask bits are the same for GLX_ARB_create_context and EGL.
						eglAttribs[egli++] = EGL_CONTEXT_OPENGL_PROFILE_MASK;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
					case GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB:
						switch(glxAttribs[++glxi])
						{
							case GLX_NO_RESET_NOTIFICATION_ARB:
								eglAttribs[egli++] =
									EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY;
								eglAttribs[egli++] = EGL_NO_RESET_NOTIFICATION;
								break;
							case GLX_LOSE_CONTEXT_ON_RESET_ARB:
								eglAttribs[egli++] =
									EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY;
								eglAttribs[egli++] = EGL_LOSE_CONTEXT_ON_RESET;
								break;
						}
				}
			}
		}
		GLXContext ctx = (GLXContext)_eglCreateContext((EGLDisplay)DPY3D,
			EGLFBC(config), (EGLContext)share, egli ? eglAttribs : NULL);
		CheckEGLErrors(dpy, egli ? X_GLXCreateContextAttribsARB :
			X_GLXCreateNewContext);
		return ctx;
	}
	else
	{
		if(glxAttribs && glxAttribs[0] != None)
			return _glXCreateContextAttribsARB(DPY3D, GLXFBC(config), share, direct,
				glxAttribs);
		else
			return _glXCreateNewContext(DPY3D, GLXFBC(config), GLX_RGBA_TYPE, share,
				direct);
	}
}


GLXPbuffer VGLCreatePbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs)
{
	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");

		int eglAttribs[256], egli = 0;
		for(int i = 0; i < 256; i++) eglAttribs[i] = EGL_NONE;

		if(glxAttribs && glxAttribs[0] != None)
		{
			for(int glxi = 0; glxAttribs[glxi] && egli < 256; glxi += 2)
			{
				switch(glxAttribs[glxi])
				{
					case GLX_PBUFFER_WIDTH:
						eglAttribs[egli++] = EGL_WIDTH;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
					case GLX_PBUFFER_HEIGHT:
						eglAttribs[egli++] = EGL_HEIGHT;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
					case GLX_LARGEST_PBUFFER:
						eglAttribs[egli++] = EGL_LARGEST_PBUFFER;
						eglAttribs[egli++] = glxAttribs[++glxi];
						break;
				}
			}
		}
		GLXPbuffer ret = (GLXPbuffer)_eglCreatePbufferSurface((EGLDisplay)DPY3D,
			EGLFBC(config), egli ? eglAttribs : NULL);
		CheckEGLErrors(dpy, X_GLXCreatePbuffer);
		return ret;
	}
	else return _glXCreatePbuffer(DPY3D, GLXFBC(config), glxAttribs);
}


int VGLGetFBConfigAttrib(Display *dpy, VGLFBConfig config, int attribute,
	int *value)
{
	if(fconfig.egl)
	{
		if(!value) return GLX_BAD_VALUE;

		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");

		switch(attribute)
		{
			case GLX_FBCONFIG_ID:
				*value = config->id;
				return Success;
			case GLX_BUFFER_SIZE:
				*value = config->attr.redSize + config->attr.greenSize +
					config->attr.blueSize + config->attr.alphaSize;
				return Success;
			case GLX_DOUBLEBUFFER:
				*value = config->attr.doubleBuffer;
				return Success;
			case GLX_STEREO:
				*value = config->attr.stereo;
				return Success;
			case GLX_RED_SIZE:
				*value = config->attr.redSize;
				return Success;
			case GLX_GREEN_SIZE:
				*value = config->attr.greenSize;
				return Success;
			case GLX_BLUE_SIZE:
				*value = config->attr.blueSize;
				return Success;
			case GLX_ALPHA_SIZE:
				*value = config->attr.alphaSize;
				return Success;
			case GLX_DEPTH_SIZE:
				*value = config->attr.depthSize;
				return Success;
			case GLX_STENCIL_SIZE:
				*value = config->attr.stencilSize;
				return Success;
			case GLX_SAMPLES:
				*value = config->attr.samples;
				return Success;
			case GLX_SAMPLE_BUFFERS:
				*value = !!config->attr.samples;
				return Success;
			case GLX_RENDER_TYPE:
				*value = GLX_RGBA_BIT;
				return Success;
			case GLX_DRAWABLE_TYPE:
				*value = GLX_PBUFFER_BIT |
					(config->visualID ? GLX_WINDOW_BIT | GLX_PIXMAP_BIT : 0);
				return Success;
			case GLX_X_RENDERABLE:
				*value = !!config->visualID;
				return Success;
			case GLX_X_VISUAL_TYPE:
				*value = config->c_class == TrueColor ?
					GLX_TRUE_COLOR : GLX_DIRECT_COLOR;
				return Success;
			case GLX_CONFIG_CAVEAT:
			case GLX_TRANSPARENT_TYPE:
				*value = GLX_NONE;
				return Success;
			case GLX_MAX_PBUFFER_WIDTH:
			{
				int ret = _eglGetConfigAttrib((EGLDisplay)DPY3D, EGLFBC(config),
					EGL_MAX_PBUFFER_WIDTH, value);
				return ret == EGL_TRUE ? Success : CheckEGLErrors_NonFatal();
			}
			case GLX_MAX_PBUFFER_HEIGHT:
			{
				int ret = _eglGetConfigAttrib((EGLDisplay)DPY3D, EGLFBC(config),
					EGL_MAX_PBUFFER_HEIGHT, value);
				return ret == EGL_TRUE ? Success : CheckEGLErrors_NonFatal();
			}
			case GLX_MAX_PBUFFER_PIXELS:
			{
				int ret = _eglGetConfigAttrib((EGLDisplay)DPY3D, EGLFBC(config),
					EGL_MAX_PBUFFER_PIXELS, value);
				return ret == EGL_TRUE ? Success : CheckEGLErrors_NonFatal();
			}
			default:
				return GLX_BAD_ATTRIBUTE;
		}
	}  // fconfig.egl
	else return _glXGetFBConfigAttrib(DPY3D, GLXFBC(config), attribute, value);
}


Bool VGLQueryExtension(Display *dpy, int *majorOpcode, int *eventBase,
	int *errorBase)
{
	if(fconfig.egl)
	{
		// If the 2D X server has a GLX extension, then we hijack its major opcode
		// and error base.
		Bool retval = _XQueryExtension(dpy, "GLX", majorOpcode, eventBase,
			errorBase);
		if(retval) return retval;
		// Otherwise, we have no choice but to create a work of fiction.  We
		// assign the maximum possible values for the major opcode and error base,
		// in order to minimize the odds of stomping on another X extension.
		*majorOpcode = 255;
		*eventBase = 0;
		*errorBase = LastExtensionError - __GLX_NUMBER_ERRORS + 1;
		return True;
	}
	// In GLX mode, all GLX errors will come from the 3D X server.
	else return _XQueryExtension(DPY3D, "GLX", majorOpcode, eventBase,
		errorBase);
}
