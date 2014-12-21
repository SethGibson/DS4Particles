#include <cstring>
#include <cstdlib>
#include "DS4ParticlesApp.h"
#include "DSAPIUtil.h"

static Vec2i S_DEPTH_SIZE(480, 360);
static Vec2i S_APP_SIZE(1280, 800);
static size_t S_MAX_PARTICLES = 2500;
void DS4ParticlesApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(S_APP_SIZE.x, S_APP_SIZE.y);
	pSettings->setFrameRate(60);
	//pSettings->setFullScreen(true);
}

void DS4ParticlesApp::setup()
{
	mIsDebug = false;
	if (!setupDSAPI())
		console() << "Error Starting DSAPI Session" << endl;

	setupScene();
	setupGUI();
}

void DS4ParticlesApp::update()
{
	if (mDSAPI->isZEnabled())
	{
		if (mDSAPI->grab())
		{
			mDepthBuffer = mDSAPI->getZImage();
			updateTextures();
			updatePointCloud();
		}
	}
	mFPS = getAverageFps();
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

	mCamera.setPerspective(45.0f, S_DEPTH_SIZE.x/(float)S_DEPTH_SIZE.y, 100, 4000);
	mCamera.setEyePoint(Vec3f(0, 700, -700));
	mCamera.setViewDirection(Vec3f(0, -0.5f, 1.0f));
	mCamera.setWorldUp(Vec3f(0, 1, 0));
	mMayaCam.setCurrentCam(mCamera);

	mArcball.setWindowSize(getWindowSize());
	mArcball.setCenter(Vec2f(getWindowWidth() / 2.0f, getWindowHeight() / 2.0f));
	mArcball.setRadius(500);
	mColorShift = 1;
}

void DS4ParticlesApp::setupGUI()
{
	mDepthMin = 0;
	mDepthMax = 2000;
	mSizeMin = 250;
	mThresh = 128;

	mFramesSpawn = 5;
	mGUI = params::InterfaceGl::create("Config", Vec2i(200, 200));
	mGUI->addParam("Min Depth", &mDepthMin);
	mGUI->addParam("Max Depth", &mDepthMax);
	mGUI->addParam("Threshold", &mThresh);
	mGUI->addParam("Min Poly Area", &mSizeMin);
	mGUI->addParam("Spawn Time", &mFramesSpawn);
	mGUI->addParam("Avg Framerate", &mFPS);
}
#pragma endregion Setup

#pragma region Update
void DS4ParticlesApp::updateTextures()
{
	int did = 0;

	while (did<(S_DEPTH_SIZE.x*S_DEPTH_SIZE.y))
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
	if (getElapsedFrames() % mFramesSpawn == 0)
	{
		mContours.clear();
		mContoursKeep.clear();
		mHulls.clear();

		cv::absdiff(mMatCurrent, mMatPrev, mMatDiff);
		mMatCurrent.copyTo(mMatPrev);
		memcpy(mPrevDepthBuffer, mDepthBuffer, (size_t)(S_DEPTH_SIZE.x*S_DEPTH_SIZE.y*sizeof(uint16_t)));
		cv::findContours(mMatDiff, mContours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		for (auto cContour : mContours)
		{
			vector<cv::Point> cHull;
			cv::convexHull(cContour, cHull);
			if (cHull.size() > 3)
			{
				vector<cv::Point> cPoly;
				cv::approxPolyDP(cHull, cPoly, 0, true);
				if (cv::contourArea(cPoly, false) > mSizeMin)
				{
					mHulls.push_back(cHull);
					mContoursKeep.push_back(cContour);
				}
			}
		}

		for (auto cKeep : mContoursKeep)
		{
			for (int vi = 0; vi < cKeep.size();vi+=4)
			{
				cv::Point cPoint = cKeep[vi];
				uint16_t cZ = mPrevDepthBuffer[cPoint.y*S_DEPTH_SIZE.x + cPoint.x];
				if (cZ>mDepthMin&&cZ < mDepthMax)
				{
					float cInPoint[] = { static_cast<float>(cPoint.x), static_cast<float>(cPoint.y), cZ }, cOutPoint[3];
					DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
					if (mParticleSystem.count() < S_MAX_PARTICLES)
					{
						if (cOutPoint[1]<50)
							mParticleSystem.add(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]),Vec3f(randFloat(-1, 1), randFloat(-2, -6), randFloat(0, -1)));
					}
				}
			}
		}

		if (mIsDebug)
			mTexBlob = gl::Texture(fromOcv(mMatDiff));
	}
	
	if (mIsDebug)
		mTexBase = gl::Texture(fromOcv(mMatCurrent));
}

void DS4ParticlesApp::updatePointCloud()
{
	mColorShift = math<float>::abs(math<float>::sin(getElapsedFrames()*0.005f));

	// Lightning Bolts
	if (mContours.size() > 0)
	{
		mContourPoints.clear();
		for (auto cit = mContoursKeep.begin(); cit != mContoursKeep.end(); ++cit)
		{
			for (auto vit = cit->begin(); vit != cit->end(); ++vit)
			{
				int cX = vit->x;
				int cY = vit->y;
				uint16_t cZ = mDepthBuffer[cY*S_DEPTH_SIZE.x + cX];
				if (cZ > mDepthMin&&cZ < mDepthMax)
				{
					float cInPoint[] = { static_cast<float>(cX), static_cast<float>(cY), cZ }, cOutPoint[3];
					DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
					mContourPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
				}
			}
		}
	}

	//PointCloud
	mCloudPoints.clear();
	for (int dY = 0; dY < S_DEPTH_SIZE.y; dY+=4)
	{
		for (int dX = 0; dX < S_DEPTH_SIZE.x; dX+=4)
		{
			if (mMatCurrent.at<uint8_t>(dY, dX) > 0)
			{
				float cZ = (float)mDepthBuffer[dY*S_DEPTH_SIZE.x+dX];
				if (cZ>mDepthMin&&cZ < mDepthMax)
				{
					float cInPoint[] = { static_cast<float>(dX), static_cast<float>(dY), cZ }, cOutPoint[3];
					DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
					mCloudPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
				}
			}
		}
	}

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
		gl::color(Color(0, 1, 0));
		gl::begin(GL_POINTS);
		glPointSize(2.0);

		for (auto cit = mContours.begin(); cit != mContours.end(); ++cit)
		{
			for (auto vit = cit->begin(); vit != cit->end(); ++vit)
			{
				gl::vertex(vit->x, vit->y);
			}
		}
		gl::end();
		gl::popMatrices();
	}

	if (mHulls.size() > 0)
	{
		gl::pushMatrices();
		gl::translate(Vec2f(S_APP_SIZE.x / 2, 0));
		gl::scale(Vec2f((S_APP_SIZE.x / (float)S_DEPTH_SIZE.x)*0.5f, (S_APP_SIZE.y / (float)S_DEPTH_SIZE.y)*0.5f));
		gl::color(Color(1, 1, 0));
		
		for (auto cHull : mHulls)
		{

			gl::begin(GL_LINE_LOOP);
			for (auto cPt : cHull)
				gl::vertex(cPt.x, cPt.y);
			gl::end();
		}

		gl::color(Color(1, 0, 1));
		for (auto cContour : mContoursKeep)
		{
			gl::begin(GL_LINE_LOOP);
			for (auto cPt : cContour)
				gl::vertex(cPt.x, cPt.y);
			gl::end();
		}
		gl::popMatrices();
	}
	mGUI->draw();
}

void DS4ParticlesApp::drawRunning()
{
	gl::clear(Color::black());
	gl::setMatrices(mMayaCam.getCamera());

	gl::pushMatrices();
	gl::rotate(mArcball.getQuat());

	gl::enableAdditiveBlending();
	gl::enable(GL_POINT_SIZE);

	//Point Cloud
	glPointSize(2.0f);
	gl::begin(GL_POINTS);
	gl::color(Color(mColorShift, 1-mColorShift, 1));


	for (auto pit = mCloudPoints.begin(); pit != mCloudPoints.end(); ++pit)
	{
		gl::vertex(*pit);
	}
	gl::end();

	//Lightning Bolts
	glPointSize(4.0f);
	gl::begin(GL_POINTS);
	gl::color(ColorA(1, 1 - mColorShift, mColorShift, 0.25f));
	for (auto pit2 = mContourPoints.begin(); pit2 != mContourPoints.end(); ++pit2)
	{
		gl::vertex(*pit2);
	}
	gl::end();

	//Particles
	glPointSize(1.0f);
	mParticleSystem.display(Color(1, 1 - mColorShift, mColorShift));
	gl::popMatrices();
}
#pragma endregion Draw

void DS4ParticlesApp::shutdown()
{
	mDSAPI->stopCapture();
}

CINDER_APP_NATIVE( DS4ParticlesApp, RendererGl )
