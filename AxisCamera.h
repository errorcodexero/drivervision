/*
 * Copyright 2009-2010 Patrick Fairbank. All Rights Reserved.
 * See LICENSE.TXT for licensing information.
 *
 * Class representing the Axis Camera.
 */

#ifndef _AXIS_CAMERA_H_
#define _AXIS_CAMERA_H_

#include <winsock2.h>
// #include <windows.h>
#include "nivision.h"
// #include <nimachinevision.h>

class AxisCamera {
public:
    AxisCamera( LPCTSTR ipaddr = TEXT("10.14.25.11") );
    ~AxisCamera();

    bool IsFreshImage();
    int GetImage( Image* img );

private:
    static DWORD WINAPI StartCamera(LPVOID param);
    void ConfigureCamera();
    void Run();
    int ReadBytes(int offset);

    void AllocError();
    void SocketError();
    void SocketEOF();
    void StreamError();
    void VisionError();

    LPTSTR m_ipaddr;
    HANDLE m_thread;
    HANDLE m_mutex;
    SOCKET m_cameraSocket;
    char*  m_cameraBuffer;
    int    m_bufferSize;
    Image* m_image;
    bool   m_freshImage;
};

#endif // _AXIS_CAMERA_H_
