/*
 * Win32 interface to the Axis camera
 */

#include "stdafx.h"
#include "AxisCamera.h"
#include "jpeg_decoder.h"

#define	CAMERA_PORT	80

// Camera white balance (auto, fixed_fluor2, fixed_indoor, fixed_outdoor1, fixed_outdoor2,
// fixed_fluor1, fixed_fluor2, or hold).
#define CAMERA_WHITE_BALANCE "fixed_fluor2"

// Camera exposure level (auto, flickerfree50, flickerfree60, hold).
#define CAMERA_EXPOSURE "flickerfree60"

// Camera exposure priority (0 - prioritize quality, 50 - none, 100 - prioritize framerate).
#define CAMERA_EXPOSURE_PRIORITY 0

#define CAMERA_BRIGHTNESS 50
#define CAMERA_COLOR_LEVEL 50

#define CAMERA_FRAMES_PER_SECOND 6
#define CAMERA_COMPRESSION 60
#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480
#define CAMERA_ROTATION 0
#define CAMERA_AUTHENTICATION "RlJDOkZSQw==" // Username 'FRC', password 'FRC'.

#define	BUFFER_INCREMENT (50*1024)

// fatal error handlers
void AxisCamera::Win32Error()
{
    printf("AxisCamera: Win32 error %d\n", GetLastError());
    ExitProcess(1);
}

void AxisCamera::AllocError()
{
    printf("AxisCamera: memory allocation failed\n");
    ExitProcess(1);
}

void AxisCamera::SocketError()
{
    int error = WSAGetLastError();
    printf("AxisCamera: winsock error %d\n", error);
    ExitProcess(1);
}

void AxisCamera::SocketEOF()
{
    printf("AxisCamera: camera closed connection\n");
    ExitProcess(1);
}

void AxisCamera::StreamError()
{
    printf("AxisCamera: stream format error - missing Content-Length\n");
    ExitProcess(1);
}

void AxisCamera::VisionError()
{
    printf("AxisCamera: Vision error - %s", imaqGetErrorText(imaqGetLastError()));
    ExitProcess(1);
}

// Constructor
AxisCamera::AxisCamera( LPCTSTR ipaddr )
{
//  printf("AxisCamera constructor at %p\n", this);

    m_ipaddr = _tcsdup(ipaddr);

    // Create an NIVision Image object to represent the new frame.
    m_image = imaqCreateImage(IMAQ_IMAGE_RGB, 3);
    if (m_image == NULL) {
	VisionError();
    }
    if (!imaqSetImageSize(m_image, CAMERA_WIDTH, CAMERA_HEIGHT)) {
	VisionError();
    }

    m_freshImage = false;

    // Create an event object for synchronization with our caller.
    m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_event == NULL) {
	Win32Error();
    }

    // Create a mutex lock to control access to shared data.
    // TBD: check for error here
    m_mutex = CreateMutex(NULL, FALSE, NULL);
    if (m_event == NULL) {
	Win32Error();
    }

    // Start the image processing thread.
    // TBD: check for error here, too
    m_thread = CreateThread(NULL, 0, AxisCamera::StartCamera, this, 0, NULL);
    if (m_thread == NULL) {
	Win32Error();
    }
}

// Destructor
AxisCamera::~AxisCamera()
{
    // Avoid terminating the thread while it's writing shared objects.
    WaitForSingleObject(m_mutex, INFINITE);
    // Kill the thread.
    TerminateThread(m_thread, 0);
    CloseHandle(m_thread);
    // Release resources.
    closesocket(m_cameraSocket);
    WSACleanup();
    free(m_cameraBuffer);
    imaqDispose(m_image);
    CloseHandle(m_mutex);
    CloseHandle(m_event);
    free(m_ipaddr);
}

#if 0
void AxisCamera::ConfigureCamera()
{
    // Create a TCP socket to the camera and connect to it.
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
	SocketError();
    }

    SOCKADDR_IN sockAddr;
    int sockLen = sizeof sockAddr;
    WSAStringToAddress(m_ipaddr, AF_INET, NULL, (LPSOCKADDR) &sockAddr, &sockLen);
    sockAddr.sin_port = htons(CAMERA_PORT);

    if (connect(s, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
	SocketError();
    }

    // Populate the settings string with user parameters defined in Constants.h
    char settingsString[512];
    sprintf_s(settingsString,
	    sizeof settingsString,
	    "GET /axis-cgi/admin/param.cgi?action=update"
		"&ImageSource.I0.Sensor.WhiteBalance=%s"
		"&ImageSource.I0.Sensor.Exposure=%s"
		"&ImageSource.I0.Sensor.ExposurePriority=%d"
		"&ImageSource.I0.Sensor.Brightness=%d"
		"&ImageSource.I0.Sensor.ColorLevel=%d"
		" HTTP/1.1\r\n"
		"User-Agent: DriverVision\r\n"
		"Accept: */*\r\n"
		"Connection: Close\r\n",
		"Authorization: Basic %s;\r\n\r\n",
	    CAMERA_WHITE_BALANCE,
	    CAMERA_EXPOSURE,
	    CAMERA_EXPOSURE_PRIORITY,
	    CAMERA_BRIGHTNESS,
	    CAMERA_COLOR_LEVEL,
	    CAMERA_AUTHENTICATION);

    // Send the settings string to the camera to ensure desired video settings are selected.
    if (send(s, settingsString, (int)strlen(settingsString), 0) == SOCKET_ERROR) {
	SocketError();
    }

    // Discard the response from the camera.
    char replyBuffer[512];
    int status;
    while ((status = recv(s, replyBuffer, 512, 0)) != 0) {
	if (status == SOCKET_ERROR) {
	    SocketError();
	}
    }

    // Reset the socket for another operation since the camera closed it on the other end.
    closesocket(s);
}
#endif

int AxisCamera::ReadBytes( int offset )
{
//  printf("AxisCamera::ReadBytes this %p m_bufferSize %d offset %d\n", this, m_bufferSize, offset);
    if (m_bufferSize < offset + BUFFER_INCREMENT) {
	m_bufferSize += BUFFER_INCREMENT;
	m_cameraBuffer = (char *) realloc((void *)m_cameraBuffer, m_bufferSize);
	if (!m_cameraBuffer) {
	    AllocError();
	}
    }
    int n = recv(m_cameraSocket, m_cameraBuffer + offset, m_bufferSize - offset, 0);
    if (n == SOCKET_ERROR) {
	SocketError();
    }
    return n;
}

/*
 * Entry point for the camera thread.
 */
DWORD WINAPI AxisCamera::StartCamera(LPVOID param)
{
    AxisCamera* camera = (AxisCamera*)param;
    camera->Run();
    return 0;
}

void AxisCamera::Run()
{
//  printf("camera thread %p starting...\n", this);

    // Initialize Winsock library.
    WSADATA WsaDat;
    if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != NO_ERROR) {
	SocketError();
    }

#if 0
    // Configure the camera's default image stream.
    ConfigureCamera();
#endif

    // Open a connection to the camera.
    SOCKADDR_IN sockAddr;
    int sockLen = sizeof sockAddr;
    WSAStringToAddress(m_ipaddr, AF_INET, NULL, (LPSOCKADDR) &sockAddr, &sockLen);
    sockAddr.sin_port = htons(CAMERA_PORT);

    m_cameraSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connect(m_cameraSocket, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) == SOCKET_ERROR) {
	SocketError();
    }

//  printf("camera thread %p connected\n", this);

    // Populate the request string with user parameters defined in Constants.h
    char requestString[256];
    sprintf_s(requestString,
	    sizeof requestString,
#if 1
	    // TrendNet IP-110
	    "GET /cgi/mjpg/mjpeg.cgi"
		" HTTP/1.1\r\n"
		"User-Agent: DriverVision\r\n"
		"Accept: */*\r\n"
		"Connection: Keep-alive\r\n"
		"DNT: 1\r\n"
		"Authorization: Basic %s\r\n\r\n",
	    CAMERA_AUTHENTICATION);
#else
	    // Axis M206, M1011
	    "GET /axis-cgi/mjpg/video.cgi"
		"?des_fps=%i&compression=%i&resolution=%ix%i&rotation=%i&color=1&colorlevel=%i"
		" HTTP/1.1\r\n"
		"User-Agent: DriverVision\r\n"
		"Accept: */*\r\n"
		"Connection: Keep-alive\r\n"
		"DNT: 1\r\n"
		"Authorization: Basic %s\r\n\r\n",
	    CAMERA_FRAMES_PER_SECOND,
	    CAMERA_COMPRESSION,
	    CAMERA_WIDTH, CAMERA_HEIGHT,
	    CAMERA_ROTATION,
	    CAMERA_COLOR_LEVEL,
	    CAMERA_AUTHENTICATION);
#endif

    // Send the request string to the camera, prompting a continuous motion JPEG stream in reply.
    if (send(m_cameraSocket, requestString, (int)strlen(requestString), 0) == SOCKET_ERROR) {
	SocketError();
    }

    // Create a buffer for storing frames acquired from the camera.
    // Start with one chunk, grow as needed.
    m_bufferSize = BUFFER_INCREMENT;
    m_cameraBuffer = (char *) malloc(m_bufferSize);
    if (!m_cameraBuffer) {
	AllocError();
    }

//  printf("camera thread %p reading...\n", this);

    // Read the stream of frames from the camera.
    int len = 0;
    for (;;)
    {
	// Each frame starts with HTTP headers ending with CRLFCRLF.
	// Read from the stream until all the headers (and perhaps some
	// following data) are in memory.
	int offset = 0;
	int found = 0;

	while (!found) {
//	    printf("camera %p offset %d len %d\n", this, offset, len);
	    while (len < offset + 4) {
		int n = ReadBytes(len);
//		printf("camera %p offset %d len %d read %d\n", this, offset, len, n);
		if (n == 0) {
		    SocketEOF();
		}
		len += n;
	    }
	    while (offset + 4 <= len) {
		if (memcmp(m_cameraBuffer + offset, "\r\n\r\n", 4) == 0) {
		    found = 1;
		    break;
		}
		offset++;
	    }
	}

//	printf("camera %p found boundary at offset %d\n", this, offset);

	// replace the second CRLF with a NUL so we can use e.g. strstr safely.
	m_cameraBuffer[offset+2] = '\0';

	// leave 'offset' pointing to data following headers
	offset += 4;

        // Determine the size in bytes of the current JPEG image.
        char* contentPtr = strstr(m_cameraBuffer, "Content-Length: ");
        if (contentPtr) {
	    int contentLength = atol(contentPtr + 16);

//	    printf("camera %p content-length %d\n", this, contentLength);

	    // Continue reading until the entire image is in memory.
	    while (len < offset + contentLength) {
		int n = ReadBytes(len);
		if (n == 0) {
		    SocketEOF();
		}
		len += n;
	    }

//	    printf("camera %p wait for mutex\n", this);

	    // Synchronize with the main thread before writing the image.
	    WaitForSingleObject(m_mutex, INFINITE);
	    {
		// strip any non-JPEG header
		while (contentLength > 0 && m_cameraBuffer[offset] != (char)0xFF) {
		    offset++;
		    contentLength--;
		}

		// Convert JPEG stream to bitmap.
		Jpeg::Decoder *jpg = new Jpeg::Decoder(
			(unsigned char *) m_cameraBuffer + offset,
			contentLength );

		// The image has been consumed.
		offset += contentLength;

		// Check conversion results.
		int result = jpg->GetResult();
		if (result != 0) {
		    printf("camera %p JPEG decode failed: %d\n", this, result);
		} else {
//		    printf("camera %p JPEG image: width %d height %d color %d size %u\n",
//			    this, jpg->GetWidth(), jpg->GetHeight(), jpg->IsColor(), jpg->GetImageSize());

		    // Convert to IMAQ Image
		    if (imaqArrayToImage(m_image, jpg->GetImage(),
		    	jpg->GetWidth(), jpg->GetHeight()))
		    {
			// Let the caller know a new image is available.
			m_freshImage = true;
			if (!SetEvent(m_event)) {
			    Win32Error();
			}
		    }
		    else
		    {
			printf("camera %p imaqArrayToImage failed: %s\n", this,
				imaqGetErrorText(imaqGetLastError()));
		    }
		    delete jpg;
		}
	    }
	    // Release the image to the main task.
	    ReleaseMutex(m_mutex);
	} else {
	    printf("camera %p no Content-length\n", this);
	}
	// Discard the current headers and image data.
	// Move any remaining data to the beginning of the buffer for the next pass.
	len -= offset;
	if (len > 0) {
	    memmove(m_cameraBuffer, m_cameraBuffer + offset, len);
	}
    }
    // no return
}

bool AxisCamera::IsFreshImage()
{
    // This assumes that read/write of a bool is atomic.
    return m_freshImage;
}

int AxisCamera::GetImage( Image *img )
{
//  printf("camera %p GetImage waiting\n", this);

    // Wait for the event signaling that a new frame is available.
    if (WaitForSingleObject(m_event, INFINITE) != 0) {
	Win32Error();
	return 0;
    }

    // Take the mutex that protects shared data.
    if (WaitForSingleObject(m_mutex, INFINITE) != 0) {
	Win32Error();
	return 0;
    }

    // Copy the image to the caller's buffer.
    int result = imaqDuplicate(img, m_image);

    // Reset the "new image available" flag and return the new image.
    m_freshImage = false;
    ReleaseMutex(m_mutex);

//  printf("camera %p return %d\n", this, result);
    return result;
}

