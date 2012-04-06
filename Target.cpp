// sample robot code
// Steve Tarr - team 1425 mentor - 11-Feb-2012

#include "stdafx.h"
#include "Target.h"

Target::Target()
{
    TargetInit();
}

void Target::TargetInit()
{
    memset(m_falseColor, 0, sizeof(m_falseColor));
    for (int i = 1; i < 255; i++) {
	switch ((i-1) % 6) {
	case 0:	// white (gray)
	    m_falseColor[i].R = m_falseColor[i].G = m_falseColor[i].B = 255;
	    break;
	case 1: // red
	    m_falseColor[i].R = 255;
	    break;
	case 2: // yellow
	    m_falseColor[i].R = m_falseColor[i].G = 255;
	    break;
	case 3: // green
	    m_falseColor[i].G = 255;
	    break;
	case 4: // magenta
	    m_falseColor[i].B = m_falseColor[i].R = 255;
	    break;
	case 5: // cyan
	    m_falseColor[i].G = m_falseColor[i].B = 255;
	    break;
	}
    }

    m_targetCenter.id = kCenter;
    m_targetCenter.height = 1;
    m_targetCenter.angle = 0.0;
    m_targetCenter.distance = 0.0;
    m_targetCenter.valid = false;

    m_targetTop.id = kTop;
    m_targetTop.height = 2;
    m_targetTop.angle = 0.0;
    m_targetTop.distance = 0.0;
    m_targetTop.valid = false;

    m_targetBottom.id = kBottom;
    m_targetBottom.height = 9;
    m_targetBottom.angle = 0.0;
    m_targetBottom.distance = 0.0;
    m_targetBottom.valid = false;

    m_targetLeft.id = kLeft;
    m_targetLeft.height = 1;
    m_targetLeft.angle = 0.0;
    m_targetLeft.distance = 0.0;
    m_targetLeft.valid = false;

    m_targetRight.id = kRight;
    m_targetRight.height = 1;
    m_targetRight.angle = 0.0;
    m_targetRight.distance = 0.0;
    m_targetRight.valid = false;
}

Target::~Target()
{
}

Target::TargetLocation Target::GetTargetLocation( TargetID which )
{
    TargetLocation location;

    switch (which) {
    case kCenter:
	{
	    location = m_targetCenter;
	}
	break;
    case kTop:
	{
	    location = m_targetTop;
	}
	break;
    case kBottom:
	{
	    location = m_targetBottom;
	}
	break;
    case kLeft:
	{
	    location = m_targetLeft;
	}
	break;
    case kRight:
	{
	    location = m_targetRight;
	}
	break;
    default:
	location.id = which;
	location.angle = 0.0;
	location.distance = 0.0;
	location.valid = false;
    }

    return location;
}

//////////////////////////////////////////////////////////////////////////////
//
// image processing constants
//
//////////////////////////////////////////////////////////////////////////////

// IMAGE_PLANE is the distance to the image plane in pixel-width units:
// d' = (half of image width) / tan(half of horizontal field-of-view angle)
//
// from the Axis 206 data sheet:
// image sensor: 1/4" (diagonal)
// lens: 4.0mm, F2.0
// horizontal FOV: 54 degrees
//
// from the Axis M1011 data sheet:
// image sensor: 1/4" (diagonal)
// lens: 4.4mm, F2.0
// horizontal FOV: 47 degrees
//
// from Axis's on-line calculator
// and camera comparison page:
// M1011 lens: 4.2mm F2.0
//
// These numbers don't make sense!  The stated FOV is much smaller than
// that calculated from the lens FL and sensor size.  The stated FOVs
// for the two cameras are very different, even though they supposedly
// use the same size sensor and only slightly different lenses.
//
// with horizontal angle = 54 degrees and WIDTH = 640 pixels
// tan(h/2) = tan(27.0 degrees) = 0.50952545
// (WIDTH/2) / tan(h/2) = 628.03536
//
// with horizontal angle = 49 degrees and WIDTH = 640 pixels
// tan(h/2) = tan(24.5 degrees) = 0.45572626
// (WIDTH/2) / tan(h/2) = 702.17591
//
// with horizontal angle = 48 degrees and WIDTH = 640 pixels
// tan(h/2) = tan(24 degrees) = 0.445228685;
// (WIDTH/2) / tan(h/2) = 718.73177;
//
// with horizontal angle = 47 degrees and WIDTH = 640 pixels
// tan(h/2) = tan(24.5 degrees) = 0.434812375;
// (WIDTH/2) / tan(h/2) = 735.94962

#define	WIDTH	640	// image width, in pixels
#define	HEIGHT	480	// image height, in pixels
#define	BORDER	4	// expected minimum spacing from objects to edge of image

//	IMAGE_PLANE 702.17591	// 49 degrees
//	IMAGE_PLANE 718.73177	// 48 degrees
//	IMAGE_PLANE 735.94962	// 47 degrees

#define	IMAGE_PLANE 693.	// M206 measured (49.6 degrees)

// FRC target dimensions:
//
// hoop vertical spacing
// 28", 61", 98" from floor to top of rim
//
// hoop horizontal spacing
// midline of top/bottom hoop to midline of side hoops 27 3/8"
// midline of left hoop to midline of right hoop 54 3/4"
//
// backboard: 44" x 31 1/2" outside, with dark 2" border
// reflective target: 24" x 18" outside, 2" reflective stripe
// (so inside dimensions are 20" x 14")
// there is an additional 2" dark border inside and outside the reflective stripe
// the bottom edge of the reflective area is 2" above the hoop rim
//
// midline of top/bottom reflector to midline of side reflector 27 3/8"
//
// midline of top/bottom reflector to nearest outer edge of side reflector
// 27 3/8" - (24" / 2) = 15 3/8"
//
// midline of top/bottom reflector to farthest outer edge of side reflector
// 27 3/8" + (24" / 2) = 39 3/8"

#define	FRC_WIDTH	12.000		// half the width of the reflector
#define	FRC_INSIDE	15.375		// center to nearest edge of reflector
#define	FRC_MIDLINE	27.375		// center to midline of reflector
#define	FRC_OUTSIDE	39.375		// center to farthest edge of reflector

// trig constants

#ifndef M_PI
#define	M_PI	3.1415926535
#endif

#define	DEGREES	(180./M_PI)
#define	RADIANS	(M_PI/180.)

//////////////////////////////////////////////////////////////////////////////
//
// image processing functions
//
// These are called from Target::Run to handle all image processing and
// target detection operations.  The variables that they use should not be
// accessed outside the background task.  Target::Run will copy the
// processing results to the TargetLocation variables that are accessible
// from the main task.
//
//////////////////////////////////////////////////////////////////////////////

bool Target::ProcessImage( Image *pImage, float threshold )
{
    m_pTop = m_pBottom = m_pLeft = m_pRight = NULL; // avoid crash in SaveImages
    m_leftX = m_rightX = -1;

    if (!imaqDuplicate(m_cameraImage.GetImaqImage(), pImage)) {
	return false;
    }

    bool result = (FindParticles(threshold) && AnalyzeParticles());
    SaveImages();

    if (result) {
	// store the analysis results
	m_targetCenter.angle = m_centerAngle;
	m_targetCenter.distance = m_centerDistance;
	m_targetCenter.valid = true;

	m_targetTop.angle = m_topAngle;
	//m_targetTop.distance = m_topDistance;
	m_targetTop.distance = m_centerDistance;
	m_targetTop.valid = (m_pTop != NULL);

	m_targetBottom.angle = m_bottomAngle;
	//m_targetBottom.distance = m_bottomDistance;
	m_targetBottom.distance = m_centerDistance;
	m_targetBottom.valid = (m_pBottom != NULL);

	m_targetLeft.angle = m_leftAngle;
	m_targetLeft.distance = m_leftDistance;
	m_targetLeft.valid = !m_leftClipped;

	m_targetRight.angle = m_rightAngle;
	m_targetRight.distance = m_rightDistance;
	m_targetRight.valid = !m_rightClipped;
    } else {
	m_targetCenter.valid = false;
	m_targetTop.valid = false;
	m_targetBottom.valid = false;
	m_targetLeft.valid = false;
	m_targetRight.valid = false;
    }

    return result;
}

static void OverlayParticle( RGBImage *image, Target::Particle *p )
{
    if (p) {
	Point points[4];

	points[0].x = (int)(p->leftBound + 0.5);
	points[0].y = (int)(p->topBound + 0.5);
	points[1].x = (int)(p->leftBound + 0.5);
	points[1].y = (int)(p->bottomBound + 0.5);
	points[2].x = (int)(p->rightBound + 0.5);
	points[2].y = (int)(p->bottomBound + 0.5);
	points[3].x = (int)(p->rightBound + 0.5);
	points[3].y = (int)(p->topBound + 0.5);
	imaqOverlayClosedContour(image->GetImaqImage(), points, 4, &IMAQ_RGB_YELLOW, IMAQ_DRAW_VALUE, NULL);

	points[0].x = (int)(p->xCenter + 0.5) - 10;
	points[0].y = (int)(p->yCenter + 0.5) - 10;
	points[1].x = (int)(p->xCenter + 0.5) + 10;
	points[1].y = (int)(p->yCenter + 0.5) + 10;
	points[2].x = (int)(p->xCenter + 0.5) - 10;
	points[2].y = (int)(p->yCenter + 0.5) + 10;
	points[3].x = (int)(p->xCenter + 0.5) + 10;
	points[3].y = (int)(p->yCenter + 0.5) - 10;
	imaqOverlayLine(image->GetImaqImage(), points[0], points[1], &IMAQ_RGB_WHITE, NULL);
	imaqOverlayLine(image->GetImaqImage(), points[2], points[3], &IMAQ_RGB_WHITE, NULL);
    }
}

static void OverlayCenter( RGBImage *image, double x, double y )
{
    Point points[4];

    points[0].x = (int)(x + 0.5) - 10;
    points[0].y = (int)(y + 0.5) - 10;
    points[1].x = (int)(x + 0.5) + 10;
    points[1].y = (int)(y + 0.5) + 10;
    points[2].x = (int)(x + 0.5) - 10;
    points[2].y = (int)(y + 0.5) + 10;
    points[3].x = (int)(x + 0.5) + 10;
    points[3].y = (int)(y + 0.5) - 10;

    imaqOverlayLine(image->GetImaqImage(), points[0], points[1], &IMAQ_RGB_WHITE, NULL);
    imaqOverlayLine(image->GetImaqImage(), points[2], points[3], &IMAQ_RGB_WHITE, NULL);
}

static void OverlayLimits( RGBImage *image, double leftX, double centerX, double rightX )
{
    Point points[2];

    if (leftX >= 0) {
	points[0].x = points[1].x = (int)(leftX + 0.5);
	points[0].y = 0;
	points[1].y = (HEIGHT - 1);
	imaqOverlayLine(image->GetImaqImage(), points[0], points[1], &IMAQ_RGB_BLUE, NULL);
    }

    if (centerX >= 0) {
	points[0].x = points[1].x = (int)(centerX + 0.5);
	points[0].y = 0;
	points[1].y = (HEIGHT - 1);
	imaqOverlayLine(image->GetImaqImage(), points[0], points[1], &IMAQ_RGB_BLUE, NULL);
    }

    if (rightX >= 0) {
	points[0].x = points[1].x = (int)(rightX + 0.5);
	points[0].y = 0;
	points[1].y = (HEIGHT - 1);
	imaqOverlayLine(image->GetImaqImage(), points[0], points[1], &IMAQ_RGB_BLUE, NULL);
    }
}

void Target::SaveImages()
{
    // long then = (long) GetFPGATime();

    if (!imaqWriteFile(m_cameraImage.GetImaqImage(), "00-camera.bmp", NULL)) {
	printf("%s: imaqWriteFile(\"00-camera.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }
    if (!imaqWriteFile(m_monoImage.GetImaqImage(), "01-monoImage.bmp", NULL)) {
	printf("%s: imaqWriteFile(\"01-monoImage.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }
    if (!imaqWriteFile(m_threshold.GetImaqImage(), "02-threshold.bmp", m_falseColor)) {
	printf("%s: imaqWriteFile(\"02-threshold.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }
    if (!imaqWriteFile(m_convexHull.GetImaqImage(), "03-convexHull.bmp", m_falseColor)) {
	printf("%s: imaqWriteFile(\"03-convexHull.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }
    if (!imaqWriteFile(m_filtered.GetImaqImage(), "04-filtered.bmp", m_falseColor)) {
	printf("%s: imaqWriteFile(\"04-filtered.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }

    imaqMergeOverlay(m_overlay.GetImaqImage(), m_cameraImage.GetImaqImage(), NULL, 0, NULL );

    OverlayParticle(&m_overlay, m_pTop);
    OverlayParticle(&m_overlay, m_pBottom);
    OverlayParticle(&m_overlay, m_pLeft);
    OverlayParticle(&m_overlay, m_pRight);

    OverlayCenter(&m_overlay, m_centerX, m_centerY);

    OverlayLimits(&m_overlay, m_leftX, m_centerX, m_rightX);

    imaqMergeOverlay(m_overlay.GetImaqImage(), m_overlay.GetImaqImage(), NULL, 0, NULL);

    if (!imaqWriteFile(m_overlay.GetImaqImage(), "05-overlay.bmp", FALSE)) {
	printf("%s: imaqWriteFile(\"05-overlay.bmp\") FAILED\n", __FUNCTION__);
	// ignore the error
    }

    // long now = (long) GetFPGATime();
    // printf("%s: image save took %ld microseconds\n", __FUNCTION__, (now - then));
}

bool Target::FindParticles( float threshold )
{
    // long then = (long) GetFPGATime();
    LARGE_INTEGER then;
    QueryPerformanceCounter(&then);

    ImageType type;
    if (!imaqGetImageType(m_cameraImage.GetImaqImage(), &type)) {
	printf("%s: imaqGetImageType FAILED\n", __FUNCTION__);
	return false;
    }

    if (type == IMAQ_IMAGE_U8) {
	m_monoImage = m_cameraImage;
    } else {
	// extract the blue plane
	if (!imaqExtractColorPlanes(m_cameraImage.GetImaqImage(), IMAQ_RGB,
		    NULL, NULL, m_monoImage.GetImaqImage()))
	{
	    printf("%s: imaqExtractColorPlanes FAILED\n", __FUNCTION__);
	    return false;
	}
    }

    // select interesting particles
#if 1
    if (!imaqThreshold(m_threshold.GetImaqImage(), m_monoImage.GetImaqImage(),
    	/*rangeMin*/ threshold, /*rangeMax*/ 256., /*useNewValue*/ 1, /*newValue */ 1))
    {
	printf("%s: imaqThreshold FAILED\n", __FUNCTION__);
	return false;
    }
#else
    ThresholdData *pThD;
    if (!(pThD = imaqAutoThreshold2(m_threshold.GetImaqImage(), m_monoImage.GetImaqImage(),
    			2, IMAQ_THRESH_INTERCLASS, NULL)))
    {
	printf("%s: imaqThreshold FAILED\n", __FUNCTION__);
	return false;
    }
    for (int i = 0; i < 2; i++) {
	printf("range min %g max %g useNewValue %d newValue %g\n",
	    pThD[i].rangeMin, pThD[i].rangeMax, pThD[i].useNewValue, pThD[i].newValue);
    }
    imaqDispose(pThD);
#endif

    if (!imaqConvexHull(m_convexHull.GetImaqImage(), m_threshold.GetImaqImage(),
    	/*connectivity8*/ 1))
    {
	printf("%s: imaqConvexHull FAILED\n", __FUNCTION__);
	return false;
    }

    if (!imaqSizeFilter(m_filtered.GetImaqImage(), m_convexHull.GetImaqImage(),
	/*connectivity8*/ 1, /*erosions*/ 2, /*keepSize*/ IMAQ_KEEP_LARGE,
	/*structuringElement*/ NULL))
    {
	printf("%s: imaqSizeFilter FAILED\n", __FUNCTION__);
	return false;
    }

    int particleCount = 0;
    if (!imaqCountParticles(m_filtered.GetImaqImage(), 1, &particleCount))
    {
	printf("%s: imaqCountParticles FAILED\n", __FUNCTION__);
	return false;
    }

    // select the four largest particles (insertion sort)
    // for now, keep track of only the particle number (index) and size
    memset((void *)m_particles, 0, sizeof m_particles);
    for (int i = 0; i < particleCount; i++) {
	double size;
	if (!imaqMeasureParticle(m_filtered.GetImaqImage(), i, FALSE,
	    IMAQ_MT_PARTICLE_AND_HOLES_AREA, &size))
	{
	    printf("%s: imaqMeasureParticle %d FAILED\n", __FUNCTION__, i);
	    break;
	}
	for (int j = 0; j < 4; j++) {
	    if (size > m_particles[j].size) {
		for (int k = 3; k > j; k--) {
		    m_particles[k].index = m_particles[k-1].index;
		    m_particles[k].size = m_particles[k-1].size;
		}
		m_particles[j].index = i;
		m_particles[j].size = size;
		break;
	    }
	}
    }

    // fill in the rest of the measured data
    for (m_numParticles = 0;
    	 m_numParticles < 4 && m_particles[m_numParticles].size > 0;
	 m_numParticles++)
    {
	Particle* p = &m_particles[m_numParticles];
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_CENTER_OF_MASS_X, &(p->xCenter));
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_CENTER_OF_MASS_Y, &(p->yCenter));
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_BOUNDING_RECT_LEFT, &(p->leftBound));
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_BOUNDING_RECT_RIGHT, &(p->rightBound));
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_BOUNDING_RECT_TOP, &(p->topBound)); 
	imaqMeasureParticle(m_filtered.GetImaqImage(), p->index, FALSE,
			    IMAQ_MT_BOUNDING_RECT_BOTTOM, &(p->bottomBound));
	// calculate height/width from bounding box
	p->height = p->bottomBound - p->topBound;
	p->width = p->rightBound - p->leftBound;
    }

    // long now = (long) GetFPGATime();
    // printf("%s: particle detection took %ld microseconds\n", __FUNCTION__, (now - then));
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LARGE_INTEGER ticksPerSec;
    QueryPerformanceFrequency(&ticksPerSec);
    printf("%s: particle detection took %ld microseconds\n", __FUNCTION__,
      ((now.QuadPart - then.QuadPart) * 1000000 / ticksPerSec.QuadPart));

    printf("%s: found %d particles\n", __FUNCTION__, particleCount);
    printf("%s: returning %d particles\n", __FUNCTION__, m_numParticles);
    for (int i = 0; i < m_numParticles; i++) {
	Particle *p = &m_particles[i];
	printf("  particle %d index %d top %g bottom %g left %g right %g size %g x %g y %g\n",
		i, p->index, p->topBound, p->bottomBound, p->leftBound, p->rightBound,
		p->size, p->xCenter, p->yCenter);
    }

    return true;
}

bool Target::AnalyzeParticles()
{
    // long then = (long) GetFPGATime();
    LARGE_INTEGER then;
    QueryPerformanceCounter(&then);

    if (m_numParticles < 3) {
	printf("ERROR: m_numParticles < 3, no analysis possible\n");
	return false;
    }

    // image quality tests:
    //
    // 1) The four particles should be approximately the same size.  If not, one or more
    // of the particles is partially hidden or we didn't identify the particles correctly.
    //
    // 2) The particles should be laid out without overlapping.  For example, the left
    // and right bounds of the top particle should not overlap the right bound of the
    // left particle or the left bound of the right particle.  This relationship can
    // be used to determine which particle is missing when only 3 are in view.
    // (If only 2 particles are in view, we can't use this to distinguish between e.g.
    // the top+left particles and the right+bottom particles.  TBD: see if we can use
    // e.g. absolute height information to sort this out when we're really close to
    // the targets.)
    //
    // 3) There should be some space between the outer edge of each particle and the edge
    // of the image.  If not, one of the particles is partially outside the field of view.
    //
    // 4) The aspect ratio (height to width ratio) of each (unclipped) particle should be
    // approximately that of the target rectangles.
    //
    // 5) The image center position calculated from the inside edges of the particles,
    // the center of mass of the particles, and the outside edges of the particles should
    // be approximately the same.

    double size; // median size
    if (m_numParticles == 4) {
	size = (m_particles[1].size + m_particles[2].size) / 2.;
    } else {
	// only 3 particles
	size = m_particles[1].size;
    }
    // These limits are very large in order to accomodate
    // clipping and image distortions from the hoops and nets at close range.
    double min_size = size * 0.20;
    double max_size = size * 3.00;

    if (m_particles[0].size > max_size) {
	printf("ERROR: particle 0 is unreasonably large, bad image\n");
	return false;
    }

    if (m_numParticles == 4 && m_particles[3].size < min_size) {
	printf("WARNING: particle 3 is unreasonably small, dropping it\n");
	m_numParticles = 3;
    }

    if (m_numParticles == 3 && m_particles[2].size < min_size) {
	printf("ERROR: particle 2 is unreasonably small\n");
	return false;
    }

    // Sort the particles by position, based on their inside edges.
    // These are in image coordinates, so (0,0) is top-left.

    m_pTop = m_pBottom = m_pLeft = m_pRight = &m_particles[0];

    for (int i = 1; i < m_numParticles; i++) {
	Particle *p = &m_particles[i];
	if (p->bottomBound < m_pTop->bottomBound) {
	    m_pTop = p;
	}
	if (p->topBound > m_pBottom->topBound) {
	    m_pBottom = p;
	}
	if (p->rightBound < m_pLeft->rightBound) {
	    m_pLeft = p;
	}
	if (p->leftBound > m_pRight->leftBound) {
	    m_pRight = p;
	}
    }

    printf("sorted: top %d bottom %d left %d right %d\n",
	m_pTop->index, m_pBottom->index, m_pLeft->index, m_pRight->index);

    if (m_numParticles < 4) {
	// TBD: The if-then-elseif structure here assumes that only one
	// particle is missing.  Rework this to deal with two, or even three
	// missing particles.
	if (m_pTop->bottomBound > m_pLeft->topBound ||
	    m_pTop->bottomBound > m_pRight->topBound)
	{
	    printf("top overlaps left/right, top removed\n");
	    m_pTop = NULL;
	}
	else if (m_pBottom->topBound < m_pLeft->bottomBound ||
	    m_pBottom->topBound < m_pRight->bottomBound)
	{
	    printf("bottom overlaps left/right, bottom removed\n");
	    m_pBottom = NULL;
	}
	else if ((m_pTop && m_pLeft->rightBound > m_pTop->leftBound) ||
	    (m_pBottom && m_pLeft->rightBound > m_pBottom->leftBound))
	{
	    printf("left overlaps top/bottom, left removed\n");
	    m_pLeft = NULL;
	}
	else if ((m_pTop && m_pRight->leftBound < m_pTop->rightBound) ||
	    (m_pBottom && m_pRight->leftBound < m_pBottom->rightBound))
	{
	    printf("right overlaps top/bottom, right removed\n");
	    m_pRight = NULL;
	}
	else
	{
	    printf("ERROR: particle overlap can't be resolved\n");
	    return false;
	}
    }

    // uniqueness check
    if (m_pTop == m_pLeft || m_pTop == m_pRight || m_pTop == m_pBottom ||
	m_pLeft == m_pRight || m_pLeft == m_pBottom || m_pRight == m_pBottom) {
	printf("ERROR: particles aren't unique.\n");
	return false;
    }

    // check outside boundaries and particle sizes for clipping and image artifacts
    m_topClipped = m_bottomClipped = m_leftClipped = m_rightClipped = false;

    if (!m_pTop || m_pTop->topBound < BORDER) {
	printf("WARNING: top particle is clipped\n");
	m_topClipped = true;
    }

    if (!m_pBottom || m_pBottom->bottomBound > (HEIGHT - BORDER)) {
	printf("WARNING: bottom particle is clipped\n");
	m_bottomClipped = true;
    }

    if (!m_pLeft || m_pLeft->leftBound < BORDER) {
	printf("WARNING: left particle is clipped\n");
	m_leftClipped = true;
    }

    if (!m_pRight || m_pRight->rightBound > (WIDTH - BORDER)) {
	printf("WARNING: right particle is clipped\n");
	m_rightClipped = true;
    }

    //////////////////////////////////////////////////////////////////////////
    //
    // Calculate the angle and distance to the center of the target(s).
    //
    // Project data from the image plane to the real-world plane of the
    // backboards using what we know about the real-world dimensions of
    // the features we're seeing.  From that, we can calculate the angle
    // and distance from the camera centerline to each feature.  In order
    // to aim the robot, we may also need to deal with a rotation and/or
    // translation between the camera centerline and the robot centerline,
    // but we'll ignore that for now.
    //
    // If we're shooting from either side of the field, we may need to
    // adjust our aiming point to hit the center of the hoops rather than
    // aiming directly for the center of the reflective targets.  Again,
    // we'll ignore that for now.
    //
    // For a given field-of-view angle and image plane width (in pixels),
    // the distance (in pixel-width units) to the image plane is given by:
    //   d' = (w'/2) / tan(h/2)
    // where w is the width of the image plane in pixels (640) and h is
    // the horizontal angle of view (49 degrees for this camera/lens.)
    // This constant has been pre-calculated (IMAGE_PLANE).
    //
    // The view angle for an object at x' pixels from the center can be
    // solved from:
    //   d' = x' / tan(a)
    //   tan(a) = x' / d'
    //   a = atan(x' / d')
    //
    // Given a desired target and the real-world distances to visible targets
    // on either side of it, we can calculate the distance to the target.
    // Once we have the distance to the center target, we can calculate
    // the distances to the targets on either side.
    //
    //////////////////////////////////////////////////////////////////////////

    if (m_pTop) {
	m_topAngle = atan((m_pTop->xCenter-(WIDTH/2))/IMAGE_PLANE)*DEGREES;
	printf("top angle %g degrees\n", m_topAngle);
    }

    if (m_pBottom) {
	m_bottomAngle = atan((m_pBottom->xCenter-(WIDTH/2))/IMAGE_PLANE)*DEGREES;
	printf("bottom angle %g degrees\n", m_bottomAngle);
    }

    if (m_pTop && m_pBottom) {
	m_centerX = (m_pTop->xCenter + m_pBottom->xCenter) / 2.;
	m_centerAngle = atan((m_centerX - (WIDTH/2)) / IMAGE_PLANE) * DEGREES;
	printf("center angle %g degrees\n", m_centerAngle);
    } else if (m_pTop) {
	m_centerX = m_pTop->xCenter;
	m_centerAngle = m_topAngle;
	m_bottomAngle = m_topAngle;  // best available estimate
    } else if (m_pBottom) {
	m_centerX = m_pBottom->xCenter;
	m_centerAngle = m_bottomAngle;
	m_topAngle = m_bottomAngle;  // best available estimate
    } else {
	printf("ERROR: m_pTop and m_pBottom are both NULL (can't happen!)\n");
	return false;
    }

    if (m_pLeft && m_pRight) {
	m_centerY = (m_pLeft->yCenter + m_pRight->yCenter) / 2.;
    } else if (m_pLeft) {
	m_centerY = m_pLeft->yCenter;
    } else if (m_pRight) {
	m_centerY = m_pRight->yCenter;
    } else {
	printf("ERROR: m_pLeft and m_pRight are both NULL (can't happen!)\n");
	return false;
    }

    double leftWidthReal;
    double leftWidthImage;

    if (m_pLeft) {
	if (m_leftClipped) {
	    m_leftX = m_pLeft->rightBound;
	    leftWidthImage = m_centerX - m_leftX;
	    leftWidthReal = FRC_INSIDE;
	} else {
	    m_leftX = m_pLeft->leftBound;
	    leftWidthImage = m_centerX - m_leftX;
	    leftWidthReal = FRC_OUTSIDE;
	}
    } else if (m_pTop) {
	m_leftX = m_pTop->leftBound;
	leftWidthImage = m_centerX - m_leftX;
	leftWidthReal = FRC_WIDTH;
    } else if (m_pBottom) {
	m_leftX = m_pBottom->leftBound;
	leftWidthImage = m_centerX - m_leftX;
	leftWidthReal = FRC_WIDTH;
    } else {
	printf("ERROR: m_pTop and m_pBottom are both NULL (can't happen!)\n");
	return false;
    }

    double theta1 = atan( leftWidthImage / IMAGE_PLANE );
    m_leftAngle = m_centerAngle - theta1 * DEGREES;	// relative to robot
    printf("theta1 %g, left angle %g degrees\n", theta1*DEGREES, m_leftAngle);

    double rightWidthReal;
    double rightWidthImage;

    if (m_pRight) {
	if (m_rightClipped) {
	    m_rightX = m_pRight->leftBound;
	    rightWidthImage = m_rightX - m_centerX;
	    rightWidthReal = FRC_INSIDE;
	} else {
	    m_rightX = m_pRight->rightBound;
	    rightWidthImage = m_rightX - m_centerX;
	    rightWidthReal = FRC_OUTSIDE;
	}
    } else if (m_pTop) {
	m_rightX = m_pTop->rightBound;
	rightWidthImage = m_rightX - m_centerX;
	rightWidthReal = FRC_WIDTH;
    } else if (m_pBottom) {
	m_rightX = m_pBottom->rightBound;
	rightWidthReal = FRC_WIDTH;
	rightWidthImage = m_rightX - m_centerX;
    } else {
	printf("ERROR: m_pTop and m_pBottom are both NULL (can't happen!)\n");
	return false;
    }

    double theta2 = atan( rightWidthImage / IMAGE_PLANE );
    m_rightAngle = m_centerAngle + theta2 * DEGREES;	// relative to robot
    printf("theta2 %g, right angle %g degrees\n", theta2*DEGREES, m_rightAngle);

    // these quantities appear repeatedly in the calculations
    double k1 = sin(theta1) / leftWidthReal;
    double k2 = sin(theta2) / rightWidthReal;
    double k3 = M_PI - (theta1 + theta2);
    double sink3 = sin(k3);
    double cosk3 = cos(k3);
    printf("k1 %g k2 %g k3 %g sin k3 %g cos k3 %g\n",
	k1, k2, k3*DEGREES, sink3, cosk3);

    // Calculate the distance to the center (c) using either
    // the triangle on the left or the right.
    double alpha = atan((k1*sink3)/(k2+k1*cosk3));
    if (alpha < 0) { alpha += M_PI; }
    printf("alpha %g degrees\n", alpha * DEGREES);
    double c1 = sin(alpha) / k1;

    double beta = atan((k2*sink3)/(k1+k2*cosk3));
    if (beta < 0) { beta += M_PI; }
    printf("beta %g degrees\n", beta * DEGREES);
    double c2 = sin(beta) / k2;

    // Check: the sum of alpha+beta should be equal to k3.
    printf("alpha+beta %g k3 %g\n", (alpha+beta)*DEGREES, k3*DEGREES);

    // Check: these two distance calculations should agree.
    printf("distance1 %g, distance2 %g\n", c1, c2);

    m_centerDistance = c1;

    // angle "gamma1" and side "b" in Jeff's diagram
    m_leftDistance = sin(M_PI - (theta1 + alpha)) / k1;

    // angle "gamma2" and side "a" in Jeff's diagram
    m_rightDistance = sin(M_PI - (theta2 + beta)) / k2;

    printf("distance left %g, center %g, right %g\n",
	    m_leftDistance, m_centerDistance, m_rightDistance);

    // long now = (long) GetFPGATime();
    // printf("%s: particle analysis took %ld microseconds\n", __FUNCTION__, (now - then));
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    LARGE_INTEGER ticksPerSec;
    QueryPerformanceFrequency(&ticksPerSec);
    printf("%s: particle analysis took %ld microseconds\n", __FUNCTION__,
      ((now.QuadPart - then.QuadPart) * 1000000 / ticksPerSec.QuadPart));

    return true;
}

bool Target::GetProcessedImage(Image* image)
{
    return (imaqDuplicate(image, m_overlay.GetImaqImage()) != 0);
}

