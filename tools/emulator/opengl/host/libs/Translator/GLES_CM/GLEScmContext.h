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
#ifndef GLES_CM_CONTEX_H
#define GLES_CM_CONTEX_H

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLESpointer.h>
#include <GLcommon/GLESbuffer.h>
#include <GLcommon/GLEScontext.h>
#include <map>
#include <vector>
#include <string>
#include <utils/threads.h>


typedef std::map<GLfloat,std::vector<int> > PointSizeIndices;

class GLEScmContext: public GLEScontext
{
public:
    void init();
    GLEScmContext();

    void setActiveTexture(GLenum tex);
    void  setClientActiveTexture(GLenum tex);
    GLenum  getActiveTexture() { return GL_TEXTURE0 + m_activeTexture;};
    GLenum  getClientActiveTexture() { return GL_TEXTURE0 + m_clientActiveTexture;};
    void convertArrs(GLESFloatArrays& fArrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices,bool direct);
    void drawPointsArrs(GLESFloatArrays& arrs,GLint first,GLsizei count);
    void drawPointsElems(GLESFloatArrays& arrs,GLsizei count,GLenum type,const GLvoid* indices);
  
    ~GLEScmContext();

private:
    void sendArr(GLvoid* arr,GLenum arrayType,GLint size,GLsizei stride,int pointsIndex = -1);
    void drawPoints(PointSizeIndices* points);
    void drawPointsData(GLESFloatArrays& arrs,GLint first,GLsizei count,GLenum type,const GLvoid* indices_in,bool isElemsDraw);
    void initExtensionString();

    GLESpointer*          m_texCoords;
    int                   m_pointsIndex;
    unsigned int          m_clientActiveTexture;
};

#endif

