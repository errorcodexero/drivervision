/*
 * Copyright 2009-2010 Patrick Fairbank. All Rights Reserved.
 * See LICENSE.TXT for licensing information.
 *
 * Class representing an image from the camera to be displayed in the application window.
 */

#include "stdafx.h"
#include "BitmapImage.h"

/*
 * Creates an HBITMAP object from the given NIVision Image object.
 */
BitmapImage::BitmapImage(Image* image) {
  bitmap_ = NULL;

  // Make sure the image is RGB for commonality.
  imaqCast(NULL, image, IMAQ_IMAGE_RGB, NULL, 0);

  ImageInfo info;
  imaqGetImageInfo(image, &info);
  BYTE* pixels = (BYTE*)info.imageStart;

  BITMAPINFOHEADER bmih;
  bmih.biSize = sizeof(BITMAPINFOHEADER);
  bmih.biWidth = info.xRes;
  bmih.biHeight = -info.yRes;
  bmih.biPlanes = 1;
  bmih.biBitCount = 24;
  bmih.biCompression = BI_RGB;
  bmih.biSizeImage = 0;
  bmih.biXPelsPerMeter = 0;
  bmih.biYPelsPerMeter = 0;
  bmih.biClrUsed = 0;
  bmih.biClrImportant = 0;

  // Create an HBITMAP with a writeable pixel data area.
  BYTE* bitmapData;
  bitmap_ = CreateDIBSection(NULL, (BITMAPINFO*)&bmih, 0, (void**)&bitmapData, NULL, 0);

  // Manually copy each pixel from the Image to the bitmap.
  for (int y = 0; y < info.yRes; y++) {
    for (int x = 0; x < info.xRes; x++) {
      int imageIndex = 3 * info.xRes * y + 3 * x;
      int bitmapIndex = 4 * info.pixelsPerLine * y + 4 * x;

      // Order of pixels is blue, green, red.
      bitmapData[imageIndex] = pixels[bitmapIndex];
      bitmapData[imageIndex + 1] = pixels[bitmapIndex + 1];
      bitmapData[imageIndex + 2] = pixels[bitmapIndex + 2];
    }
  }
}

BitmapImage::~BitmapImage() {
  if (bitmap_) {
    DeleteObject(bitmap_);
  }
}

HBITMAP BitmapImage::GetBitmap() {
  return bitmap_;
}
