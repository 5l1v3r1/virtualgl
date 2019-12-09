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

#include "VirtualPbuffer.h"


#define TRY_VK(f, m) \
{ \
	VkResult result = f; \
	char temp[256]; \
	if(result != VK_SUCCESS) \
	{ \
		snprintf(temp, 256, "Could not %s (Vulkan error %d)\n", m, result); \
		THROW(temp); \
	} \
}


VirtualPbuffer::VirtualPbuffer(Display *dpy, VGLFBConfig config,
	const int *glxAttribs) : glxpb(0), eglpb(0), dimage(0), fbo(0), rbod(0),
	instance(NULL), device(NULL)
{
	for(int i = 0; i < 4; i++)
	{
		cimage[i] = 0;  rboc[i] = 0;
	}

	if(fconfig.egl)
	{
		if(!_eglBindAPI(EGL_OPENGL_API))
			THROW("Could not enable OpenGL API");

		if(glxAttribs && glxAttribs[0] != None)
		{
			for(int glxi = 0; glxAttribs[glxi]; glxi += 2)
			{
				switch(glxAttribs[glxi])
				{
					case GLX_PBUFFER_WIDTH:
						width = glxAttribs[++glxi];
						break;
					case GLX_PBUFFER_HEIGHT:
						height = glxAttribs[++glxi];
						break;
				}
			}
		}

		VkExtensionProperties *props = NULL;
		EGLContext ctx = 0;
		VkPhysicalDevice *devices = NULL;
		try
		{
			int eglAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
			eglpb = _eglCreatePbufferSurface(EDPY, EGLFBC(config), eglAttribs);
			CheckEGLErrors(dpy, X_GLXCreatePbuffer);
			if(!eglpb) THROW("Could not create EGL Pbuffer surface");

			TempContext tc(dpy, (GLXDrawable)eglpb, (GLXDrawable)eglpb, NULL,
				config);

			if(!strstr(_glGetString(GL_EXTENSIONS), "GL_EXT_memory_object"))
				THROW("GL_EXT_memory_object OpenGL extension not available");
			int numDeviceUUIDs = 0, numDriverUUIDs = 0;
			GLubyte deviceUUID[GL_UUID_SIZE_EXT], driverUUID[GL_UUID_SIZE_EXT];
			_glGetIntegerv(GL_NUM_DEVICE_UUIDS_EXT, &numDeviceUUIDs);
			if(!numDeviceUUIDs)
				THROW("GL_NUM_DEVICE_UUIDS_EXT == 0 (this shouldn't happen)");
			_glGetUnsignedBytei_vEXT(GL_DEVICE_UUID_EXT, 0, deviceUUID);
			_glGetIntegerv(GL_DRIVER_UUID_EXT, &numDriverUUIDs);
			if(!numDriverUUIDs)
				THROW("GL_NUM_DRIVER_UUIDS_EXT == 0 (this shouldn't happen)");
			_glGetUnsignedBytei_vEXT(GL_DRIVER_UUID_EXT, 0, driverUUID);

			uint32_t nExtensions = 0;
			TRY_VK(vkEnumerateInstanceExtensionProperties(NULL, &nExtensions,
				NULL), "enumerate Vulkan extensions");
			props = new VkExtensionProperties[nExtensions];
			TRY_VK(vkEnumerateInstanceExtensionProperties(NULL, &nExtensions,
				props), "enumerate Vulkan extensions");
			for(uint32_t i = 0; i < nExtensions; i++)
				if(!strcmp(props.extensionName, "VK_KHR_external_memory")) break;
			if(i == nExtensions)
				THROW("VK_KHR_external_memory Vulkan extension not available");
			delete props;  props = NULL;

			VkInstanceCreateInfo instanceci =
			{
				VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, NULL,	0, NULL, 0, NULL, 0, NULL
			};
			uint32_t nDevices = 0;
			TRY_VK(vkCreateInstance(&instanceci, NULL, &instance),
				"create Vulkan instance");
			TRY_VK(vkEnumeratePhysicalDevices(instance, &nDevices, NULL),
				"enumerate Vulkan devices");
			devices = new VkPhysicalDevice[nDevices];
			TRY_VK(vkEnumeratePhysicalDevices(instance, &nDevices, devices),
				"enumerate Vulkan devices");
			for(uint_t i = 0; i < nDevices; i++)
			{
				VkPhysicalDeviceIDPropertiesKHR deviceIDProps =
				{
					VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHR, 0
				};
				VkPhysicalDeviceProperties2KHR deviceProps =
				{
					VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR, &deviceIDProps
				};
				vkGetPhysicalDeviceProperties2KHR(devices[i], &deviceProps);
				if(!memcmp(deviceProps.deviceUUID, deviceUUID) &&
					!memcmp(deviceProps.driverUUID, driverUUID))
					device = devices[i];
			}
			if(!device) THROW("Could not match Vulkan device to EGL device");

			VkImageCreateInfo imageci =
			{
				VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, NULL, 0, VK_IMAGE_TYPE_2D,
				VK_FORMAT_UNDEFINED, { width, height, 1 }, 1, 1, config.attr.samples,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_SHARING_MODE_EXCLUSIVE, 0, NULL, VK_IMAGE_LAYOUT_UNDEFINED
			};
			imageci.format = config->attr.redSize == 10 ?
				VK_FORMAT_A2R10G10B10_UINT_PACK32 :
				(config->attr.alphaSize ?
					VK_FORMAT_R8G8B8A8_UINT : VK_FORMAT_R8G8B8_UINT);
			// 0 = front left, 1 = back left, 2 = front right, 3 = back right
			for(int i = 0; i < 2 * (!!config->attr.stereo + 1);
				i += (1 - !!config->attr.doubleBuffer + 1))
			{
				VkMemoryAllocateInfo memAllocInfo =
				{
					VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, NULL, 0, 0
				};
				VkMemoryRequirements memReqs;
				VkDeviceMemory mem;

				TRY_VK(vkCreateImage(device, &imageci, NULL, &cimage[i]),
					"create Vulkan image");
				vkGetImageMemoryRequirements(device, cimage[i], &memReqs);
				memAllocInfo
				TRY_VK(vkAllocateMemory(device, &memAllocInfo, NULL, &mem),
					"allocate Vulkan memory");
			}
			if(config->attr.depthSize)
			{
				imageci.format = VK_FORMAT_D24_UNORM_S8_UINT;
				TRY_VK(vkCreateImage(device, &imageci, NULL, dimage),
					"create Vulkan image");
			}

		}
		catch(...)
		{
			if(device)
			{
				for(int i = 0; i < 4; i++)
				{
					if(cimage[i])
					{
						vkDestroyImage(device, cimage[i], NULL);  cimage[i] = 0;
					}
				}
				if(dimage) { vkDestroyImage(device, dimage, NULL);  dimage = 0; }
			}
			if(devices) delete devices;
			if(ctx) _eglDestroyContext(EDPY, ctx);
			if(props) delete props;
			if(eglpb) { _eglDestroySurface(EDPY, eglpb);  eglpb = NULL; }
		}
	}
	else glxpb = _glXCreatePbuffer(DPY3D, GLXFBC(config), glxAttribs);
}


VirtualPbuffer::~VirtualPbuffer(void)
{
	if(fconfig.egl)
	{
		if(device)
		{
			for(int i = 0; i < 4; i++)
			{
				if(cimage[i]) vkDestroyImage(device, cimage[i], NULL);
			}
			if(dimage) vkDestroyImage(device, dimage, NULL);
		}
		if(eglpb)
		{
			_eglDestroySurface(EDPY, eglpb);
			CheckEGLErrors(dpy, X_GLXDestroyPbuffer);
		}
	}
	else
	{
		if(glxpb) _glXDestroyPbuffer(DPY3D, glxpb);
	}
}
