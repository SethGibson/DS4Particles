#ifndef DS4_PARTICLE_H
#define DS4_PARTICLE_H

#include "cinder/Vector.h"
#include "cinder/Color.h"

using namespace ci;
using namespace std;
class DS4Particle
{
public:
	DS4Particle();
	~DS4Particle();

	void step();
	Vec3f PPosition;
	Vec3f PVelocity;
	Color PColor;

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

private:
	vector<DS4Particle> mParticles;

};
#endif