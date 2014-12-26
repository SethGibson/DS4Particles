#include <boost/filesystem.hpp>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include "DS4ParticlesApp.h"
#include "DSAPIUtil.h"

namespace bfs = boost::filesystem;

static Vec2i S_DEPTH_SIZE(480, 360);
static Vec2i S_APP_SIZE(1280, 800);
static size_t S_MAX_PARTICLES = 5000;
static Vec2i S_LOGO_SIZE(192, 48);

#pragma region Cinder Loop
void DS4ParticlesApp::prepareSettings(Settings *pSettings)
{
	pSettings->setWindowSize(S_APP_SIZE.x, S_APP_SIZE.y);
	pSettings->setWindowPos(20, 40);
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
	setupAudio();
}

void DS4ParticlesApp::update()
{
	if (mDSAPI->isZEnabled())
	{
		if (mDSAPI->grab())
		{
			mDepthBuffer = mDSAPI->getZImage();
			updateAudio();
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
}

void DS4ParticlesApp::keyDown(KeyEvent pEvent)
{
	switch (pEvent.getChar())
	{
	case 'd':
		mIsDebug = !mIsDebug;
		break;
	case 'b':
		mDrawBackground = !mDrawBackground;
		break;
	case 'l':
		mDrawLogo = !mDrawLogo;
		break;
	case 'c':
	{
		int cColorMode = static_cast<int>(mColorMode);
		cColorMode = (cColorMode + 1) % NUM_COLORMODES;
		mColorMode = static_cast<DS4PColorMode>(cColorMode);
		break;
	}
	}
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
#pragma endregion Cinder Loop

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

	//mBackground = gl::Texture(loadImage(loadAsset("background.png")));
	mBackground = gl::Texture(loadImage(loadAsset("TEST_bg.jpg")));
	mLogo = gl::Texture(loadImage(loadAsset("rs_badge.png")));
	mDrawBackground = false;
	mDrawLogo = false;
}

void DS4ParticlesApp::setupGUI()
{
	bfs::path cConfigFile = getAssetPath("particle_config.cfg");
	if (bfs::exists(cConfigFile))
	{
		readConfig();
	}
	else
	{
		mDepthMin = 0;
		mDepthMax = 2000;
		mThresh = 128;
		mSizeMin = 250;

		mCloudRes = 2; //Cloud Resolution
		mSpawnRes = 4; //Spawn Resolution
		mBoltRes = 2;  //Bolt Resolution

		mPointSize = 2.0f;		//Point Size
		mBoltWidth = 2.0f;		//Bolt Width

		mNumParticles = 5000;
		mParticleSize = 2.0f;	//Particle Size
		mAgeMin = 30;			//Min Particle Age
		mAgeMax = 120;			//Max Particle Age
		mFramesSpawn = 5;
		mBoltWidthScale = 2.0f;
		mBoltAlphaScale = 2.0f;
	}
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
	mGUI->addParam("Bolt Scale", &mBoltWidthScale);
	mGUI->addParam("Bolt Brightness", &mBoltAlphaScale);
	mGUI->addSeparator();
	mGUI->addText("Particle Params");
	mGUI->addParam("Particle Count", &mNumParticles);
	mGUI->addParam("Particle Size", &mParticleSize);
	mGUI->addParam("Min Age", &mAgeMin);
	mGUI->addParam("Max Age", &mAgeMax);
	mGUI->addParam("Spawn Rate", &mFramesSpawn);
	mGUI->addParam("Avg Framerate", &mFPS);
	mGUI->addButton("Save Settings", std::bind(&DS4ParticlesApp::writeConfig, this));
	mXDDLogo = gl::Texture(loadImage(loadAsset("xDDBadge.png")));
	mCinderLogo = gl::Texture(loadImage(loadAsset("cinderBadge.png")));
}

void DS4ParticlesApp::setupAudio()
{
	auto cAudioCtx = audio::Context::master();
	mInputDeviceNode = cAudioCtx->createInputDeviceNode();

	auto cMonitorFormat = audio::MonitorSpectralNode::Format().fftSize(2048).windowSize(1024);
	mMonitorSpectralNode = cAudioCtx->makeNode(new audio::MonitorSpectralNode(cMonitorFormat));

	mInputDeviceNode >> mMonitorSpectralNode;
	mInputDeviceNode->enable();
	cAudioCtx->enable();

	mMagMean = 0;
}
void DS4ParticlesApp::setupColors()
{
	mIntelBlue = Color::hex(0x0071c5);
	mIntelPaleBlue = Color::hex(0x7ed3f7);
	mIntelLightBlue = Color::hex(0x00aeef);
	mIntelDarkBlue = Color::hex(0x004280);
	mIntelYellow = Color::hex(0xffda00);
	mIntelOrange = Color::hex(0xfdb813);
	mIntelGreen = Color::hex(0xa6ce39);

	mColorMode = COLOR_MODE_BLUE;
}

void DS4ParticlesApp::readConfig()
{
	ifstream cConfigFile(getAssetPath("particle_config.cfg").c_str());
	bpo::options_description cDesc("Configuration");
	bpo::variables_map cConfigVars = bpo::variables_map();

	cDesc.add_options()
		("min_depth", bpo::value<int>(), "Min Depth")
		("max_depth", bpo::value<int>(), "Max Depth")
		("threshold", bpo::value<double>(), "Threshold")
		("min_poly_area", bpo::value<double>(), "Min Poly Area")
		("cloud_res", bpo::value<int>(), "Cloud Res")
		("bolt_res", bpo::value<int>(), "Bolt Res")
		("spawner_res", bpo::value<int>(), "Spawner Res")
		("point_size", bpo::value<float>(), "Point Size")
		("bolt_width", bpo::value<float>(), "Bolt Width")
		("bolt_scale", bpo::value<float>(), "Bolt Scale")
		("bolt_alpha", bpo::value<float>(), "Bolt Brightness")
		("particle_count", bpo::value<size_t>(), "Particle Count")
		("particle_size", bpo::value<float>(), "Particle Size")
		("min_age", bpo::value<int>(), "Min Age")
		("max_age", bpo::value<int>(), "Max Age")
		("spawn_rate", bpo::value<int>(), "Spawn Rate")
	;

	try
	{
		bpo::store(bpo::parse_config_file(cConfigFile, cDesc), cConfigVars);
		bpo::notify(cConfigVars);
		
		if (cConfigVars.count("min_depth"))
			mDepthMin = cConfigVars["min_depth"].as<int>();
		else
			mDepthMin = 0;
		if (cConfigVars.count("max_depth"))
			mDepthMax = cConfigVars["max_depth"].as<int>();
		else
			mDepthMax = 5000;
		if (cConfigVars.count("threshold"))
			mThresh = cConfigVars["threshold"].as<double>();
		else
			mThresh = 128;
		if (cConfigVars.count("min_poly_area"))
			mSizeMin = cConfigVars["min_poly_area"].as<double>();
		else
			mSizeMin = 250;
		if (cConfigVars.count("cloud_res"))
			mCloudRes = cConfigVars["cloud_res"].as<int>();
		else

			mCloudRes = 2;
		if (cConfigVars.count("bolt_res"))
			mBoltRes = cConfigVars["bolt_res"].as<int>();
		else
			mBoltRes = 2;
		if (cConfigVars.count("spawner_res"))
			mSpawnRes = cConfigVars["spawner_res"].as<int>();
		else
			mSpawnRes = 4;
		if (cConfigVars.count("point_size"))
			mPointSize = cConfigVars["point_size"].as<float>();
		else
			mPointSize = 2.0f;
		if (cConfigVars.count("bolt_width"))
			mBoltWidth= cConfigVars["bolt_width"].as<float>();
		else
			mBoltWidth = 2.0f;
		if (cConfigVars.count("bolt_scale"))
			mBoltWidthScale = cConfigVars["bolt_scale"].as<float>();
		else
			mBoltWidthScale = 2.0f;
		if (cConfigVars.count("bolt_alpha"))
			mBoltAlphaScale = cConfigVars["bolt_alpha"].as<float>();
		else
			mBoltAlphaScale = 2.0f;
		if (cConfigVars.count("particle_count"))
			mNumParticles = cConfigVars["particle_count"].as<size_t>();
		else
			mNumParticles = 5000;
		if (cConfigVars.count("particle_size"))
			mParticleSize = cConfigVars["particle_size"].as<float>();
		else
			mParticleSize = 2.0f;
		if (cConfigVars.count("min_age"))
			mAgeMin = cConfigVars["min_age"].as<int>();
		else
			mAgeMin = 30;
		if (cConfigVars.count("max_age"))
			mAgeMax = cConfigVars["max_age"].as<int>();
		else
			mAgeMax = 120;
		if (cConfigVars.count("spawn_rate"))
			mFramesSpawn = cConfigVars["spawn_rate"].as<int>();
		else
			mFramesSpawn = 5;
	}
	catch (bpo::required_option &e)
	{
		console() << "Error parsing config file: " << e.what() << endl;
		quit();
	}
	catch (bpo::error &e)
	{
		console() << "Error parsing config file: " << e.what() << endl;
		quit();
	}
}

void DS4ParticlesApp::writeConfig()
{
	ofstream cOutFile;
	cOutFile.open(getAssetPath("particle_config.cfg").c_str());

	cOutFile << "min_depth=" << to_string(mDepthMin) << endl;
	cOutFile << "max_depth=" << to_string(mDepthMax) << endl;
	cOutFile << "threshold=" << to_string(mThresh) << endl;
	cOutFile << "min_poly_area=" << to_string(mSizeMin) << endl;
	cOutFile << "cloud_res=" << to_string(mCloudRes) << endl;
	cOutFile << "bolt_res=" << to_string(mBoltRes) << endl;
	cOutFile << "spawner_res=" << to_string(mSpawnRes) << endl;
	cOutFile << "point_size=" << to_string(mPointSize) << endl;
	cOutFile << "bolt_width=" << to_string(mBoltWidth) << endl;
	cOutFile << "bolt_scale=" << to_string(mBoltWidthScale) << endl;
	cOutFile << "bolt_alpha=" << to_string(mBoltAlphaScale) << endl;
	cOutFile << "particle_count=" << to_string(mNumParticles) << endl;
	cOutFile << "particle_size=" << to_string(mParticleSize) << endl;
	cOutFile << "min_age=" << to_string(mAgeMin) << endl;
	cOutFile << "max_age=" << to_string(mAgeMax) << endl;
	cOutFile << "spawn_rate=" << to_string(mFramesSpawn) << endl;

	cOutFile.close();
}
#pragma endregion Setup

#pragma region Update

void DS4ParticlesApp::updateAudio()
{
	mMagSpectrum = mMonitorSpectralNode->getMagSpectrum();
	auto cFFTBounds = std::minmax(mMagSpectrum.begin(), mMagSpectrum.end());

	float cSum = 0.0f;
	float cMax = -1.0f;
	float cMin = 10000.0f;
	for (auto cV : mMagSpectrum)
	{
		if (cV > cMax)
			cMax = cV;
		else if (cV < cMin)
			cMin = cV;
		cSum += cV;
	}

	cSum /= (float)mMagSpectrum.size();
	cMin = 0;
	cMax = .00001f;
	mMagMean = lmap<float>(cSum, cMin, cMax, 0, 1);
}

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

	if (!mIsDebug)
	{
		for (int dy = 0; dy < S_DEPTH_SIZE.y; dy++)
		{
			for (int dx = 0; dx < S_DEPTH_SIZE.x; dx++)
			{
				float cDepthVal = (float)mDepthBuffer[dy*S_DEPTH_SIZE.x + dx];
				float cInPoint[] = { static_cast<float>(dx), static_cast<float>(dy), cDepthVal }, cOutPoint[3];
				if ((cDepthVal>mDepthMin&&cDepthVal < mDepthMax) && mMatCurrent.at<uint8_t>(dy, dx)>128)
				{
					DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
					if (dx % mCloudRes == 0 && dy%mCloudRes == 0)
						mCloudPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
					else if (dy >= S_DEPTH_SIZE.y - 2 && dx%mCloudRes == 0)
						mBorderPoints.push_back(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]));
				}
			}
		}
	}

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
				if (!mIsDebug && (vi % mSpawnRes == 0) && (getElapsedFrames() % mFramesSpawn == 0))
				{
					uint16_t cZ = mPrevDepthBuffer[cPoint.y*S_DEPTH_SIZE.x + cPoint.x];
					if (cZ>mDepthMin&&cZ < mDepthMax)
					{
						float cInPoint[] = { static_cast<float>(cPoint.x), static_cast<float>(cPoint.y), cZ }, cOutPoint[3];
						DSTransformFromZImageToZCamera(mZIntrinsics, cInPoint, cOutPoint);
						if (mParticleSystem.count() < mNumParticles&&cOutPoint[1] < 50)
							mParticleSystem.add(Vec3f(cOutPoint[0], -cOutPoint[1], cOutPoint[2]), Vec3f(randFloat(-0.15f, 0.15f), randFloat(-2, -6), randFloat(0, -1)), Vec2f(mAgeMin, mAgeMax));
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
	gl::setMatricesWindow(getWindowWidth(), getWindowHeight());
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
		gl::color(mIntelGreen);
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
		gl::color(mIntelYellow);
		
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

	gl::enableAlphaBlending();
	gl::color(ColorA::white());
	float cXDDY = getWindowHeight() - 10 - mXDDLogo.getHeight();
	gl::draw(mXDDLogo, Vec2i(0, cXDDY));

	float cCiX = getWindowWidth() - mCinderLogo.getWidth();
	float cCiY = getWindowHeight() - 5 - mCinderLogo.getHeight();
	gl::draw(mCinderLogo, Vec2i(cCiX, cCiY));

	mGUI->draw();
	gl::disableAlphaBlending();
}

void DS4ParticlesApp::drawRunning()
{
	gl::clear(Color::black());
	gl::color(Color::white());
	if (mDrawBackground)
	{
		gl::setMatricesWindow(getWindowSize());
		//gl::draw(mBackground, Vec2i::zero());
		gl::draw(mBackground, Rectf(0,0,S_APP_SIZE.x, S_APP_SIZE.y));
	}

	gl::setMatrices(mMayaCam.getCamera());
	gl::pushMatrices();
	gl::rotate(mArcball.getQuat());
	gl::scale(-1, 1, 1);
	gl::enableAdditiveBlending();
	gl::enable(GL_POINT_SIZE);

	//Point Cloud
	if (mColorMode==COLOR_MODE_BLUE)
		gl::color(mIntelDarkBlue);
	else if (mColorMode == COLOR_MODE_GOLD)
		gl::color(mIntelOrange);

	glPointSize(mPointSize);
	gl::begin(GL_POINTS);

	for (auto pit : mCloudPoints)
	{
		gl::vertex(pit);
	}
	gl::end();

	//Lightning Bolts
	glPointSize(mBoltWidth*mMagMean*mBoltWidthScale);
	if (mColorMode == COLOR_MODE_BLUE)
		gl::color(ColorA(mIntelPaleBlue.r, mIntelPaleBlue.g, mIntelPaleBlue.b, mMagMean*mBoltAlphaScale));
	else if (mColorMode == COLOR_MODE_GOLD)
		gl::color(ColorA(mIntelYellow.r, mIntelYellow.g, mIntelYellow.b, mMagMean*mBoltAlphaScale));
	gl::begin(GL_POINTS);
	for (auto pit2 : mContourPoints)
	{
		gl::vertex(pit2);
	}
	gl::end();

	//glPointSize(mBoltWidth);
	if (mColorMode == COLOR_MODE_BLUE)
		gl::color(ColorA(mIntelPaleBlue.r, mIntelPaleBlue.g, mIntelPaleBlue.b, 0.75f));
	else if (mColorMode == COLOR_MODE_GOLD)
		gl::color(ColorA(mIntelOrange.r, mIntelOrange.g, mIntelOrange.b, 0.75f));
	gl::begin(GL_POINTS);
	for (auto bpit : mBorderPoints)
	{
		gl::vertex(bpit);
	}
	gl::end();

	//Particles
	glPointSize(mParticleSize);
	if (mColorMode == COLOR_MODE_BLUE)
		gl::color(ColorA(mIntelBlue.r, mIntelBlue.g, mIntelBlue.b, 0.75f));
	else if (mColorMode == COLOR_MODE_GOLD)
		gl::color(ColorA(mIntelYellow.r, mIntelYellow.g, mIntelYellow.b, 0.75f));
	mParticleSystem.display();
	gl::popMatrices();

	if (mCamInfo)
		drawCamInfo();

	//Logo
	if (mDrawLogo)
	{
		gl::setMatricesWindow(getWindowWidth(), getWindowHeight());
		float cX = getWindowWidth() - S_LOGO_SIZE.x - 10;
		float cY = getWindowHeight() - S_LOGO_SIZE.y - 10;
		gl::color(Color::white());
		gl::draw(mLogo, Rectf(cX, cY, cX+S_LOGO_SIZE.x, cY+S_LOGO_SIZE.y));
	}
	gl::disableAlphaBlending();
	gl::disable(GL_POINT_SIZE);
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
