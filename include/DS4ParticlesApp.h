#ifndef DS4_PARTICLESAPP_H
#define DS4_PARTICLESAPP_H

#ifdef _DEBUG
#pragma comment(lib, "DSAPI32.dbg.lib")
#else
#pragma comment(lib, "DSAPI32.lib")
#endif
#include <memory>
#include "cinder/app/AppNative.h"
#include "cinder/Arcball.h"
#include "cinder/Camera.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/MayaCamUI.h"
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
	void update();
	void resize();
	void mouseDown(MouseEvent pEvent);
	void mouseDrag(MouseEvent pEvent);
	void keyDown(KeyEvent pEvent);
	void draw();
	void shutdown();

private:
	void setupGUI();
	bool setupDSAPI();
	void setupScene();
	void setupColors();
	void updateTextures();
	void updatePointCloud();
	void updateCV();
	void drawDebug();
	void drawRunning();

	//Colors
	Color IntelBlue;
	Color IntelPaleBlue;	
	Color IntelLightBlue;
	Color IntelDarkBlue;
	Color IntelYellow;
	Color IntelOrange;
	Color IntelGreen;

	//scene
	Arcball mArcball;
	MayaCamUI mMayaCam;
	CameraPersp mCamera;

	//Point cloud
	float mColorShift;
	vector<Vec3f> mCloudPoints;
	vector<Vec3f> mContourPoints;
	vector<Vec3f> mBorderPoints;
	DS4ParticleSystem mParticleSystem;
	gl::Texture mBackground;

	//DS
	DSAPIRef mDSAPI;
	DSCalibIntrinsicsRectified mZIntrinsics;
	uint8_t *mDepthPixels;
	uint16_t *mDepthBuffer, *mPrevDepthBuffer;

	//OpenCV
	cv::Mat mMatCurrent;
	cv::Mat mMatPrev;
	cv::Mat mMatDiff;
	vector<vector<cv::Point>> mContours;
	vector<vector<cv::Point>> mContoursKeep;
	vector<cv::Vec4i> mHierarchy;

	//Debug
	params::InterfaceGlRef mGUI;
	int mDepthMin, mDepthMax, mFramesSpawn, mCloudRes, mSpawnRes, mBoltRes, mAgeMin, mAgeMax;
	size_t mNumParticles;
	double mThresh, mSizeMin;
	float mFPS, mPointSize, mBoltWidth, mParticleSize; 
	bool mIsDebug;
	gl::Texture mTexBase;
	gl::Texture mTexCountour;
	gl::Texture mTexBlob;
	gl::Texture mTexBlobContour;
};

#endif