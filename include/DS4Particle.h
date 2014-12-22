#ifndef DS4_PARTICLE_H
#define DS4_PARTICLE_H

#include "cinder/Vector.h"
#include "cinder/Color.h"
#include "cinder/Rand.h"

using namespace ci;
using namespace std;
class DS4Particle
{
public:
	DS4Particle();
	DS4Particle(Vec3f pPos, Vec3f pVel, int pAge);
	~DS4Particle();

	void step();

	bool IsActive;
	Vec3f PPosition;
	Vec3f PVelocity;

private:
	int mAge;
};

class DS4ParticleSystem
{
public:
	DS4ParticleSystem();
	~DS4ParticleSystem();

	void step();
	void display();
	void add(Vec3f pPos, Vec3f pVel, Vec2i pAge);
	void add(DS4Particle pParticle);
	inline size_t count() { return mParticles.size(); }

private:
	vector<DS4Particle> mParticles;

};
#endif