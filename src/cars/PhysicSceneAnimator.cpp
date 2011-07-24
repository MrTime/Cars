#include "PhysicSceneAnimator.h"
#include "World.h"

#include <irrlicht.h>

using namespace irr;

CPhysicSceneAnimator::CPhysicSceneAnimator(CWorld *world) : m_world(world)
{
}


CPhysicSceneAnimator::~CPhysicSceneAnimator(void)
{
}

void CPhysicSceneAnimator::animateNode(irr::scene::ISceneNode* node, irr::u32 timeMs)
{
	m_world->animate((float)timeMs * 0.00001f);
}

scene::ISceneNodeAnimator* CPhysicSceneAnimator::createClone(irr::scene::ISceneNode* node,irr::scene::ISceneManager* newManager)
{
	return (new CPhysicSceneAnimator(m_world));
}