#include "gfx_api.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include "lib/framework/frame.h"

static GLenum to_gl(const gfx_api::pixel_format& format)
{
	switch (format)
	{
	case gfx_api::pixel_format::rgba:
		return GL_RGBA;
	case gfx_api::pixel_format::rgb:
		return GL_RGB;
	case gfx_api::pixel_format::compressed_rgb:
		return GL_COMPRESSED_RGB_ARB;
	case gfx_api::pixel_format::compressed_rgba:
		return GL_COMPRESSED_RGBA_ARB;
	default:
		debug(LOG_FATAL, "Unrecognised pixel format");
	}
	return GL_INVALID_ENUM;
}

struct gl_texture : public gfx_api::texture
{
private:
	friend struct gl_context;
	GLuint _id;
	const gfx_api::pixel_format _internal_format;

	gl_texture(const gfx_api::pixel_format & internal_format)
		: _internal_format(internal_format)
	{
		glGenTextures(1, &_id);
	}

	~gl_texture()
	{
		glDeleteTextures(1, &_id);
	}
public:
	virtual void bind() override
	{
		glBindTexture(GL_TEXTURE_2D, _id);
	}

	virtual void upload(const size_t& mip_level, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override
	{
		bind();
		glTexImage2D(GL_TEXTURE_2D, mip_level, to_gl(_internal_format), width, height, 0, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
	}

	virtual void update_subrect(const size_t& mip_level, const size_t& offset_x, const size_t& offset_y, const size_t & width, const size_t & height, const gfx_api::pixel_format & buffer_format, const void * data) override
	{
		bind();
		glTexSubImage2D(GL_TEXTURE_2D, mip_level, offset_x, offset_y, width, height, to_gl(buffer_format), GL_UNSIGNED_BYTE, data);
	}

	virtual unsigned id() override
	{
		return _id;
	}

	virtual void generate_mip_levels() override
	{
		glGenerateMipmap(GL_TEXTURE_2D);
	}

};

struct gl_context : public gfx_api::context
{
	virtual gfx_api::texture* create_texture(const size_t & width, const size_t & height, const gfx_api::pixel_format & internal_format, const std::string& filename) override
	{
		auto* new_texture = new gl_texture(internal_format);
		new_texture->bind();
		if (!filename.empty() && (GLEW_VERSION_4_3 || GLEW_KHR_debug))
		{
			glObjectLabel(GL_TEXTURE, new_texture->id(), -1, filename.c_str());
		}
		return new_texture;
	}
};

gfx_api::context& gfx_api::context::get()
{
	static gl_context ctx;
	return ctx;
}
