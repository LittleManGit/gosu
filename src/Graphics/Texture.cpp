#include <Gosu/Graphics.hpp>
#include "Texture.hpp"
#include "TexChunk.hpp"
#include <Gosu/Bitmap.hpp>
#include <Gosu/Platform.hpp>
#include <stdexcept>

namespace Gosu
{
    bool undocumentedRetrofication = false;
}

Gosu::Texture::Texture(unsigned size, bool retro)
: allocator_(size, size), retro_(retro)
{
    ensureCurrentContext();
    
    // Create texture name.
    glGenTextures(1, &texName_);
    if (texName_ == static_cast<GLuint>(-1))
        throw std::runtime_error("Couldn't create OpenGL texture");
   
    // Create empty texture.
    glBindTexture(GL_TEXTURE_2D, texName_);
#ifdef GOSU_IS_OPENGLES
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, allocator_.width(), allocator_.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
#else
    glTexImage2D(GL_TEXTURE_2D, 0, 4, allocator_.width(), allocator_.height(), 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, 0);
#endif
    
    if (retro || undocumentedRetrofication)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    
#ifdef GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
}

Gosu::Texture::~Texture()
{
    ensureCurrentContext();
    
    glDeleteTextures(1, &texName_);
}

unsigned Gosu::Texture::size() const
{
    return allocator_.width(); // == height
}

GLuint Gosu::Texture::texName() const
{
    return texName_;
}

bool Gosu::Texture::retro() const
{
    return retro_;
}

GOSU_UNIQUE_PTR<Gosu::TexChunk>
    Gosu::Texture::tryAlloc(std::tr1::shared_ptr<Texture> ptr, const Bitmap& bmp, unsigned padding)
{
    GOSU_UNIQUE_PTR<Gosu::TexChunk> result;
    
    BlockAllocator::Block block;
    if (!allocator_.alloc(bmp.width(), bmp.height(), block))
        return result;
    
    result.reset(new TexChunk(ptr, block.left + padding, block.top + padding,
                              block.width - 2 * padding, block.height - 2 * padding, padding));
    
    ensureCurrentContext();
    
    glBindTexture(GL_TEXTURE_2D, texName_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, block.left, block.top, block.width, block.height,
                 Color::GL_FORMAT, GL_UNSIGNED_BYTE, bmp.data());

    return GOSU_MOVE_UNIQUE_PTR(result);
}

void Gosu::Texture::block(unsigned x, unsigned y, unsigned width, unsigned height)
{
    allocator_.block(x, y, width, height);
}

void Gosu::Texture::free(unsigned x, unsigned y, unsigned width, unsigned height)
{
    allocator_.free(x, y, width, height);
}

Gosu::Bitmap Gosu::Texture::toBitmap(unsigned x, unsigned y, unsigned width, unsigned height) const
{
#ifdef GOSU_IS_OPENGLES
    throw std::logic_error("Texture::toBitmap not supported on iOS");
#else
    ensureCurrentContext();
    
    Gosu::Bitmap fullTexture(size(), size());
    glBindTexture(GL_TEXTURE_2D, texName());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, fullTexture.data());
    Gosu::Bitmap bitmap(width, height);
    bitmap.insert(fullTexture, -int(x), -int(y));
    
    return bitmap;
#endif
}
