/*
* Copyright(C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0(the "License"){    GET_CTX();}
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

#ifdef _WIN32
#undef  GL_APICALL
#define GL_APICALL __declspec(dllexport)
#endif

#define GL_GLEXT_PROTOTYPES
#include <stdio.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLcommon/TranslatorIfaces.h>
#include <GLcommon/ThreadInfo.h>
#include "GLESv2Context.h"
#include "GLESv2Validate.h"
#include "ShaderParser.h"

extern "C" {

//decleration
static void initContext(GLEScontext* ctx);
static void deleteGLESContext(GLEScontext* ctx);
static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp);
static GLEScontext* createGLESContext();
static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName);

}

/************************************** GLES EXTENSIONS *********************************************************/
//extentions descriptor
typedef std::map<std::string, __translatorMustCastToProperFunctionPointerType> ProcTableMap;
ProcTableMap *s_glesExtensions = NULL;
/****************************************************************************************************************/

static EGLiface*  s_eglIface = NULL;
static GLESiface  s_glesIface = {
    createGLESContext:createGLESContext,
    initContext      :initContext,
    deleteGLESContext:deleteGLESContext,
    flush            :(FUNCPTR)glFlush,
    finish           :(FUNCPTR)glFinish,
    setShareGroup    :setShareGroup,
    getProcAddress   :getProcAddress
};

#include <GLcommon/GLESmacros.h>

extern "C" {

static void initContext(GLEScontext* ctx) {
    ctx->init();
}
static GLEScontext* createGLESContext() {
    return new GLESv2Context();
}

static void deleteGLESContext(GLEScontext* ctx) {
    delete ctx;
}

static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp) {
    if(ctx) {
        ctx->setShareGroup(grp);
    }
}

static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName) {
    GET_CTX_RET(NULL)
    ctx->getGlobalLock();
    static bool proc_table_initialized = false;
    if (!proc_table_initialized) {
        proc_table_initialized = true;
        if (!s_glesExtensions)
            s_glesExtensions = new ProcTableMap();
        else
            s_glesExtensions->clear();
        (*s_glesExtensions)["glEGLImageTargetTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType)glEGLImageTargetTexture2DOES;
        (*s_glesExtensions)["glEGLImageTargetRenderbufferStorageOES"]=(__translatorMustCastToProperFunctionPointerType)glEGLImageTargetRenderbufferStorageOES;
    }
    __translatorMustCastToProperFunctionPointerType ret=NULL;
    ProcTableMap::iterator val = s_glesExtensions->find(procName);
    if (val!=s_glesExtensions->end())
        ret = val->second;
    ctx->releaseGlobalLock();

    return ret;
}

GL_APICALL GLESiface* __translator_getIfaces(EGLiface* eglIface){
    s_eglIface = eglIface;
    return & s_glesIface;
}

}

GL_APICALL void  GL_APIENTRY glActiveTexture(GLenum texture){
    GET_CTX_V2();
    SET_ERROR_IF (!GLESv2Validate::textureEnum(texture,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
    ctx->setActiveTexture(texture);
    ctx->dispatcher().glActiveTexture(texture);
}

GL_APICALL void  GL_APIENTRY glAttachShader(GLuint program, GLuint shader){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        const GLuint globalShaderName  = thrd->shareGroup->getGlobalName(SHADER,shader);
        ctx->dispatcher().glAttachShader(globalProgramName,globalShaderName);
    }
}

GL_APICALL void  GL_APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar* name){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glBindAttribLocation(globalProgramName,index,name);
    }
}

GL_APICALL void  GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(target),GL_INVALID_ENUM);
    //if buffer wasn't generated before,generate one
    if(thrd->shareGroup.Ptr() && !thrd->shareGroup->isObject(VERTEXBUFFER,buffer)){
        thrd->shareGroup->genName(VERTEXBUFFER,buffer);
        thrd->shareGroup->setObjectData(VERTEXBUFFER,buffer,ObjectDataPtr(new GLESbuffer()));
    }
    ctx->bindBuffer(target,buffer);
    GLESbuffer* vbo = (GLESbuffer*)thrd->shareGroup->getObjectData(VERTEXBUFFER,buffer).Ptr();
    vbo->wasBinded();
}

GL_APICALL void  GL_APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::framebufferTarget(target),GL_INVALID_ENUM);

    GLuint globalFrameBufferName = framebuffer;
    if(framebuffer && thrd->shareGroup.Ptr()){
        globalFrameBufferName = thrd->shareGroup->getGlobalName(FRAMEBUFFER,framebuffer);
        //if framebuffer wasn't generated before,generate one
        if(!globalFrameBufferName){
            thrd->shareGroup->genName(FRAMEBUFFER,framebuffer);
            globalFrameBufferName = thrd->shareGroup->getGlobalName(FRAMEBUFFER,framebuffer);
        }
    }
    ctx->dispatcher().glBindFramebuffer(target,globalFrameBufferName);
}

GL_APICALL void  GL_APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::renderbufferTarget(target),GL_INVALID_ENUM);

    GLuint globalRenderBufferName = renderbuffer;
    if(renderbuffer && thrd->shareGroup.Ptr()){
        globalRenderBufferName = thrd->shareGroup->getGlobalName(RENDERBUFFER,renderbuffer);
        //if renderbuffer wasn't generated before,generate one
        if(!globalRenderBufferName){
            thrd->shareGroup->genName(RENDERBUFFER,renderbuffer);
            globalRenderBufferName = thrd->shareGroup->getGlobalName(RENDERBUFFER,renderbuffer);
        }
    }
    ctx->dispatcher().glBindRenderbuffer(target,globalRenderBufferName);
}

GL_APICALL void  GL_APIENTRY glBindTexture(GLenum target, GLuint texture){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTarget(target),GL_INVALID_ENUM)

    GLuint globalTextureName = texture;
    if(texture && thrd->shareGroup.Ptr()){
        globalTextureName = thrd->shareGroup->getGlobalName(TEXTURE,texture);
        //if texture wasn't generated before,generate one
        if(!globalTextureName){
            thrd->shareGroup->genName(TEXTURE,texture);
            globalTextureName = thrd->shareGroup->getGlobalName(TEXTURE,texture);
        }
    }
    ctx->setBindedTexture(globalTextureName);
    ctx->dispatcher().glBindTexture(target,globalTextureName);
}

GL_APICALL void  GL_APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){
    GET_CTX();
    ctx->dispatcher().glBlendColor(red,green,blue,alpha);
}

GL_APICALL void  GL_APIENTRY glBlendEquation( GLenum mode ){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::blendEquationMode(mode),GL_INVALID_ENUM)
    ctx->dispatcher().glBlendEquation(mode);
}

GL_APICALL void  GL_APIENTRY glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::blendEquationMode(modeRGB) && GLESv2Validate::blendEquationMode(modeAlpha)),GL_INVALID_ENUM);
    ctx->dispatcher().glBlendEquationSeparate(modeRGB,modeAlpha);
}

GL_APICALL void  GL_APIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::blendSrc(sfactor) || !GLESv2Validate::blendDst(dfactor),GL_INVALID_ENUM)
    ctx->dispatcher().glBlendFunc(sfactor,dfactor);
}

GL_APICALL void  GL_APIENTRY glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha){
    GET_CTX();
    SET_ERROR_IF(
!(GLESv2Validate::blendSrc(srcRGB) && GLESv2Validate::blendDst(dstRGB) && GLESv2Validate::blendSrc(srcAlpha) && GLESv2Validate::blendDst(dstAlpha)),GL_INVALID_ENUM);
    ctx->dispatcher().glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);
}

GL_APICALL void  GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(target),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    ctx->setBufferData(target,size,data,usage);
}

GL_APICALL void  GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data){
    GET_CTX();
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLESv2Validate::bufferTarget(target),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->setBufferSubData(target,offset,size,data),GL_INVALID_VALUE);
}


GL_APICALL GLenum GL_APIENTRY glCheckFramebufferStatus(GLenum target){
    GET_CTX_RET(GL_FRAMEBUFFER_COMPLETE);
    RET_AND_SET_ERROR_IF(!GLESv2Validate::framebufferTarget(target),GL_INVALID_ENUM,GL_FRAMEBUFFER_COMPLETE);
    return ctx->dispatcher().glCheckFramebufferStatus(target);
}

GL_APICALL void  GL_APIENTRY glClear(GLbitfield mask){
    GET_CTX();
    ctx->dispatcher().glClear(mask);
}
GL_APICALL void  GL_APIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha){
    GET_CTX();
    ctx->dispatcher().glClearColor(red,green,blue,alpha);
}
GL_APICALL void  GL_APIENTRY glClearDepthf(GLclampf depth){
    GET_CTX();
    ctx->dispatcher().glClearDepth(depth);
}
GL_APICALL void  GL_APIENTRY glClearStencil(GLint s){
    GET_CTX();
    ctx->dispatcher().glClearStencil(s);
}
GL_APICALL void  GL_APIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha){
    GET_CTX();
    ctx->dispatcher().glColorMask(red,green,blue,alpha);
}

GL_APICALL void  GL_APIENTRY glCompileShader(GLuint shader){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
       const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
       ctx->dispatcher().glCompileShader(globalShaderName);
    }
}

GL_APICALL void  GL_APIENTRY glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(target),GL_INVALID_ENUM);
    SET_ERROR_IF(border != 0 , GL_INVALID_VALUE);
    ctx->dispatcher().glCompressedTexImage2D(target,level,internalformat,width,height,border,imageSize,data);
}

GL_APICALL void  GL_APIENTRY glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(target),GL_INVALID_ENUM);
    ctx->dispatcher().glCompressedTexSubImage2D(target,level,xoffset,yoffset,width,height,format,imageSize,data);
}

GL_APICALL void  GL_APIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::pixelFrmt(ctx,internalformat) && GLESv2Validate::textureTargetEx(target)),GL_INVALID_ENUM);
    SET_ERROR_IF(border != 0,GL_INVALID_VALUE);
    ctx->dispatcher().glCopyTexImage2D(target,level,internalformat,x,y,width,height,border);
}

GL_APICALL void  GL_APIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(target),GL_INVALID_ENUM);
    ctx->dispatcher().glCopyTexSubImage2D(target,level,xoffset,yoffset,x,y,width,height);
}

GL_APICALL GLuint GL_APIENTRY glCreateProgram(void){
    GET_CTX_RET(0);
    const GLuint globalProgramName = ctx->dispatcher().glCreateProgram();
    if(thrd->shareGroup.Ptr() && globalProgramName) {
            const GLuint localProgramName = thrd->shareGroup->genName(SHADER);
            thrd->shareGroup->replaceGlobalName(SHADER,localProgramName,globalProgramName);
            return localProgramName;
    }
    if(globalProgramName){
        ctx->dispatcher().glDeleteProgram(globalProgramName);
    }
    return 0;
}

GL_APICALL GLuint GL_APIENTRY glCreateShader(GLenum type){
    GET_CTX_V2_RET(0);
    const GLuint globalShaderName = ctx->dispatcher().glCreateShader(type);
    if(thrd->shareGroup.Ptr() && globalShaderName) {
            const GLuint localShaderName = thrd->shareGroup->genName(SHADER);
            ShaderParser* sp = new ShaderParser(type);
            thrd->shareGroup->replaceGlobalName(SHADER,localShaderName,globalShaderName);
            thrd->shareGroup->setObjectData(SHADER,localShaderName,ObjectDataPtr(sp));
            return localShaderName;
    }
    if(globalShaderName){
        ctx->dispatcher().glDeleteShader(globalShaderName);
    }
    return 0;
}

GL_APICALL void  GL_APIENTRY glCullFace(GLenum mode){
    GET_CTX();
    ctx->dispatcher().glCullFace(mode);
}

GL_APICALL void  GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint* buffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i < n; i++){
           thrd->shareGroup->deleteName(VERTEXBUFFER,buffers[i]);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i < n; i++){
           const GLuint globalFrameBufferName = thrd->shareGroup->getGlobalName(FRAMEBUFFER,framebuffers[i]);
           thrd->shareGroup->deleteName(FRAMEBUFFER,framebuffers[i]);
           ctx->dispatcher().glDeleteFramebuffers(1,&globalFrameBufferName);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i < n; i++){
           const GLuint globalRenderBufferName = thrd->shareGroup->getGlobalName(RENDERBUFFER,renderbuffers[i]);
           thrd->shareGroup->deleteName(RENDERBUFFER,renderbuffers[i]);
           ctx->dispatcher().glDeleteRenderbuffers(1,&globalRenderBufferName);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i < n; i++){
           const GLuint globalTextureName = thrd->shareGroup->getGlobalName(TEXTURE,textures[i]);
           thrd->shareGroup->deleteName(TEXTURE,textures[i]);
           ctx->dispatcher().glDeleteTextures(1,&globalTextureName);
        }
    }
}

GL_APICALL void  GL_APIENTRY glDeleteProgram(GLuint program){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        thrd->shareGroup->deleteName(SHADER,program);
        ctx->dispatcher().glDeleteProgram(program);
    }
}

GL_APICALL void  GL_APIENTRY glDeleteShader(GLuint shader){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
        thrd->shareGroup->deleteName(SHADER,shader);
        ctx->dispatcher().glDeleteShader(shader);
    }
}

GL_APICALL void  GL_APIENTRY glDepthFunc(GLenum func){
    GET_CTX();
    ctx->dispatcher().glDepthFunc(func);
}
GL_APICALL void  GL_APIENTRY glDepthMask(GLboolean flag){
    GET_CTX();
    ctx->dispatcher().glDepthMask(flag);
}
GL_APICALL void  GL_APIENTRY glDepthRangef(GLclampf zNear, GLclampf zFar){
    GET_CTX();
    ctx->dispatcher().glDepthRange(zNear,zFar);
}

GL_APICALL void  GL_APIENTRY glDetachShader(GLuint program, GLuint shader){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        const GLuint globalShaderName  = thrd->shareGroup->getGlobalName(SHADER,shader);
        ctx->dispatcher().glDetachShader(globalProgramName,globalShaderName);
    }
}

GL_APICALL void  GL_APIENTRY glDisable(GLenum cap){
    GET_CTX();
    ctx->dispatcher().glDisable(cap);
}

GL_APICALL void  GL_APIENTRY glDisableVertexAttribArray(GLuint index){
    GET_CTX();
    ctx->enableArr(index,false);
    ctx->dispatcher().glDisableVertexAttribArray(index);
}


GL_APICALL void  GL_APIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count){
    GET_CTX();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!GLESv2Validate::drawMode(mode),GL_INVALID_ENUM);

    //if no vertex are enabled no need to convert anything
    if(!ctx->isArrEnabled(0)) return;

    GLESFloatArrays tmpArrs;
    ctx->convertArrs(tmpArrs,first,count,0,NULL,true);
    ctx->dispatcher().glDrawArrays(mode,first,count);
}

GL_APICALL void  GL_APIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* elementsIndices){
    GET_CTX();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!(GLESv2Validate::drawMode(mode) && GLESv2Validate::drawType(type)),GL_INVALID_ENUM);

    const GLvoid* indices = elementsIndices;
    if(ctx->isBindedBuffer(GL_ELEMENT_ARRAY_BUFFER)) { // if vbo is binded take the indices from the vbo
        const unsigned char* buf = static_cast<unsigned char *>(ctx->getBindedBuffer(GL_ELEMENT_ARRAY_BUFFER));
        indices = buf+reinterpret_cast<unsigned int>(elementsIndices);
    }

    //if no vertex are enabled no need to convert anything
    if(!ctx->isArrEnabled(0)) return;

    GLESFloatArrays tmpArrs;
    ctx->convertArrs(tmpArrs,0,count,type,indices,false);
    ctx->dispatcher().glDrawElements(mode,count,type,indices);
}

GL_APICALL void  GL_APIENTRY glEnable(GLenum cap){
    GET_CTX();
    ctx->dispatcher().glEnable(cap);
}

GL_APICALL void  GL_APIENTRY glEnableVertexAttribArray(GLuint index){
    GET_CTX();
    ctx->enableArr(index,true);
    ctx->dispatcher().glEnableVertexAttribArray(index);
}

GL_APICALL void  GL_APIENTRY glFinish(void){
    GET_CTX();
    ctx->dispatcher().glFinish();
}
GL_APICALL void  GL_APIENTRY glFlush(void){
    GET_CTX();
    ctx->dispatcher().glFlush();
}


GL_APICALL void  GL_APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(target)              &&
                   GLESv2Validate::renderbufferTarget(renderbuffertarget) &&
                   GLESv2Validate::framebufferAttachment(attachment)),GL_INVALID_ENUM);

    if(thrd->shareGroup.Ptr()) {
            GLuint globalRenderbufferName = thrd->shareGroup->getGlobalName(RENDERBUFFER,renderbuffer);
            ctx->dispatcher().glFramebufferRenderbuffer(target,attachment,renderbuffertarget,globalRenderbufferName);
    }

}

GL_APICALL void  GL_APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(target) &&
                   GLESv2Validate::textureTargetEx(textarget)  &&
                   GLESv2Validate::framebufferAttachment(attachment)),GL_INVALID_ENUM);
    SET_ERROR_IF(level != 0, GL_INVALID_VALUE);

    if(thrd->shareGroup.Ptr()) {
            GLuint globalTextureName = thrd->shareGroup->getGlobalName(TEXTURE,texture);
            ctx->dispatcher().glFramebufferTexture2D(target,attachment,textarget,globalTextureName,level);
    }
}


GL_APICALL void  GL_APIENTRY glFrontFace(GLenum mode){
    GET_CTX();
    ctx->dispatcher().glFrontFace(mode);
}

GL_APICALL void  GL_APIENTRY glGenBuffers(GLsizei n, GLuint* buffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i<n ;i++) {
            buffers[i] = thrd->shareGroup->genName(VERTEXBUFFER);
            //generating vbo object related to this buffer name
            thrd->shareGroup->setObjectData(VERTEXBUFFER,buffers[i],ObjectDataPtr(new GLESbuffer()));
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenerateMipmap(GLenum target){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetEx(target),GL_INVALID_ENUM);
    ctx->dispatcher().glGenerateMipmap(target);
}

GL_APICALL void  GL_APIENTRY glGenFramebuffers(GLsizei n, GLuint* framebuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i<n ;i++) {
            framebuffers[i] = thrd->shareGroup->genName(FRAMEBUFFER);
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenRenderbuffers(GLsizei n, GLuint* renderbuffers){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i<n ;i++) {
            renderbuffers[i] = thrd->shareGroup->genName(RENDERBUFFER);
        }
    }
}

GL_APICALL void  GL_APIENTRY glGenTextures(GLsizei n, GLuint* textures){
    GET_CTX();
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()) {
        for(int i=0; i<n ;i++) {
            textures[i] = thrd->shareGroup->genName(TEXTURE);
        }
    }
}

GL_APICALL void  GL_APIENTRY glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetActiveAttrib(globalProgramName,index,bufsize,length,size,type,name);
    }
}

GL_APICALL void  GL_APIENTRY glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetActiveUniform(globalProgramName,index,bufsize,length,size,type,name);
    }
}

GL_APICALL void  GL_APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetAttachedShaders(globalProgramName,maxcount,count,shaders);
        for(int i=0 ; i < *count ;i++){
           shaders[i] = thrd->shareGroup->getLocalName(SHADER,shaders[i]);
        }
    }
}

GL_APICALL int GL_APIENTRY glGetAttribLocation(GLuint program, const GLchar* name){
     GET_CTX_RET(-1);
     if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        return ctx->dispatcher().glGetAttribLocation(globalProgramName,name);
     }
     return -1;
}

GL_APICALL void  GL_APIENTRY glGetBooleanv(GLenum pname, GLboolean* params){
    GET_CTX();
    ctx->dispatcher().glGetBooleanv(pname,params);
}

GL_APICALL void  GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::bufferTarget(target) && GLESv2Validate::bufferParam(pname)),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    bool ret = true;
    switch(pname) {
    case GL_BUFFER_SIZE:
        ctx->getBufferSize(target,params);
        break;
    case GL_BUFFER_USAGE:
        ctx->getBufferUsage(target,params);
        break;
    }
}


GL_APICALL GLenum GL_APIENTRY glGetError(void){
    GET_CTX_RET(GL_NO_ERROR)
    GLenum err = ctx->getGLerror();
    if(err != GL_NO_ERROR) {
        ctx->setGLerror(GL_NO_ERROR);
        return err;
    }
    return ctx->dispatcher().glGetError();
}

GL_APICALL void  GL_APIENTRY glGetFloatv(GLenum pname, GLfloat* params){
    GET_CTX();
    ctx->dispatcher().glGetFloatv(pname,params);
}

GL_APICALL void  GL_APIENTRY glGetIntegerv(GLenum pname, GLint* params){
    GET_CTX();
    ctx->dispatcher().glGetIntegerv(pname,params);
}

GL_APICALL void  GL_APIENTRY glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::framebufferTarget(target)         &&
                   GLESv2Validate::framebufferAttachment(attachment) &&
                   GLESv2Validate::framebufferAttachmentParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glGetFramebufferAttachmentParameteriv(target,attachment,pname,params);
}

GL_APICALL void  GL_APIENTRY glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::renderbufferTarget(target) && GLESv2Validate::renderbufferParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glGetRenderbufferParameteriv(target,pname,params);
}


GL_APICALL void  GL_APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint* params){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetProgramiv(globalProgramName,pname,params);
    }
}

GL_APICALL void  GL_APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetProgramInfoLog(globalProgramName,bufsize,length,infolog);
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint* params){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
        ctx->dispatcher().glGetShaderiv(globalShaderName,pname,params);
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
        ctx->dispatcher().glGetShaderInfoLog(globalShaderName,bufsize,length,infolog);
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision){
    GET_CTX_V2();
    SET_ERROR_IF(!(GLESv2Validate::shaderType(shadertype) && GLESv2Validate::precisionType(precisiontype)),GL_INVALID_ENUM);
    if(ctx->glslVersion() < Version(1,30,10)){ //version 1.30.10 is the first version of GLSL Language containing precision qualifiers
        range[0] = range[1] = 0;
        precision = 0;
    } else {
        ctx->dispatcher().glGetShaderPrecisionFormat(shadertype,precisiontype,range,precision);
    }
}

GL_APICALL void  GL_APIENTRY glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
       const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
       SET_ERROR_IF(globalShaderName == 0,GL_INVALID_VALUE);
       ObjectDataPtr objData = thrd->shareGroup->getObjectData(SHADER,shader);
       SET_ERROR_IF(!objData.Ptr(),GL_INVALID_OPERATION);
       const char* src = ((ShaderParser*)objData.Ptr())->getOriginalSrc();
       int srcLength = strlen(src);
       SET_ERROR_IF(bufsize < 0 || srcLength > bufsize,GL_INVALID_VALUE);
       *length = srcLength;
       strncpy(source,src,srcLength);
    }
}


GL_APICALL const GLubyte* GL_APIENTRY glGetString(GLenum name){
    GET_CTX_RET(NULL)
    static GLubyte VENDOR[]     = "Google";
    static GLubyte RENDERER[]   = "OpenGL ES 2.0";
    static GLubyte VERSION[]    = "OpenGL ES 2.0";
    static GLubyte EXTENSIONS[] = "";
    static GLubyte SHADING[]    = "OpenGL ES GLSL ES 1.0.17";
    switch(name) {
        case GL_VENDOR:
            return VENDOR;
        case GL_RENDERER:
            return RENDERER;
        case GL_VERSION:
            return VERSION;
        case GL_SHADING_LANGUAGE_VERSION:
            return SHADING;
        case GL_EXTENSIONS:
            return EXTENSIONS;
        default:
            RET_AND_SET_ERROR_IF(true,GL_INVALID_ENUM,NULL);
    }
}

GL_APICALL void  GL_APIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glGetTexParameterfv(target,pname,params);

}
GL_APICALL void  GL_APIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glGetTexParameteriv(target,pname,params);
}

GL_APICALL void  GL_APIENTRY glGetUniformfv(GLuint program, GLint location, GLfloat* params){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetUniformfv(globalProgramName,location,params);
    }
}

GL_APICALL void  GL_APIENTRY glGetUniformiv(GLuint program, GLint location, GLint* params){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glGetUniformiv(globalProgramName,location,params);
    }
}

GL_APICALL int GL_APIENTRY glGetUniformLocation(GLuint program, const GLchar* name){
    GET_CTX_RET(-1);
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        return ctx->dispatcher().glGetUniformLocation(globalProgramName,name);
    }
    return -1;
}



GL_APICALL void  GL_APIENTRY glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params){
    GET_CTX();
    const GLESpointer* p = ctx->getPointer(pname);
    if(p) {
        switch(pname){
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = 0;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = p->isEnable();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = p->getSize();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = p->getStride();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = p->getType();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = p->isNormalize();
            break;
        case GL_CURRENT_VERTEX_ATTRIB:
            ctx->dispatcher().glGetVertexAttribfv(index,pname,params);
            break;
        default:
            ctx->setGLerror(GL_INVALID_ENUM);
        }
    } else {
        ctx->setGLerror(GL_INVALID_ENUM);
    }
}

GL_APICALL void  GL_APIENTRY glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params){
    GET_CTX();
    const GLESpointer* p = ctx->getPointer(pname);
    if(p) {
        switch(pname){
        case GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING:
            *params = 0;
            break;
        case GL_VERTEX_ATTRIB_ARRAY_ENABLED:
            *params = p->isEnable();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_SIZE:
            *params = p->getSize();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_STRIDE:
            *params = p->getStride();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_TYPE:
            *params = p->getType();
            break;
        case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED:
            *params = p->isNormalize();
            break;
        case GL_CURRENT_VERTEX_ATTRIB:
            ctx->dispatcher().glGetVertexAttribiv(index,pname,params);
            break;
        default:
            ctx->setGLerror(GL_INVALID_ENUM);
        }
    } else {
        ctx->setGLerror(GL_INVALID_ENUM);
    }
}

GL_APICALL void  GL_APIENTRY glGetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer){
    GET_CTX();
    SET_ERROR_IF(pname != GL_VERTEX_ATTRIB_ARRAY_POINTER,GL_INVALID_ENUM); 
    
    const GLESpointer* p = ctx->getPointer(pname);
    if(p) {
        *pointer = const_cast<void *>( p->getBufferData());
    } else {
        ctx->setGLerror(GL_INVALID_ENUM);
    }
}

GL_APICALL void  GL_APIENTRY glHint(GLenum target, GLenum mode){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::hintTargetMode(target,mode),GL_INVALID_ENUM);
    ctx->dispatcher().glHint(target,mode);
}

GL_APICALL GLboolean    GL_APIENTRY glIsEnabled(GLenum cap){
    GET_CTX_RET(GL_FALSE);
    RET_AND_SET_ERROR_IF(!GLESv2Validate::capability(cap),GL_INVALID_ENUM,GL_FALSE);
    return ctx->dispatcher().glIsEnabled(cap);
}

GL_APICALL GLboolean    GL_APIENTRY glIsBuffer(GLuint buffer){
    GET_CTX_RET(GL_FALSE)
    if(buffer && thrd->shareGroup.Ptr()) {
       ObjectDataPtr objData = thrd->shareGroup->getObjectData(VERTEXBUFFER,buffer);
       return objData.Ptr() ? ((GLESbuffer*)objData.Ptr())->wasBinded():GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsFramebuffer(GLuint framebuffer){
    GET_CTX_RET(GL_FALSE)
    if(framebuffer && thrd->shareGroup.Ptr()){
        return thrd->shareGroup->isObject(FRAMEBUFFER,framebuffer) ? GL_TRUE :GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsRenderbuffer(GLuint renderbuffer){
    GET_CTX_RET(GL_FALSE)
    if(renderbuffer && thrd->shareGroup.Ptr()){
        return thrd->shareGroup->isObject(RENDERBUFFER,renderbuffer) ? GL_TRUE :GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsTexture(GLuint texture){
    GET_CTX_RET(GL_FALSE)
    if(texture && thrd->shareGroup.Ptr()){
        return thrd->shareGroup->isObject(TEXTURE,texture) ? GL_TRUE :GL_FALSE;
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsProgram(GLuint program){
    GET_CTX_RET(GL_FALSE)
    if(program && thrd->shareGroup.Ptr() &&
       thrd->shareGroup->isObject(SHADER,program)) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        return ctx->dispatcher().glIsProgram(globalProgramName);
    }
    return GL_FALSE;
}

GL_APICALL GLboolean    GL_APIENTRY glIsShader(GLuint shader){
    GET_CTX_RET(GL_FALSE)
    if(shader && thrd->shareGroup.Ptr() &&
       thrd->shareGroup->isObject(SHADER,shader)) {
        const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
        return ctx->dispatcher().glIsShader(globalShaderName);
    }
    return GL_FALSE;
}

GL_APICALL void  GL_APIENTRY glLineWidth(GLfloat width){
    GET_CTX();
    ctx->dispatcher().glLineWidth(width);
}

GL_APICALL void  GL_APIENTRY glLinkProgram(GLuint program){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glLinkProgram(globalProgramName);
    }
}

GL_APICALL void  GL_APIENTRY glPixelStorei(GLenum pname, GLint param){
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::pixelStoreParam(pname),GL_INVALID_ENUM);
    ctx->dispatcher().glPixelStorei(pname,param);
}

GL_APICALL void  GL_APIENTRY glPolygonOffset(GLfloat factor, GLfloat units){
    GET_CTX();
    ctx->dispatcher().glPolygonOffset(factor,units);
}

GL_APICALL void  GL_APIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::readPixelFrmt(format) && GLESv2Validate::pixelType(ctx,type)),GL_INVALID_ENUM);
    SET_ERROR_IF(!(GLESv2Validate::pixelOp(format,type)),GL_INVALID_OPERATION);
    ctx->dispatcher().glReadPixels(x,y,width,height,format,type,pixels);
}


GL_APICALL void  GL_APIENTRY glReleaseShaderCompiler(void){
    GET_CTX();
    ctx->dispatcher().glReleaseShaderCompiler();
}

GL_APICALL void  GL_APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height){
    GET_CTX();
    ctx->dispatcher().glRenderbufferStorage(target,internalformat,width,height);
}

GL_APICALL void  GL_APIENTRY glSampleCoverage(GLclampf value, GLboolean invert){
    GET_CTX();
    ctx->dispatcher().glSampleCoverage(value,invert);
}

GL_APICALL void  GL_APIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX();
    ctx->dispatcher().glScissor(x,y,width,height);
}

GL_APICALL void  GL_APIENTRY glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length){
    GET_CTX();
    if(thrd->shareGroup.Ptr()){
        for(int i=0; i < n ; i++){
            const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shaders[i]);
            ctx->dispatcher().glShaderBinary(1,&globalShaderName,binaryformat,binary,length);
        }
    }
}

GL_APICALL void  GL_APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar** string, const GLint* length){
    GET_CTX_V2();
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE);
    if(thrd->shareGroup.Ptr()){
            const GLuint globalShaderName = thrd->shareGroup->getGlobalName(SHADER,shader);
            SET_ERROR_IF(globalShaderName == 0,GL_INVALID_VALUE);
            ObjectDataPtr objData = thrd->shareGroup->getObjectData(SHADER,shader);
            SET_ERROR_IF(!objData.Ptr(),GL_INVALID_OPERATION);
            ShaderParser* sp = (ShaderParser*)objData.Ptr();
            sp->setSrc(ctx->glslVersion(),count,string,length);
            ctx->dispatcher().glShaderSource(globalShaderName,1,sp->parsedLines(),NULL);
    }
}

GL_APICALL void  GL_APIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask){
    GET_CTX();
    ctx->dispatcher().glStencilFunc(func,ref,mask);
}
GL_APICALL void  GL_APIENTRY glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask){
    GET_CTX();
    ctx->dispatcher().glStencilFuncSeparate(face,func,ref,mask);
}
GL_APICALL void  GL_APIENTRY glStencilMask(GLuint mask){
    GET_CTX();
    ctx->dispatcher().glStencilMask(mask);
}

GL_APICALL void  GL_APIENTRY glStencilMaskSeparate(GLenum face, GLuint mask){
    GET_CTX();
    ctx->dispatcher().glStencilMaskSeparate(face,mask);
}

GL_APICALL void  GL_APIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass){
    GET_CTX();
    ctx->dispatcher().glStencilOp(fail,zfail,zpass);
}

GL_APICALL void  GL_APIENTRY glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass){
    GET_CTX();
    ctx->dispatcher().glStencilOp(fail,zfail,zpass);
}

static TextureData* getTextureData(){
    GET_CTX_RET(NULL);
    unsigned int tex = ctx->getBindedTexture();
    TextureData *texData = NULL;
    ObjectDataPtr objData = thrd->shareGroup->getObjectData(TEXTURE,tex);
    if(!objData.Ptr()){
        texData = new TextureData();
        thrd->shareGroup->setObjectData(TEXTURE, tex, ObjectDataPtr(texData));
    } else {
        texData = (TextureData*)objData.Ptr();
    }
    return texData;
}

GL_APICALL void  GL_APIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTargetEx(target) &&
                   GLESv2Validate::pixelFrmt(ctx,internalformat) &&
                   GLESv2Validate::pixelFrmt(ctx,format)&&
                   GLESv2Validate::pixelType(ctx,type)),GL_INVALID_ENUM);

    SET_ERROR_IF(!(GLESv2Validate::pixelOp(format,type) && internalformat == ((GLint)format)),GL_INVALID_OPERATION);
    SET_ERROR_IF(border != 0,GL_INVALID_VALUE);

    if (thrd->shareGroup.Ptr()){
        unsigned int tex = ctx->getBindedTexture();
        TextureData *texData = getTextureData();
        if(texData) {
            texData->width = width;
            texData->height = height;
            texData->border = border;
            texData->internalFormat = internalformat;
        }
    }
    if (type==GL_HALF_FLOAT_OES)
        type = GL_HALF_FLOAT_NV;
    ctx->dispatcher().glTexImage2D(target,level,internalformat,width,height,border,format,type,pixels);
}


GL_APICALL void  GL_APIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameterf(target,pname,param);
}
GL_APICALL void  GL_APIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameterfv(target,pname,params);
}
GL_APICALL void  GL_APIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameteri(target,pname,param);
}
GL_APICALL void  GL_APIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint* params){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTarget(target) && GLESv2Validate::textureParams(pname)),GL_INVALID_ENUM);
    ctx->dispatcher().glTexParameteriv(target,pname,params);
}

GL_APICALL void  GL_APIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels){
    GET_CTX();
    SET_ERROR_IF(!(GLESv2Validate::textureTargetEx(target) &&
                   GLESv2Validate::pixelFrmt(ctx,format)&&
                   GLESv2Validate::pixelType(ctx,type)),GL_INVALID_ENUM);
    SET_ERROR_IF(!GLESv2Validate::pixelOp(format,type),GL_INVALID_OPERATION);
    if (type==GL_HALF_FLOAT_OES)
        type = GL_HALF_FLOAT_NV;

    ctx->dispatcher().glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,pixels);

}

GL_APICALL void  GL_APIENTRY glUniform1f(GLint location, GLfloat x){
    GET_CTX();
    ctx->dispatcher().glUniform1f(location,x);
}
GL_APICALL void  GL_APIENTRY glUniform1fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform1fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform1i(GLint location, GLint x){
    GET_CTX();
    ctx->dispatcher().glUniform1i(location,x);
}
GL_APICALL void  GL_APIENTRY glUniform1iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform1iv(location,count,v);
}
GL_APICALL void  GL_APIENTRY glUniform2f(GLint location, GLfloat x, GLfloat y){
    GET_CTX();
    ctx->dispatcher().glUniform2f(location,x,y);
}
GL_APICALL void  GL_APIENTRY glUniform2fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform2fv(location,count,v);
}
GL_APICALL void  GL_APIENTRY glUniform2i(GLint location, GLint x, GLint y){
    GET_CTX();
    ctx->dispatcher().glUniform2i(location,x,y);
}
GL_APICALL void  GL_APIENTRY glUniform2iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform2iv(location,count,v);
}
GL_APICALL void  GL_APIENTRY glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z){
    GET_CTX();
    ctx->dispatcher().glUniform3f(location,x,y,z);
}
GL_APICALL void  GL_APIENTRY glUniform3fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform3fv(location,count,v);
}
GL_APICALL void  GL_APIENTRY glUniform3i(GLint location, GLint x, GLint y, GLint z){
    GET_CTX();
    ctx->dispatcher().glUniform3i(location,x,y,z);
}

GL_APICALL void  GL_APIENTRY glUniform3iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform3iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w){
    GET_CTX();
    ctx->dispatcher().glUniform4f(location,x,y,z,w);
}

GL_APICALL void  GL_APIENTRY glUniform4fv(GLint location, GLsizei count, const GLfloat* v){
    GET_CTX();
    ctx->dispatcher().glUniform4fv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w){
    GET_CTX();
    ctx->dispatcher().glUniform4i(location,x,y,z,w);
}

GL_APICALL void  GL_APIENTRY glUniform4iv(GLint location, GLsizei count, const GLint* v){
    GET_CTX();
    ctx->dispatcher().glUniform4iv(location,count,v);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX();
    SET_ERROR_IF(transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix2fv(location,count,transpose,value);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX();
    SET_ERROR_IF(transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix3fv(location,count,transpose,value);
}

GL_APICALL void  GL_APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value){
    GET_CTX();
    SET_ERROR_IF(transpose != GL_FALSE,GL_INVALID_VALUE);
    ctx->dispatcher().glUniformMatrix4fv(location,count,transpose,value);
}

GL_APICALL void  GL_APIENTRY glUseProgram(GLuint program){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glUseProgram(globalProgramName);
    }
}

GL_APICALL void  GL_APIENTRY glValidateProgram(GLuint program){
    GET_CTX();
    if(thrd->shareGroup.Ptr()) {
        const GLuint globalProgramName = thrd->shareGroup->getGlobalName(SHADER,program);
        ctx->dispatcher().glValidateProgram(globalProgramName);
    }
}

GL_APICALL void  GL_APIENTRY glVertexAttrib1f(GLuint indx, GLfloat x){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib1f(indx,x);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib1fv(GLuint indx, const GLfloat* values){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib1fv(indx,values);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib2f(indx,x,y);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib2fv(GLuint indx, const GLfloat* values){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib2fv(indx,values);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib3f(indx,x,y,z);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib3fv(GLuint indx, const GLfloat* values){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib3fv(indx,values);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib4f(indx,x,y,z,w);
}

GL_APICALL void  GL_APIENTRY glVertexAttrib4fv(GLuint indx, const GLfloat* values){
    GET_CTX();
    ctx->dispatcher().glVertexAttrib4fv(indx,values);
}

GL_APICALL void  GL_APIENTRY glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr){
    GET_CTX();
    if (type==GL_HALF_FLOAT_OES)
        type = GL_HALF_FLOAT;
    const GLvoid* data = ctx->setPointer(indx,size,type,stride,ptr);
    if(type != GL_FIXED) ctx->dispatcher().glVertexAttribPointer(indx,size,type,normalized,stride,ptr);
}

GL_APICALL void  GL_APIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height){
    GET_CTX();
    ctx->dispatcher().glViewport(x,y,width,height);
}

GL_APICALL void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    GET_CTX();
    SET_ERROR_IF(!GLESv2Validate::textureTargetLimited(target),GL_INVALID_ENUM);
    EglImage *img = s_eglIface->eglAttachEGLImage((unsigned int)image);
    if (img) {
        // Create the texture object in the underlying EGL implementation,
        // flag to the OpenGL layer to skip the image creation and map the
        // current binded texture object to the existing global object.
        if (thrd->shareGroup.Ptr()) {
            unsigned int tex = ctx->getBindedTexture();
            unsigned int oldGlobal = thrd->shareGroup->getGlobalName(TEXTURE, tex);
            // Delete old texture object
            if (oldGlobal) {
                ctx->dispatcher().glDeleteTextures(1, &oldGlobal);
            }
            // replace mapping and bind the new global object
            thrd->shareGroup->replaceGlobalName(TEXTURE, tex,img->globalTexName);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, img->globalTexName);
            TextureData *texData = getTextureData();
            SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
            texData->sourceEGLImage = (unsigned int)image;
            texData->eglImageDetach = s_eglIface->eglDetachEGLImage;
        }
    }
}

GL_APICALL void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
    GET_CTX()
    //not supported by EGL
    SET_ERROR_IF(false,GL_INVALID_OPERATION);
}
