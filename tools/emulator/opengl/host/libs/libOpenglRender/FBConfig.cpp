/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "FBConfig.h"
#include "FrameBuffer.h"
#include "EGLDispatch.h"
#include <stdio.h>

FBConfig **FBConfig::s_fbConfigs = NULL;
int FBConfig::s_numConfigs = 0;

const GLuint FBConfig::s_configAttribs[] = {
    EGL_DEPTH_SIZE,     // must be first - see getDepthSize()
    EGL_STENCIL_SIZE,   // must be second - see getStencilSize()
    EGL_RENDERABLE_TYPE, // must be third - see getRenderableType()
    EGL_SURFACE_TYPE,  // must be fourth - see getSurfaceType()
    EGL_BUFFER_SIZE,
    EGL_ALPHA_SIZE,
    EGL_BLUE_SIZE,
    EGL_GREEN_SIZE,
    EGL_RED_SIZE,
    EGL_CONFIG_CAVEAT,
    EGL_CONFIG_ID,
    EGL_LEVEL,
    EGL_MAX_PBUFFER_HEIGHT,
    EGL_MAX_PBUFFER_PIXELS,
    EGL_MAX_PBUFFER_WIDTH,
    EGL_NATIVE_RENDERABLE,
    EGL_NATIVE_VISUAL_ID,
    EGL_NATIVE_VISUAL_TYPE,
    EGL_SAMPLES,
    EGL_SAMPLE_BUFFERS,
    EGL_TRANSPARENT_TYPE,
    EGL_TRANSPARENT_BLUE_VALUE,
    EGL_TRANSPARENT_GREEN_VALUE,
    EGL_TRANSPARENT_RED_VALUE,
    EGL_BIND_TO_TEXTURE_RGB,
    EGL_BIND_TO_TEXTURE_RGBA,
    EGL_MIN_SWAP_INTERVAL,
    EGL_MAX_SWAP_INTERVAL,
    EGL_LUMINANCE_SIZE,
    EGL_ALPHA_MASK_SIZE,
    EGL_COLOR_BUFFER_TYPE,
    //EGL_MATCH_NATIVE_PIXMAP,
    EGL_CONFORMANT
};

const int FBConfig::s_numConfigAttribs = sizeof(FBConfig::s_configAttribs) / sizeof(GLuint);

InitConfigStatus FBConfig::initConfigList(FrameBuffer *fb)
{
    InitConfigStatus ret = INIT_CONFIG_FAILED;

    if (!fb) {
        return ret;
    }

    const FrameBufferCaps &caps = fb->getCaps();
    EGLDisplay dpy = fb->getDisplay();

    if (dpy == EGL_NO_DISPLAY) {
        fprintf(stderr,"Could not get EGL Display\n");
        return ret;
    }

    //
    // Query the set of configs in the EGL backend
    //
    EGLint nConfigs;
    if (!s_egl.eglGetConfigs(dpy, NULL, 0, &nConfigs)) {
        fprintf(stderr, "Could not get number of available configs\n");
        return ret;
    }
    EGLConfig *configs = new EGLConfig[nConfigs];
    s_egl.eglGetConfigs(dpy, configs, nConfigs, &nConfigs);

    //
    // Find number of usable configs which support pbuffer rendering
    // for each ES and ES2 as well as number of configs supporting
    // EGL_BIND_TO_TEXTURE_RGBA for each of ES and ES2.
    //
    const int GL = 0;
    const int GL1 = 1;
    const int GL2 = 2;
    int numPbuf[3] = {0, 0, 0};
    int numBindToTexture[3] = {0, 0, 0};
    for (int i=0; i<nConfigs; i++) {
        GLint depthSize, stencilSize;
        GLint renderType, surfaceType;
        GLint bindToTexture;

        s_egl.eglGetConfigAttrib(dpy, configs[i], EGL_DEPTH_SIZE, &depthSize);
        s_egl.eglGetConfigAttrib(dpy, configs[i], EGL_STENCIL_SIZE, &stencilSize);
        s_egl.eglGetConfigAttrib(dpy, configs[i], EGL_RENDERABLE_TYPE, &renderType);
        s_egl.eglGetConfigAttrib(dpy, configs[i], EGL_SURFACE_TYPE, &surfaceType);
        if (depthSize > 0 && stencilSize > 0 &&
            (surfaceType & EGL_PBUFFER_BIT) != 0) {

            numPbuf[GL]++;

            if ((renderType & EGL_OPENGL_ES_BIT) != 0) {
                numPbuf[GL1]++;
            }

            if ((renderType & EGL_OPENGL_ES2_BIT) != 0) {
                numPbuf[GL2]++;
            }

            s_egl.eglGetConfigAttrib(dpy, configs[i],
                                     EGL_BIND_TO_TEXTURE_RGBA, &bindToTexture);
            if (bindToTexture) {
                numBindToTexture[GL]++;
                if ((renderType & EGL_OPENGL_ES_BIT) != 0) {
                    numBindToTexture[GL1]++;
                }

                if ((renderType & EGL_OPENGL_ES2_BIT) != 0) {
                    numBindToTexture[GL2]++;
                }
            }
        }
    }

    bool useOnlyPbuf = false;
    bool useOnlyBindToTexture = false;
    int numConfigs = nConfigs;
    if ( numPbuf[GL1] > 0 &&
        (!caps.hasGL2 || numPbuf[GL2] > 0)) {
        useOnlyPbuf = true;
        numConfigs = numPbuf[GL];
    }
    if (useOnlyPbuf &&
        !(caps.has_eglimage_texture_2d &&
          caps.has_eglimage_renderbuffer) &&
        numBindToTexture[GL1] > 0 &&
        (!caps.hasGL2 || numBindToTexture[GL2] > 0)) {
        useOnlyBindToTexture = true;
        numConfigs = numBindToTexture[GL];
        ret = INIT_CONFIG_HAS_BIND_TO_TEXTURE;
    }
    else {
        ret = INIT_CONFIG_PASSED;
    }

    int j = 0;
    s_fbConfigs = new FBConfig*[nConfigs];
    for (int i=0; i<nConfigs; i++) {
        if (useOnlyBindToTexture) {
            EGLint bindToTexture;
            s_egl.eglGetConfigAttrib(dpy, configs[i],
                                     EGL_BIND_TO_TEXTURE_RGBA, &bindToTexture);
            if (!bindToTexture) continue;
        }
        else if (useOnlyPbuf) {
            EGLint surfaceType;
            s_egl.eglGetConfigAttrib(dpy, configs[i],
                                     EGL_SURFACE_TYPE, &surfaceType);
            if (!(surfaceType & EGL_PBUFFER_BIT)) continue;
        }

        s_fbConfigs[j++] = new FBConfig(dpy, configs[i]);
    }
    s_numConfigs = j;

    delete configs;
    return ret;
}

const FBConfig *FBConfig::get(int p_config)
{
    if (p_config >= 0 && p_config < s_numConfigs) {
        return s_fbConfigs[p_config];
    }
    return NULL;
}

int FBConfig::getNumConfigs()
{
    return s_numConfigs;
}

void FBConfig::packConfigsInfo(GLuint *buffer)
{
    memcpy(buffer, s_configAttribs, s_numConfigAttribs * sizeof(GLuint));
    for (int i=0; i<s_numConfigs; i++) {
        memcpy(buffer+(i+1)*s_numConfigAttribs,
               &s_fbConfigs[i]->m_attribValues,
               s_numConfigAttribs * sizeof(GLuint));
    }
}

FBConfig::FBConfig(EGLDisplay p_eglDpy, EGLConfig p_eglCfg)
{
    m_eglConfig = p_eglCfg;
    m_attribValues = new GLint[s_numConfigAttribs];
    for (int i=0; i<s_numConfigAttribs; i++) {
        s_egl.eglGetConfigAttrib(p_eglDpy, p_eglCfg, s_configAttribs[i], &m_attribValues[i]);

        //
        // All exported configs supports android native window rendering
        //
        if (s_configAttribs[i] == EGL_SURFACE_TYPE) {
            m_attribValues[i] |= EGL_WINDOW_BIT;
        }
    }
}

FBConfig::~FBConfig()
{
    if (m_attribValues) {
        delete m_attribValues;
    }
}
