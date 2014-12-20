#ifndef DS4_PARTICLESAPP_H
#define DS4_PARTICLESAPP_H

#ifdef _DEBUG
#pragma comment(lib, "DSAPI32.dbg.lib")
#else
#pragma comment(lib, "DSAPI32.lib")
#endif
#include <memory>
#include "cinder/app/AppNative.h"
#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/params/Params.h"
#include "DSAPI.h"
#include "CinderOpenCV.h"
#include "DS4Particle.h"

using namespace ci;
using namespace ci::app;
using namespace std;

typedef shared_ptr<DSAPI> DSAPIRef;

class DS4ParticlesApp : public AppNative
{
public:

	void prepareSettings(Settings *pSettings);
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void shutdown();

private:
	void setupGUI();
	bool setupDSAPI();
	void setupCVandGL();
	void updateTextures();
	void updatePointCloud();
	void updateParticles();
	void drawDebug();
	void drawRunning();

	//Point cloud
	CameraPersp mCamera;
	vector<Vec3f> mCloudPoints;
	vector<Vec3f> mContourPoints;
	vector<Vec3f> mParticles;

	//DS
	DSAPIRef mDSAPI;
	DSCalibIntrinsicsRectified mZIntrinsics;
	uint8_t *mDepthPixels;
	uint16_t *mDepthBuffer;

	//OpenCV
	cv::Mat mMatCurrent;
	cv::Mat mMatPrev;
	cv::Mat mMatDiff;
	vector<vector<cv::Point>> mContours;
	vector<cv::Vec4i> mHierarchy;

	//Debug
	params::InterfaceGlRef mGUI;
	int mDepthMin, mDepthMax;
	double mThresh, mSizeMin;
	float mFPS;
	bool mIsDebug;
	gl::Texture mTexBase;
	gl::Texture mTexCountour;
	gl::Texture mTexBlob;
	gl::Texture mTexBlobContour;
};

#endif