#ifndef DS4_PARTICLESAPP_H
#define DS4_PARTICLESAPP_H

#ifdef _DEBUG
#pragma comment(lib, "DSAPI32.dbg.lib")
#else
#pragma comment(lib, "DSAPI32.lib")
#endif
#include <memory>
#include <boost/program_options.hpp>
#include "cinder/app/AppNative.h"
#include "cinder/Arcball.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/MonitorNode.h"
#include "cinder/audio/Utilities.h"
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
namespace bpo = boost::program_options;

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

	const int NUM_COLORMODES = 4;
	enum DS4PColorMode
	{
		COLOR_MODE_BLUE=0,	//all blue
		COLOR_MODE_GOLD,	//all gold
		COLOR_MODE_BLUE_P,	//blue
		COLOR_MODE_GOLD_P,
		COLOR_MODE_BLUE_P2
	};

private:
	void setupGUI();
	bool setupDSAPI();
	void setupScene();
	void setupAudio();
	void setupColors();

	void updateCV();
	void updateAudio();

	void drawDebug();
	void drawRunning();
	void drawCamInfo();

	void readConfig();
	void writeConfig();

	//Audio stuff
	audio::InputDeviceNodeRef mInputDeviceNode;
	audio::MonitorSpectralNodeRef mMonitorSpectralNode;
	vector<float> mMagSpectrum;
	float mMagMean;

	//Colors
	Color mIntelBlue;
	Color mIntelPaleBlue;	
	Color mIntelLightBlue;
	Color mIntelDarkBlue;
	Color mIntelYellow;
	Color mIntelOrange;
	Color mIntelGreen;

	Color mCloudColor;
	Color mParticleColor;
	Color mBoltColor;
	DS4PColorMode mColorMode;

	//scene
	bool mDrawBackground, mDrawLogo;
	Arcball mArcball;
	MayaCamUI mMayaCam;
	CameraPersp mCamera;
	gl::Texture mBackground;
	gl::Texture mLogo;

	//Point cloud
	vector<Vec3f> mCloudPoints;
	vector<Vec3f> mContourPoints;
	vector<Vec3f> mBorderPoints;
	DS4ParticleSystem mParticleSystem;

	//DS
	DSAPIRef mDSAPI;
	DSCalibIntrinsicsRectified mZIntrinsics;
	uint8_t *mDepthPixels, *mPrevDepthPixels;
	uint16_t *mDepthBuffer, *mPrevDepthBuffer;

	//OpenCV
	cv::Mat mMatCurrent;
	cv::Mat mMatPrev;
	cv::Mat mMatDiff;
	vector<vector<cv::Point>> mContours;
	vector<vector<cv::Point>> mContoursKeep;
	vector<cv::Vec4i> mHierarchy;

	//Settings
	params::InterfaceGlRef mGUI;
	int mDepthMin,
		mDepthMax,
		mFramesSpawn,
		mCloudRes,
		mSpawnRes,
		mBoltRes,
		mAgeMin,
		mAgeMax,
		mLogoSize;

	int mNumParticles;
	double	mThresh,
			mSizeMin;
	float	mFPS,
			mPointSize,
			mBoltWidthMin,
			mBoltWidthMax,
			mParticleSize,
			mSpawnLevel,
			mBoltAlphaMin,
			mBoltAlphaMax,
			mParticleAlpha,
			mLogoAlpha,
			mBGAlpha;
	bool mIsDebug;
	gl::Texture mTexBase;
	gl::Texture mTexCountour;
	gl::Texture mTexBlob;
	gl::Texture mTexBlobContour;
	gl::Texture mXDDLogo;
	gl::Texture mCinderLogo;

	//Mine
	bool mCamInfo;
};

#endif