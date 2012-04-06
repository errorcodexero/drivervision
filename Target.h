// sample robot code
// Steve Tarr - team 1425 mentor - 11-Feb-2012

#ifndef _TARGET_H_
#define _TARGET_H_

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "nivision.h"
// #include "nimachinevision.h"
#include <windows.h>
#include "RGBImage.h"
#include "MonoImage.h"

class Target
{
public:
    Target();
    ~Target();

    typedef enum { kCenter, kTop, kBottom, kLeft, kRight } TargetID;

    struct TargetLocation {
	TargetID id;		// which target
	int height;		// 0 = bottom, 1 = middle, 2 = top
	double angle;		// in degrees, positive is to the right
	double distance;	// in inches
	bool valid;		// is the position data valid for targeting?
    };

    bool ProcessImage( Image* image, float threshold );
    void DisplayImage();
    TargetLocation GetTargetLocation( TargetID which );
	bool GetProcessedImage(Image* image);

    struct Particle {
	int index;
	double size;
	double xCenter;
	double yCenter;
	double leftBound;
	double rightBound;
	double topBound;
	double bottomBound;
	double height;
	double width;
    };

private:

    ////////////////////////////////////////////////////
    // Target locations returned to the main thread.
    // Accesses must be locked with the mutex.
    ////////////////////////////////////////////////////

    TargetLocation m_targetCenter;
    TargetLocation m_targetTop;
    TargetLocation m_targetBottom;
    TargetLocation m_targetLeft;
    TargetLocation m_targetRight;

    ///////////////////////////////////////////////////
    // The following variables are used by the
    // processing task and should not be accessed
    // from the main thread.
    ///////////////////////////////////////////////////

    // processing constants
    RGBValue m_falseColor[256];

    // images
#if 0
	RGBImage m_cameraImage;
#else
	MonoImage m_cameraImage;
#endif
    MonoImage m_monoImage;
    MonoImage m_threshold;
    MonoImage m_convexHull;
    MonoImage m_filtered;
    RGBImage m_overlay;  // for debugging

    // particles (regions of interest)
    Particle m_particles[4];

    int m_numParticles;
    Particle *m_pTop, *m_pBottom, *m_pLeft, *m_pRight;
    bool m_topClipped, m_bottomClipped, m_leftClipped, m_rightClipped;

    // target locations
    double m_centerX;
    double m_centerY;
    double m_topAngle;
    double m_bottomAngle;
    double m_centerAngle;
    double m_centerDistance;
    double m_leftX;
    double m_leftAngle;
    double m_leftDistance;
    double m_rightX;
    double m_rightAngle;
    double m_rightDistance;
 
    // initialization
    void TargetInit();

    // targeting calculations
#if 0
    bool GetImage();
#endif
    bool FindParticles( float threshold );
    bool AnalyzeParticles();
    void SaveImages();
};

#endif // _TARGET_H_
