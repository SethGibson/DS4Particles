#include "DS4Particle.h"
#include "cinder/gl/gl.h"

#pragma region DS4Particle
DS4Particle::DS4Particle()
{

}

DS4Particle::DS4Particle(Vec3f pPos, Vec3f pVel, int pAge, float pMaxAlpha, bool pIsMica) : PPosition(pPos), PVelocity(pVel), IsActive(true), mAge(pAge), mLife(pAge), IsMica(pIsMica)
{
	float cAlpha = randFloat(0.1f, pMaxAlpha);
	mStartColor = ColorA(222 / 255.0f, 190 / 255.0f, 131 / 255.0f, cAlpha);
	mEndColor = ColorA(138 / 255.0f, 109 / 255.0f, 78 / 255.0f, cAlpha*0.1f);
	if (pIsMica)
	{
		mStartColor = ColorA(1, 1, 1, 1);
		mEndColor = ColorA(0, 0, 0, 0.25f);
	}
	PColor = mStartColor;
}

void DS4Particle::step()
{
	mAge -= 1;
	if (mAge <= 0)
		IsActive = false;
	if (IsActive)
	{
		PPosition += PVelocity;
		PVelocity *= 1.0001f;
		
		float cLife = mAge / static_cast<float>(mLife);
		PColor = mEndColor.lerp(cLife, mStartColor);
	}
}
#pragma endregion DS4Particle

#pragma region DS4ParticleSystem
DS4ParticleSystem::DS4ParticleSystem()
{

}

DS4ParticleSystem::~DS4ParticleSystem()
{

}

void DS4ParticleSystem::step()
{
	for (auto pit = mParticles.begin(); pit != mParticles.end();)
	{
		if (!pit->IsActive)
			pit = mParticles.erase(pit);
		else
		{
			pit->step();
			++pit;
		}
	}
}

void DS4ParticleSystem::display()
{
	gl::begin(GL_POINTS);
	for (auto p : mParticles)
	{
		gl::color(p.PColor);
		gl::vertex(p.PPosition);
	}
	gl::end();
}

void DS4ParticleSystem::add(Vec3f pPos, Vec3f pVel, Vec2i pAge, float pAlpha, bool pIsMica)
{
	int cAge = randInt(pAge.x, pAge.y);
	mParticles.push_back(DS4Particle(pPos, pVel, cAge, pAlpha, pIsMica));
}

void DS4ParticleSystem::add(DS4Particle pParticle)
{
	mParticles.push_back(pParticle);
}
#pragma endregion DS4ParticleSystem

