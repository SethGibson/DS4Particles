#include <cstring>
#include <cstdlib>
#include "DS4ParticlesApp.h"
#include "DSAPIUtil.h"

static Vec2i S_DEPTH_SIZE(480, 360);
static Vec2i S_APP_SIZE(1280, 800);
void DS4ParticlesApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(S_APP_SIZE.x, S_APP_SIZE.y);
	pSettings->setFrameRate(60);
	//pSettings->setFullScreen(true);
}

void DS4ParticlesApp::setup()
{
	mIsDebug = true;
	if (!setupDSAPI())
		console() << "Error Starting DSAPI Session" << endl;

	setupCVandGL();
	setupGUI();
}

void DS4ParticlesApp::mouseDown( MouseEvent event )
{
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
			updateParticles();
		}
	}
	mFPS = getAverageFps();
}

void DS4ParticlesApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 

	if (mIsDebug)
		drawDebug();
	else
		drawRunning();
}

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

void DS4ParticlesApp::setupCVandGL()
{
	mDepthPixels = new uint8_t[S_DEPTH_SIZE.x*S_DEPTH_SIZE.y]{0};
	mMatCurrent = cv::Mat::zeros(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1));
	mMatPrev = cv::Mat::zeros(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1));
}

void DS4ParticlesApp::setupGUI()
{
	mDepthMin = 0;
	mDepthMax = 2000;
	mSizeMin = 50;
	mThresh = 128;
	mGUI = params::InterfaceGl::create("Config", Vec2i(200, 200));
	mGUI->addParam("Min Depth", &mDepthMin);
	mGUI->addParam("Max Depth", &mDepthMax);
	mGUI->addParam("Threshold", &mThresh);
	mGUI->addParam("Min Blob Size", &mSizeMin);
	mGUI->addParam("Avg Framerate", &mFPS);
}

void DS4ParticlesApp::updateTextures()
{
	int did = 0;

	while (did<(S_DEPTH_SIZE.x*S_DEPTH_SIZE.y))
	{
		float cDepthVal = (float)mDepthBuffer[did];
		if (cDepthVal>mDepthMin&&cDepthVal<mDepthMax)
			mDepthPixels[did] = (uint8_t)(lmap<float>(cDepthVal, mDepthMin, mDepthMax, 255, 0));
		else
			mDepthPixels[did] = 0;
		did += 1;
	}

	//run cv opps
	//get cv mats
	mMatCurrent = cv::Mat(S_DEPTH_SIZE.y, S_DEPTH_SIZE.x, CV_8U(1), mDepthPixels);
	cv::threshold(mMatCurrent, mMatCurrent, mThresh, 255, CV_THRESH_BINARY);
	cv::absdiff(mMatPrev, mMatCurrent, mMatDiff);
	
	//diff contours gives us a shorter list to walk
	mContours.clear();
	cv::findContours(mMatDiff, mContours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

	if (mIsDebug)
	{
		mTexBase = gl::Texture(fromOcv(mMatCurrent));
		mTexBlob = gl::Texture(fromOcv(mMatDiff));
	}

	mMatCurrent.copyTo(mMatPrev);
}

void DS4ParticlesApp::updatePointCloud()
{
	//get contour points
	if (mContours.size() > 0)
	{
		mContourPoints.clear();
		for (auto cit = mContours.begin(); cit != mContours.end(); ++cit)
		{
			for (auto vit = cit->begin(); vit != cit->end(); ++vit)
			{
				int cX = vit->x;
				int cY = vit->y;
				uint16_t cZ = mDepthBuffer[cY*S_DEPTH_SIZE.x + cX];
				float cInPoint[] = { static_cast<float>(cX), static_cast<float>(cY), cZ }, cOutPoint[3];
				DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
				mContourPoints.push_back(Vec3f(cOutPoint[0], cOutPoint[1], cOutPoint[2]));
			}
		}
	}

	//get cloud points
	mCloudPoints.clear();
	int did = 0;
	for (int dY = 0; dY < S_DEPTH_SIZE.y; ++dY)
	{
		for (int dX = 0; dX < S_DEPTH_SIZE.x; ++dX)
		{
			float cVal = (float)*(mDepthBuffer + did);
			if (cVal>mDepthMin&&cVal < mDepthMax)
			{
				float cInPoint[] = { static_cast<float>(dX), static_cast<float>(dY), cVal }, cOutPoint[3];
				DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
				mCloudPoints.push_back(Vec3f(cOutPoint[0], cOutPoint[1], cOutPoint[2]));
			}
		}
	}
}

void DS4ParticlesApp::updateParticles()
{

}

void DS4ParticlesApp::drawDebug()
{
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

		vector<cv::Point> cC = mContours[0];
		
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
	mGUI->draw();
}

void DS4ParticlesApp::drawRunning()
{

}

void DS4ParticlesApp::shutdown()
{
	mDSAPI->stopCapture();
}

CINDER_APP_NATIVE( DS4ParticlesApp, RendererGl )
