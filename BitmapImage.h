/*
 * Copyright 2009-2010 Patrick Fairbank. All Rights Reserved.
 * See LICENSE.TXT for licensing information.
 *
 * Class representing an image from the camera to be displayed in the application window.
 */

#ifndef _BITMAP_IMAGE_H_
#define _BITMAP_IMAGE_H_

#include "nivision.h"
#include <Windows.h>

class BitmapImage {
public:
  BitmapImage(Image* image);
  ~BitmapImage();
  HBITMAP GetBitmap();

private:
  HBITMAP bitmap_;
};

#endif // _BITMAP_IMAGE_H_
