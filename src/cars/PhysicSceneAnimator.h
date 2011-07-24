#pragma once
#include <iscenenodeanimator.h>
#include <ode/ode.h>

class CWorld;

/**
*	Perform computing physics
*/
class CPhysicSceneAnimator :
	public irr::scene::ISceneNodeAnimator
{
private:
	CWorld *m_world;

public:
	CPhysicSceneAnimator(CWorld *world);
	virtual ~CPhysicSceneAnimator(void);

	//! Animates a scene node.
	/** \param node Node to animate.
	\param timeMs Current time in milli seconds. */
	virtual void animateNode(irr::scene::ISceneNode* node, irr::u32 timeMs);

	//! Creates a clone of this animator.
	/** Please note that you will have to drop
	(IReferenceCounted::drop()) the returned pointer after calling this. */
	virtual ISceneNodeAnimator* createClone(irr::scene::ISceneNode* node,
			irr::scene::ISceneManager* newManager=0);
};

