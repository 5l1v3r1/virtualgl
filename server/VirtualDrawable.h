// Copyright (C)2004 Landmark Graphics Corporation
// Copyright (C)2005 Sun Microsystems, Inc.
// Copyright (C)2009-2015, 2017-2019 D. R. Commander
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

#ifndef __VIRTUALDRAWABLE_H__
#define __VIRTUALDRAWABLE_H__

#include "glxvisual.h"
#include "Mutex.h"
#include "X11Trans.h"
#include "fbx.h"
#include "Frame.h"


namespace vglserver
{
	class VirtualDrawable
	{
		public:

			VirtualDrawable(Display *dpy, Drawable x11Draw);
			~VirtualDrawable(void);
			int init(int width, int height, VGLFBConfig config);
			void setDirect(Bool direct);
			void clear(void);
			Display *getX11Display(void);
			Drawable getX11Drawable(void);
			GLXDrawable getGLXDrawable(void);
			int getFBConfigID(void);
			void copyPixels(GLint srcX, GLint srcY, GLint width, GLint height,
				GLint destX, GLint destY, GLXDrawable draw);
			int getWidth(void) { return oglDraw ? oglDraw->getWidth() : -1; }
			int getHeight(void) { return oglDraw ? oglDraw->getHeight() : -1; }
			GLuint getFBO(void) { return oglDraw ? oglDraw->getFBO() : 0; }
			bool isInit(void) { return direct == True || direct == False; }
			void setEventMask(unsigned long mask) { eventMask = mask; }
			unsigned long getEventMask(void) { return eventMask; }

			void makeCurrent(bool draw, bool read)
			{
				if(oglDraw) oglDraw->makeCurrent(draw, read);
			}

			bool drawingToFront(void)
			{
				return oglDraw ? oglDraw->drawingToFront() : false;
			}

			bool drawingToRight(void)
			{
				return oglDraw ? oglDraw->drawingToRight() : false;
			}

			void setDrawBuffer(GLenum mode)
			{
				if(oglDraw) oglDraw->setDrawBuffer(mode);
			}

			void setReadBuffer(GLenum mode)
			{
				if(oglDraw) oglDraw->setReadBuffer(mode);
			}

		protected:

			// A container class for the actual off-screen drawable
			class OGLDrawable
			{
				public:

					OGLDrawable(Display *dpy, int width, int height, VGLFBConfig config);
					OGLDrawable(int width, int height, int depth, VGLFBConfig config,
						const int *attribs);
					~OGLDrawable(void);
					GLXDrawable getGLXDrawable(void) { return glxDraw; }

					Pixmap getPixmap(void)
					{
						if(!isPixmap) THROW("Not a pixmap");
						return pm;
					}

					int getWidth(void) { return width; }
					int getHeight(void) { return height; }
					int getDepth(void) { return depth; }
					int getRGBSize(void) { return rgbSize; }
					VGLFBConfig getFBConfig(void) { return config; }
					int getFBO(void) { return fbo; }

					void clear(void);
					void swap(void);
					void makeCurrent(bool draw, bool read);
					bool isStereo(void) { return stereo; }
					GLenum getFormat(void) { return glFormat; }
					bool drawingToFront(void);
					bool drawingToRight(void);
					void setDrawBuffer(GLenum mode);
					void setReadBuffer(GLenum mode);

				private:

					void setVisAttribs(void);
					void createBuffers(void);

					bool cleared, stereo, doubleBuffer;
					GLXDrawable glxDraw;
					GLuint fbo, rboc[4], rbod;
					Display *dpy;
					int width, height, depth, rgbSize;
					VGLFBConfig config;
					GLenum glFormat;
					Pixmap pm;
					Window win;
					bool isPixmap;
			};

			void readPixels(GLint x, GLint y, GLint width, GLint pitch, GLint height,
				GLenum glFormat, PF *pf, GLubyte *bits, GLint readBuf, bool stereo);

			vglutil::CriticalSection mutex;
			Display *dpy;  Drawable x11Draw;
			OGLDrawable *oglDraw;  VGLFBConfig config;
			GLXContext ctx;
			Bool direct;
			X11Trans *x11Trans;
			vglcommon::Profiler profReadback;
			int autotestFrameCount;

			GLuint pbo;
			int numSync, numFrames, lastFormat;
			bool usePBO;
			bool alreadyPrinted, alreadyWarned, alreadyWarnedRenderMode;
			const char *ext;
			unsigned long eventMask;
	};
}

#endif  // __VIRTUALDRAWABLE_H__
