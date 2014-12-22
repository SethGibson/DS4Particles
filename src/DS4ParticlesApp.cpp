#include <cstring>
#include <cstdlib>
#include "DS4ParticlesApp.h"
#include "DSAPIUtil.h"


static Vec2i S_DEPTH_SIZE(480, 360);
static Vec2i S_APP_SIZE(1280, 800);
static size_t S_MAX_PARTICLES = 5000;

void DS4ParticlesApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(S_APP_SIZE.x, S_APP_SIZE.y);
	pSettings->setFrameRate(60);
	//pSettings->setFullScreen(true);
}

void DS4ParticlesApp::setup()
{
	mIsDebug = false;
	mCamInfo = false;
	if (!setupDSAPI())
		console() << "Error Starting DSAPI Session" << endl;

	setupScene();
	setupGUI();
	setupColors();
}

void DS4ParticlesApp::update()
{
	if (mDSAPI->isZEnabled())
	{
		if (mDSAPI->grab())
		{
			mDepthBuffer = mDSAPI->getZImage();
			updateCV();
		}
	}
	mFPS = getAverageFps();
}

void DS4ParticlesApp::resize()
{
	mArcball.setWindowSize(getWindowSize());
	mArcball.setCenter(Vec2f(getWindowWidth() / 2.0f, getWindowHeight() / 2.0f));
	mArcball.setRadius(500);

	//
}

void DS4ParticlesApp::keyDown(KeyEvent pEvent)
{
	if (pEvent.getChar() == 'd')
		mIsDebug = !mIsDebug;

}

void DS4ParticlesApp::mouseDown(MouseEvent pEvent)
{
	if (pEvent.isAltDown())
		mMayaCam.mouseDown(pEvent.getPos());
	else
		mArcball.mouseDown(pEvent.getPos());
}

void DS4ParticlesApp::mouseDrag(MouseEvent pEvent)
{
	if (pEvent.isAltDown())
		mMayaCam.mouseDrag(pEvent.getPos(), pEvent.isLeftDown(), pEvent.isMiddleDown(), pEvent.isRightDown());
	else
		mArcball.mouseDrag(pEvent.getPos());

}

void DS4ParticlesApp::draw()
{
	if (mIsDebug)
		drawDebug();
	else
		drawRunning();
}

#pragma region Setup
bool DS4ParticlesApp::setupDSAPI()
{
	bool retVal = true;
	mDSAPI = DSAPIRef(DSCreate(DS_DS4_PLATFORM), DSDestroy);
	if (!mDSAPI->probeConfiguration())
	{
		retVal = false;
		console() << "Unable to get DS hardware config" << endl;
	}
	if (!mDSAPI->isCalibrationValid())
	{
		retVal = false;
		console() << "Calibration is invalid" << endl;
	}
	if (!mDSAPI->enableLeft(true))
	{
		retVal = false;
		console() << "Unable to start left stream" << endl;
	}
	if (!mDSAPI->enableRight(true))
	{
		retVal = false;
		console() << "Unable to start right stream" << endl;
	}
	if (!mDSAPI->enableZ(true))
	{
		retVal = false;
		console() << "Unable to start depth stream" << endl;
	}
	if (!mDSAPI->setLRZResolutionMode(true, S_DEPTH_SIZE.x, S_DEPTH_SIZE.y, 60, DS_LUMINANCE8))
	{
		retVal = false;
		console() << "Unable to set requested depth resolution" << endl;
	}
	if(!mDSAPI->getCalibIntrinsicsZ(mZIntrinsics))
	{
		retVal = false;
		console() << "Unable to set get depth intrinsics" << endl;
	}
	if (!mDSAPI->startCapture())
	{
		retVal = false;
		console() << "Unable to start ds4" << endl;
	}

	return retVal;
}

void DS4ParticlesApp::setupScene()
{
	mDepthPixels = new uint8_t[S_DEPTH_SIZE.x*S_DEPTH_SIZE.y]{0};
	mPrevDepthBuffer = new uint16_t[S_DEPTH_SIZE.x*S_DEPTH_SIZE.y];
	mMatCurrent = cv::Mat::zeros(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1));
	mMatPrev = cv::Mat::zeros(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1));

	mCamera.setPerspective(45.0f, getWindowAspectRatio(), 100, 4000);
	mCamera.setEyePoint(Vec3f(0, 705, -731));
	mCamera.setViewDirection(Vec3f(0, -0.43f, 0.903f));
	mCamera.setWorldUp(Vec3f(0, 1, 0));
	mMayaCam.setCurrentCam(mCamera);

	mArcball.setWindowSize(getWindowSize());
	mArcball.setCenter(Vec2f(getWindowWidth() / 2.0f, getWindowHeight() / 2.0f));
	mArcball.setRadius(500);
	mColorShift = 1;

	mBackground = gl::Texture(loadImage(loadAsset("background.png")));
}

void DS4ParticlesApp::setupGUI()
{
	mDepthMin = 0;
	mDepthMax = 2000;
	mThresh = 128;
	mSizeMin = 250;


	mCloudRes = 2; //Cloud Resolution
	mSpawnRes = 4; //Spawn Resolution
	mBoltRes = 2;  //Bolt Resolution

	mPointSize = 2.0f;		//Point Size
	mBoltWidth = 4.0f;		//Bolt Width

	mNumParticles = 5000;
	mParticleSize = 2.0f;	//Particle Size
	mAgeMin = 30;			//Min Particle Age
	mAgeMax = 120;			//Max Particle Age
	mFramesSpawn = 5;

	mGUI = params::InterfaceGl::create("Config", Vec2i(250, 400));
	mGUI->addText("Depth Params");
	mGUI->addParam("Min Depth", &mDepthMin);
	mGUI->addParam("Max Depth", &mDepthMax);
	mGUI->addParam("Threshold", &mThresh);
	mGUI->addParam("Min Poly Area", &mSizeMin);
	mGUI->addSeparator();
	mGUI->addText("Point Cloud Params");
	mGUI->addParam("Cloud Res", &mCloudRes);
	mGUI->addParam("Bolt Res", &mBoltRes);
	mGUI->addParam("Spawner Res", &mSpawnRes);
	mGUI->addParam("Point Size", &mPointSize);
	mGUI->addParam("Bolt Width", &mBoltWidth);
	mGUI->addSeparator();
	mGUI->addText("Particle Params");
	mGUI->addParam("Particle Count", &mNumParticles);
	mGUI->addParam("Particle Size", &mParticleSize);
	mGUI->addParam("Min Age", &mAgeMin);
	mGUI->addParam("Max Age", &mAgeMax);
	mGUI->addParam("Spawn Rate", &mFramesSpawn);
	mGUI->addParam("Avg Framerate", &mFPS);
}

void DS4ParticlesApp::setupColors()
{
	IntelBlue = Color::hex(0x0071c5);
	IntelPaleBlue = Color::hex(0x7ed3f7);
	IntelLightBlue = Color::hex(0x00aeef);
	IntelDarkBlue = Color::hex(0x004280);
	IntelYellow = Color::hex(0xffda00);
	IntelOrange = Color::hex(0xfdb813);
	IntelGreen = Color::hex(0xa6ce39);
}
#pragma endregion Setup

#pragma region Update

void DS4ParticlesApp::updateCV()
{
	int did = 0;
	mCloudPoints.clear();
	mBorderPoints.clear();
	while (did < S_DEPTH_SIZE.x*S_DEPTH_SIZE.y)
	{
		float cDepthVal = (float)mDepthBuffer[did];
		if (cDepthVal > mDepthMin&&cDepthVal < mDepthMax)
			mDepthPixels[did] = (uint8_t)(lmap<float>(cDepthVal, mDepthMin, mDepthMax, 255, 0));

		else
			mDepthPixels[did] = 0;

		did += 1;
	}

	mMatCurrent = cv::Mat(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1), mDepthPixels);
	cv::threshold(mMatCurrent, mMatCurrent, mThresh, 255, CV_THRESH_BINARY);

	for (int dy = 0; dy < S_DEPTH_SIZE.y; dy++)
	{
		for (int dx = 0; dx < S_DEPTH_SIZE.x; dx ++)
		{
			float cDepthVal = (float)mDepthBuffer[dy*S_DEPTH_SIZE.x + dx];
			float cInPoint[] = { static_cast<float>(dx), static_cast<float>(dy), cDepthVal }, cOutPoint[3];
			if ((cDepthVal>mDepthMin&&cDepthVal < mDepthMax) && mMatCurrent.at<uint8_t>(dy, dx)>128)
			{
				DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
				if (dx % mCloudRes == 0&&dy%mCloudRes==0)
					mCloudPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
				else if (dy >= S_DEPTH_SIZE.y - 2 && dx%mCloudRes==0)
					mBorderPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
			}
		}
	}

	if (getElapsedFrames() % mFramesSpawn == 0)
	{
		mContours.clear();
		mContourPoints.clear();

		cv::absdiff(mMatCurrent, mMatPrev, mMatDiff);
		cv::findContours(mMatDiff, mContours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		for (auto cContour : mContours)
		{
			if (cv::contourArea(cContour, false) > mSizeMin)
			{
				for (int vi = 0; vi < cContour.size(); vi++)
				{
					cv::Point cPoint = cContour[vi];
					if (vi % mSpawnRes == 0)
					{
						uint16_t cZ = mPrevDepthBuffer[cPoint.y*S_DEPTH_SIZE.x + cPoint.x];
						if (cZ>mDepthMin&&cZ < mDepthMax)
						{
							float cInPoint[] = { static_cast<float>(cPoint.x), static_cast<float>(cPoint.y), cZ }, cOutPoint[3];
							DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
							if (mParticleSystem.count() < mNumParticles&&cOutPoint[1] < 50)
								mParticleSystem.add(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]), Vec3f(randFloat(-0.15f, 0.15f), randFloat(-2, -6), randFloat(0, -1)), Vec2f(mAgeMin,mAgeMax));
						}
					}

					uint16_t cZ2 = mDepthBuffer[cPoint.y*S_DEPTH_SIZE.x + cPoint.x];
					if (cZ2>mDepthMin&&cZ2 < mDepthMax)
					{
						float cInPoint2[] = { static_cast<float>(cPoint.x), static_cast<float>(cPoint.y), cZ2 }, cOutPoint2[3];
						DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint2, cOutPoint2);
						mContourPoints.push_back(Vec3f(cOutPoint2[0], -cOutPoint2[1], cOutPoint2[2]));
					}
				}
			}
		}

		mMatCurrent.copyTo(mMatPrev);
		memcpy(mPrevDepthBuffer, mDepthBuffer, (size_t)(S_DEPTH_SIZE.x*S_DEPTH_SIZE.y*sizeof(uint16_t)));
		if (mIsDebug)
			mTexBlob = gl::Texture(fromOcv(mMatDiff));

	}

	if (mIsDebug)
		mTexBase = gl::Texture(fromOcv(mMatCurrent));

	if (!mIsDebug)
		mParticleSystem.step();
}

#pragma endregion Update

#pragma region Draw
void DS4ParticlesApp::drawDebug()
{
	gl::clear(Color::black());
	gl::setMatricesWindow(S_APP_SIZE.x, S_APP_SIZE.y);
	gl::color(Color::white());
	
	if (mTexBase)
		gl::draw(mTexBase, Rectf(0, 0, S_APP_SIZE.x / 2, S_APP_SIZE.y / 2));
	if (mTexBlob)
		gl::draw(mTexBlob, Rectf(0, S_APP_SIZE.y / 2, S_APP_SIZE.x / 2, S_APP_SIZE.y));
	if (mContours.size() > 0)
	{
		gl::pushMatrices();
		gl::translate(Vec2f(S_APP_SIZE.x / 2, S_APP_SIZE.y / 2));
		gl::scale(Vec2f((S_APP_SIZE.x / (float)S_DEPTH_SIZE.x)*0.5f, (S_APP_SIZE.y / (float)S_DEPTH_SIZE.y)*0.5f));
		gl::color(IntelGreen);
		gl::begin(GL_POINTS);
		glPointSize(2.0);

		for (auto cit : mContours)
		{
			for (auto vit : cit)
				gl::vertex(vit.x, vit.y);
		}
		gl::end();
		gl::popMatrices();
	}

	if (mContours.size() > 0)
	{
		gl::pushMatrices();
		gl::translate(Vec2f(S_APP_SIZE.x / 2, 0));
		gl::scale(Vec2f((S_APP_SIZE.x / (float)S_DEPTH_SIZE.x)*0.5f, (S_APP_SIZE.y / (float)S_DEPTH_SIZE.y)*0.5f));
		gl::color(IntelYellow);
		
		for (auto cContour : mContours)
		{
			if (cv::contourArea(cContour, false) > mSizeMin)
			{
				gl::begin(GL_LINE_LOOP);
				for (auto cPt : cContour)
					gl::vertex(cPt.x, cPt.y);
				gl::end();
			}
		}
		gl::popMatrices();
	}
	mGUI->draw();
}

void DS4ParticlesApp::drawRunning()
{
	mColorShift = math<float>::abs(math<float>::sin(getElapsedFrames()*0.005f));

	gl::clear(Color::black());
	gl::color(Color::white());
	gl::setMatricesWindow(getWindowSize());
	gl::draw(mBackground, Vec2i::zero());

	gl::setMatrices(mMayaCam.getCamera());
	gl::pushMatrices();
	gl::rotate(mArcball.getQuat());
	gl::scale(-1, 1, 1);
	gl::enableAdditiveBlending();
	gl::enable(GL_POINT_SIZE);

	//Point Cloud
	gl::color(IntelDarkBlue);
	glPointSize(mPointSize);
	gl::begin(GL_POINTS);

	for (auto pit : mCloudPoints)
	{
		gl::vertex(pit);
	}
	gl::end();

	//Lightning Bolts
	glPointSize(mBoltWidth);
	gl::color(ColorA(IntelPaleBlue.r, IntelPaleBlue.g, IntelPaleBlue.b, 0.25f));
	gl::begin(GL_POINTS);
	for (auto pit2 : mContourPoints)
	{
		gl::vertex(pit2);
	}
	gl::end();

	//glPointSize(mBoltWidth);
	gl::color(ColorA(IntelPaleBlue.r, IntelPaleBlue.g, IntelPaleBlue.b, 0.75f));
	gl::begin(GL_POINTS);
	for (auto bpit : mBorderPoints)
	{
		gl::vertex(bpit);
	}
	gl::end();

	//Particles
	glPointSize(mParticleSize);
	gl::color(ColorA(IntelPaleBlue.r, IntelPaleBlue.g, IntelPaleBlue.b, 0.75f));
	mParticleSystem.display();
	gl::popMatrices();

	if (mCamInfo)
		drawCamInfo();
}

void DS4ParticlesApp::drawCamInfo()
{
	gl::setMatricesWindow(S_APP_SIZE.x, S_APP_SIZE.y);
	Vec3f cEyePoint(mMayaCam.getCamera().getEyePoint());
	Vec3f cViewDir(mMayaCam.getCamera().getViewDirection());

	string cEyeStr("Eye Point: " + to_string(cEyePoint.x) + " " + to_string(cEyePoint.y) + " " + to_string(cEyePoint.z));
	string cViewStr("Eye Point: " + to_string(cViewDir.x) + " " + to_string(cViewDir.y) + " " + to_string(cViewDir.z));

	gl::drawString(cEyeStr, Vec2i(20, 20));
	gl::drawString(cViewStr, Vec2i(20, 40));
}


#pragma endregion Draw

void DS4ParticlesApp::shutdown()
{
	mDSAPI->stopCapture();
}

CINDER_APP_NATIVE( DS4ParticlesApp, RendererGl )
