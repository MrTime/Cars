#include "Car.h"
#include "World.h"

#include "PhysicAnimator.h"

using namespace irr;
using namespace irr::io;
using namespace irr::core;

CCar::CCar(CWorld *world) 
	: m_world(world), m_chassis(0), m_glass(0), m_damaged(false), m_speed(0.0f), m_steer(0.0f)
{
	memset(m_wheel, 0, sizeof(irr::scene::IMeshSceneNode*)*4);

	p.car_space = dSimpleSpaceCreate(world->getPhysicSpace());
	dSpaceSetCleanup(p.car_space, 0);
}

CCar::~CCar(void)
{
	// clean ODE resources
	dGeomDestroy(p.chassis_geom);
	for (int i = 0; i < 4; i++)
		dGeomDestroy(p.wheel_geom[i]);

	// remove from world car list

}

void CCar::setChassis(irr::scene::IMesh * mesh, irr::video::ITexture * map, float mass)
{
	scene::ISceneManager * smgr = m_world->getSceneManager();
	
	// save clean mesh
	g.clean_model = mesh;
	g.clean_chassis_texture = map;

	// create scene node
	m_chassis = smgr->addMeshSceneNode(mesh, smgr->getRootSceneNode());
	m_chassis->setMaterialTexture(0, map);

	// create physic model
	dMass m;
	
	// bounding box sizes
	const irr::core::aabbox3df &aabb = mesh->getBoundingBox();
	float car_width = aabb.MaxEdge.X - aabb.MinEdge.X,
     	  car_height = aabb.MaxEdge.Y - aabb.MinEdge.Y,
		  car_lenght = aabb.MaxEdge.Z - aabb.MinEdge.Z;
					
	// create ODE stuff
	p.chassis_body = dBodyCreate(m_world->getPhysicWorld());
	dBodySetPosition(p.chassis_body, 0.0f, 0.0f, 0.0f);
	dMassSetBox(&m, 1, car_width, car_height, car_lenght);
	dMassAdjust(&m, mass);
	dBodySetMass(p.chassis_body, &m);

	p.chassis_geom = dCreateBox(0, car_width, car_height, car_lenght);
	dGeomSetBody(p.chassis_geom, p.chassis_body);

	// add geom into space
	dSpaceAdd(p.car_space, p.chassis_geom);

	// attach animator
	CPhysicAnimator * animator = new CPhysicAnimator(p.chassis_geom);
	m_chassis->addAnimator(animator);
	animator->drop();
}

void CCar::setGlass(irr::scene::IMesh * mesh, irr::video::ITexture * map)
{
	scene::ISceneManager * smgr = m_world->getSceneManager();
	
	// save clean model
	g.clean_glass = mesh;
	g.clean_glass_texture = map;

	// create scene node
	m_glass = smgr->addMeshSceneNode(mesh, m_chassis);
	m_glass->setMaterialTexture(0, g.getGlassTexture());
	m_glass->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
}

void CCar::addWheel(EWheel id, const irr::core::vector3df &position, float mass)
{
	scene::ISceneManager * smgr = m_world->getSceneManager();

	// create scene node
	m_wheel[id] = smgr->addMeshSceneNode(g.wheel, smgr->getRootSceneNode(), -1, position);
	m_wheel[id]->setMaterialTexture(0, g.wheel_texture);

	// create physic stuff
	dMass m;

	// compute radius from mesh
	float wheel_radius = abs(g.wheel->getBoundingBox().MaxEdge.Y);

	// create body
	dBodyID wheel_body = p.wheel_body[id] = dBodyCreate(m_world->getPhysicWorld());
	dQuaternion q;
	dQFromAxisAndAngle(q, 1, 0, 0, M_PI*0.5);
	dBodySetQuaternion(wheel_body, q);
		
	dMassSetSphere(&m, 1, wheel_radius);
	dMassAdjust(&m, mass);
		
	dBodySetMass(wheel_body, &m);
		
	// create sphere geom
	dGeomID wheel_geom = p.wheel_geom[id] = dCreateSphere(0, wheel_radius);
	dGeomSetBody(wheel_geom, wheel_body);
	
	// TODO: set body global position
	dBodySetPosition(wheel_body, position.X, position.Y, position.Z);

	// create joint
	dJointID joint = p.joint[id] = dJointCreateHinge2(m_world->getPhysicWorld(), 0);
	dJointAttach(joint, p.chassis_body, wheel_body);
	const dReal *a = dBodyGetPosition(wheel_body);
	dJointSetHinge2Anchor(joint, a[0], a[1], a[2]);
	dJointSetHinge2Axis1(joint, 0,1,0);
	dJointSetHinge2Axis2(joint, 1,0,0);
	
	// set joint suspension
	dJointSetHinge2Param(joint, dParamSuspensionERP, 0.4f);
	dJointSetHinge2Param(joint, dParamSuspensionCFM, 0.8f);
	
	dJointSetHinge2Param(joint, dParamLoStop, 0.0f);
	dJointSetHinge2Param(joint, dParamHiStop, 0.0f);

	// add geom into space
	dSpaceAdd(p.car_space, wheel_geom);

	// attach animator
	CPhysicAnimator * animator = new CPhysicAnimator(wheel_geom);
	m_wheel[id]->addAnimator(animator);
	animator->drop();
}

void CCar::animate()
{
	for (int i = 0; i < 2; i++)
	{
		// motor
		dJointSetHinge2Param (p.joint[i],dParamVel2, m_speed);
		dJointSetHinge2Param (p.joint[i],dParamFMax2, 0.1f);
	}
	for (int i = 0; i < 2; i++)
	{
		// steering
		dReal v = m_steer - dJointGetHinge2Angle1 (p.joint[i]);
		if (v > 0.1f) v = 0.1f;
		if (v < -0.1f) v = -0.1f;	
		v *= 10.0;
		dJointSetHinge2Param (p.joint[i],dParamVel,v);
		dJointSetHinge2Param (p.joint[i],dParamFMax,0.2f);
		dJointSetHinge2Param (p.joint[i],dParamLoStop,-0.75f);
		dJointSetHinge2Param (p.joint[i],dParamHiStop,0.75f);
		dJointSetHinge2Param (p.joint[i],dParamFudgeFactor,0.1f);
	}
}

void CCar::setPosition (const irr::core::vector3df &newpos) { 
	assert(m_chassis != NULL);
	
	//matrix4 parentInvTransform;
	//m_chassis->getAbsoluteTransformation().getInverse(parentInvTransform);
	//
	//matrix4 wheelRelativeTransforms[4];

	//for (int i = 0; i < 4; i++)
	//{
	//	matrix4 wheelTransform = m_wheel[i]->getAbsoluteTransformation();
	//	wheelRelativeTransforms[i] = wheelTransform * parentInvTransform;
	//}

	// set chassis position
	m_chassis->setPosition(newpos);

	// update bodies positions
/*	vector3df chassis_body = m_chassis->getAbsolutePosition();
	dBodySetPosition(p.chassis_body, chassis_body.X, chassis_body.Y, chassis_body.Z);

	for (int i = 0; i < 4; i++)
	{
		m_wheel[i]->setPosition(wheelRelativeTransforms[i].getTranslation());
		m_wheel[i]->setRotation(wheelRelativeTransforms[i].getRotationDegrees());
		m_wheel[i]->updateAbsolutePosition();
	
		vector3df pos = m_wheel[i]->getAbsolutePosition();
		dBodySetPosition(p.wheel_body[i], pos.X, pos.Y, pos.Z);
	}*/
}

void CCar::setRotation(const irr::core::vector3df &rotation)
{

}

void CCar::turnRight(irr::s32 angle) { 
	m_steer -= 0.03f; 
	if (m_steer < -1.0f)
		m_steer = -1.0f;
}
void CCar::turnLeft(irr::s32 angle) { 
	m_steer += 0.03f; 
	if (m_steer > 1.0f)
		m_steer = 1.0f;
}

void CCar::accelerate(irr::s32 force) { 
	m_speed += 0.03f; 
}
void CCar::slowdown(irr::s32 force) { 
	m_speed -= 0.03f; 
}

//! set car looking as damaged
void CCar::damage()
{
	if (g.damaged_model)
		m_chassis->setMesh(g.damaged_model);

	m_chassis->setMaterialTexture(0, g.getDamagedChassisTexture());
	m_glass->setMaterialTexture(0, g.getDamagedGlassTexture());

	m_damaged = true;
}