#include "DS4Particle.h"
#include "cinder/gl/gl.h"

#pragma region DS4Particle
DS4Particle::DS4Particle()
{

}

DS4Particle::DS4Particle(Vec3f pPos, Vec3f pVel) : PPosition(pPos), PVelocity(pVel), IsActive(true)
{
	mAge = randInt(60, 180);
}

DS4Particle::~DS4Particle()
{

}


void DS4Particle::step()
{
	mAge -= 1;
	if (mAge <= 0)
		IsActive = false;
	if (IsActive)
	{
		PPosition += PVelocity;
		if (PPosition.y <= -280)
			PVelocity *= Vec3f(0.5f, 0, 0.5f);
		else
			PVelocity *= 0.998f;
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

void DS4ParticleSystem::display(Color pColor)
{
	gl::begin(GL_POINTS);
	gl::color(ColorA(pColor.r, pColor.g, pColor.b, 1.0f));
	glPointSize(2.0f);
	for (auto p : mParticles)
	{
		gl::vertex(p.PPosition);
	}
	gl::end();
}

void DS4ParticleSystem::add(Vec3f pPos, Vec3f pVel)
{
	mParticles.push_back(DS4Particle(pPos, pVel));
}

void DS4ParticleSystem::add(DS4Particle pParticle)
{
	mParticles.push_back(pParticle);
}
#pragma endregion DS4ParticleSystem

