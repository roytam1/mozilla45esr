//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferImpl_mock.h:
//   Defines a mock of the FramebufferImpl class.
//

#ifndef LIBANGLE_RENDERER_FRAMEBUFFERIMPLMOCK_H_
#define LIBANGLE_RENDERER_FRAMEBUFFERIMPLMOCK_H_

#include "gmock/gmock.h"

#include "libANGLE/renderer/FramebufferImpl.h"

namespace rx
{

class MockFramebufferImpl : public rx::FramebufferImpl
{
  public:
    MockFramebufferImpl() : rx::FramebufferImpl(gl::Framebuffer::Data()) {}
    virtual ~MockFramebufferImpl() { destroy(); }

    MOCK_METHOD1(onUpdateColorAttachment, void(size_t));
    MOCK_METHOD0(onUpdateDepthAttachment, void());
    MOCK_METHOD0(onUpdateStencilAttachment, void());
    MOCK_METHOD0(onUpdateDepthStencilAttachment, void());

    MOCK_METHOD2(setDrawBuffers, void(size_t, const GLenum *));
    MOCK_METHOD1(setReadBuffer, void(GLenum));

    MOCK_METHOD2(discard, gl::Error(size_t, const GLenum *));
    MOCK_METHOD2(invalidate, gl::Error(size_t, const GLenum *));
    MOCK_METHOD3(invalidateSub, gl::Error(size_t, const GLenum *, const gl::Rectangle &));

    MOCK_METHOD2(clear, gl::Error(const gl::Data &, GLbitfield));
    MOCK_METHOD4(clearBufferfv, gl::Error(const gl::State &, GLenum, GLint, const GLfloat *));
    MOCK_METHOD4(clearBufferuiv, gl::Error(const gl::State &, GLenum, GLint, const GLuint *));
    MOCK_METHOD4(clearBufferiv, gl::Error(const gl::State &, GLenum, GLint, const GLint *));
    MOCK_METHOD5(clearBufferfi, gl::Error(const gl::State &, GLenum, GLint, GLfloat, GLint));

    MOCK_CONST_METHOD0(getImplementationColorReadFormat, GLenum());
    MOCK_CONST_METHOD0(getImplementationColorReadType, GLenum());
    MOCK_CONST_METHOD5(
        readPixels,
        gl::Error(const gl::State &, const gl::Rectangle &, GLenum, GLenum, GLvoid *));

    MOCK_METHOD6(blit,
                 gl::Error(const gl::State &,
                           const gl::Rectangle &,
                           const gl::Rectangle &,
                           GLbitfield,
                           GLenum,
                           const gl::Framebuffer *));

    MOCK_CONST_METHOD0(checkStatus, GLenum());

    MOCK_METHOD0(destroy, void());
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_FRAMEBUFFERIMPLMOCK_H_
