// Wasim Abbas
// http://www.waZim.com
// Copyright (c) 2019
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the 'Software'),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the Software
// is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Version: 1.0.0

#pragma once

#define MODEL3D_CLIENT_GL_RENDERER
//#define MODEL3D_CLIENT_GLES2_RENDERER
//#define MODEL3D_CLIENT_GLES3_RENDERER

#if defined MODEL3D_CLIENT_GL_RENDERER
#	if defined WIN32
#		include <gl/glew.h>
#	elif defined __APPLE__
// #include <OpenGL/OpenGL.h>
#		include <OpenGL/gl3.h>
#	elif defined android
#		error "Can't render on android using OpenGL"
#	elif defined __linux__
#		define GL_GLEXT_PROTOTYPES
#		include <GL/gl.h>
#	else
#		include <GL/gl.h>
#	endif
#elif defined MODEL3D_CLIENT_GLES2_RENDERER
#	include <EGL/egl.h>
#	include <GLES2/gl2.h>
#	include <GLES2/gl2ext.h>
#elif defined MODEL3D_CLIENT_GLES3_RENDERER
#	include <EGL/egl.h>
#	include <GLES3/gl3.h>
#	include <GLES3/gl3ext.h>
#endif

namespace core
{
}        // namespace core
