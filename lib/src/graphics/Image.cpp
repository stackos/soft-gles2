/*
* Viry3D
* Copyright 2014-2018 by Stack - stackos@qq.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Image.h"
#include "io/File.h"
#include "memory/Memory.h"

extern "C"
{
#include "png/png.h"
#include "png/pngstruct.h"
}

namespace Viry3D
{
	struct PNGData
	{
		char *buffer;
		int size;
	};

	static void user_png_read(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		memcpy(data, png_ptr->io_ptr, length);
		png_ptr->io_ptr = (char *) png_ptr->io_ptr + length;
	}

	static void user_png_write(png_structp png_ptr, png_bytep data, png_size_t length)
	{
		PNGData *png = (PNGData *) png_get_io_ptr(png_ptr);

		if (png->buffer == 0)
		{
			png->buffer = (char *) malloc(length);
		}
		else
		{
			png->buffer = (char *) realloc(png->buffer, png->size + length);
		}

		memcpy(&png->buffer[png->size], data, length);
		png->size += (int) length;
	}

	static void user_png_flush(png_structp png_ptr)
	{
	}

	ByteBuffer Image::LoadPNG(const ByteBuffer& png, int& width, int& height, int& bpp)
	{
		ByteBuffer colors;

		png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		png_infop info_ptr = png_create_info_struct(png_ptr);
		setjmp(png_jmpbuf(png_ptr));

		png_set_read_fn(png_ptr, png.Bytes(), user_png_read);
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

		width = png_get_image_width(png_ptr, info_ptr);
		height = png_get_image_height(png_ptr, info_ptr);

		int color_type = png_get_color_type(png_ptr, info_ptr);
		if (color_type == PNG_COLOR_TYPE_RGBA)
		{
			bpp = png_get_bit_depth(png_ptr, info_ptr) * 4;

			png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

			colors = ByteBuffer(width * height * 4);

			unsigned char *pPixel = colors.Bytes();

			for (int i = 0; i < height; i++)
			{
				memcpy(pPixel, row_pointers[i], width * 4);
				pPixel += width * 4;
			}
		}
		else if (color_type == PNG_COLOR_TYPE_RGB)
		{
			bpp = png_get_bit_depth(png_ptr, info_ptr) * 3;

			png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

			colors = ByteBuffer(width * height * 3);

			unsigned char *pPixel = colors.Bytes();

			for (int i = 0; i < height; i++)
			{
				memcpy(pPixel, row_pointers[i], width * 3);
				pPixel += width * 3;
			}
		}
		else if (color_type == PNG_COLOR_TYPE_GRAY)
		{
			bpp = png_get_bit_depth(png_ptr, info_ptr) * 1;

			png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

			colors = ByteBuffer(width * height);

			unsigned char *pPixel = colors.Bytes();

			for (int i = 0; i < height; i++)
			{
				memcpy(pPixel, row_pointers[i], width * 1);
				pPixel += width * 1;
			}
		}
		else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		{
			bpp = png_get_bit_depth(png_ptr, info_ptr) * 4;

			png_bytep* row_pointers = png_get_rows(png_ptr, info_ptr);

			colors = ByteBuffer(width * height * 4);

			byte* pPixel = colors.Bytes();

			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					png_byte g = row_pointers[i][j * 2];
					png_byte a = row_pointers[i][j * 2 + 1];
					pPixel[0] = g;
					pPixel[1] = g;
					pPixel[2] = g;
					pPixel[3] = a;

					pPixel += 4;
				}
			}
		}

		png_destroy_read_struct(&png_ptr, &info_ptr, 0);

		return colors;
	}
}
