// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005, 2006 Sun Microsystems, Inc.
// Copyright (C)2011, 2014-2015, 2018-2019 D. R. Commander
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

// This is a container class which allows us to temporarily swap in a new
// drawable and then "pop the stack" after we're done with it.  It does nothing
// unless there is already a valid context established

#ifndef __TEMPCONTEXT_H__
#define __TEMPCONTEXT_H__

#include "faker-sym.h"
#include "ContextHash.h"


#define EXISTING_DRAWABLE  (GLXDrawable)-1


namespace vglserver
{
	class TempContext
	{
		public:

			TempContext(Display *dpy_, GLXDrawable draw, GLXDrawable read,
				GLXContext ctx, VGLFBConfig config) : dpy(dpy_),
				oldctx(VGLGetCurrentContext()), newctx(NULL),
				oldread(VGLGetCurrentReadDrawable()),
				olddraw(VGLGetCurrentDrawable()), ctxChanged(false)
			{
				if(read == EXISTING_DRAWABLE) read = oldread;
				if(draw == EXISTING_DRAWABLE) draw = olddraw;
				if(draw && read && !ctx && config)
					newctx = ctx = VGLCreateContext(dpy, config, NULL, True, NULL);
				if(((read || draw) && ctx)
					&& (oldread != read  || olddraw != draw || oldctx != ctx))
				{
					if(!VGLMakeCurrent(dpy, draw, read, ctx))
						THROW("Could not bind OpenGL context to window (window may have disappeared)");
					// If oldctx has already been destroyed, then we don't want to
					// restore it.  This can happen if the application is rendering to
					// the front buffer and glXDestroyContext() is called to destroy the
					// active context before glXMake*Current*() is called to switch it.
					if(oldctx && ctxhash.findConfig(oldctx))
						ctxChanged = true;
				}
			}

			void restore(void)
			{
				if(ctxChanged)
				{
					VGLMakeCurrent(dpy, olddraw, oldread, oldctx);
					ctxChanged = false;
				}
				if(newctx)
				{
					VGLDestroyContext(dpy, newctx);  newctx = NULL;
				}
			}

			~TempContext(void)
			{
				restore();
			}

		private:

			Display *dpy;
			GLXContext oldctx, newctx;
			GLXDrawable oldread, olddraw;
			bool ctxChanged;
	};
}

#endif  // __TEMPCONTEXT_H__
