#include "DS4Particle.h"

#pragma region DS4Particle
DS4Particle::DS4Particle()
{

}

DS4Particle::DS4Particle(Vec3f pPos, Vec3f pVel, Color pCol) : PPosition(pPos), PVelocity(pVel), PColor(pCol), IsActive(true)
{
	mAge = randInt(60, 180);
}

DS4Particle::~DS4Particle()
{

}


void DS4Particle::step()
{
	--mAge;
	if (mAge <= 0)
		IsActive = false;
	if (IsActive)
	{
		PPosition += PVelocity;
		PVelocity *= 0.99f;
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

}

void DS4ParticleSystem::display()
{

}

void DS4ParticleSystem::add(DS4Particle pParticle)
{
	mParticles.push_back(pParticle);
}
#pragma endregion DS4ParticleSystem

