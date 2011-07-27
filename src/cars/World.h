#pragma once
#include <irrlicht.h>
#include <ode/ode.h>

class CCar;

/**
*	World Singleton.
*/
class CWorld
{
private:
	static CWorld * m_instance;

	irr::scene::ISceneManager * m_scene_manager;
	irr::io::IFileSystem *		m_file_system;

	dWorldID					m_world;
	dSpaceID					m_space;
	dJointGroupID				m_contactgroup;
	dGeomID						m_ground;

	irr::core::list<CCar*>		m_cars;

	irr::s32					m_ground_material;

	//! collusion callback
	static void collusionCallback(void *data, dGeomID o1, dGeomID o2);
	
	//! Create new shader material
	void	createGroundMaterial();

	//! load graphic and physic part of scene node
	void	loadSceneNode(const irr::io::IXMLReader * reader, irr::scene::IMeshCache * cache);

	//! create ODE box based on AABB
	dGeomID	createPhysicBox(const irr::scene::IMesh * mesh, const irr::core::vector3df &pos);

	//! create ODE TriMesh based on IMesh
	dGeomID	createPhysicMesh(const irr::scene::IMesh * mesh, const irr::core::vector3df &pos);

	//! Constructor
	CWorld(irr::scene::ISceneManager * smgr, irr::io::IFileSystem * fs);
public:
	//! create single instance of world
	static CWorld*	create(irr::scene::ISceneManager * smgr, irr::io::IFileSystem * fs);
	//! destroy world
	virtual ~CWorld(void);

	//! load scene from XML
	bool								loadScene(const irr::io::path &path);

	void								animate(float time);

	inline irr::scene::ISceneManager*	getSceneManager() { return m_scene_manager; }

	inline dWorldID						getPhysicWorld() { return m_world; }
	inline dSpaceID						getPhysicSpace() { return m_space; }

	//! load car from file
	CCar*	addCar(const irr::io::path &path, 
					const irr::core::vector3df &position = irr::core::vector3df(0.0f, 0.0f, 0.0f), 
					const irr::core::vector3df &rotation = irr::core::vector3df(0.0f, 0.0f, 0.0f));

	//! remove car from list
	void	removeCar(CCar * car);
};

