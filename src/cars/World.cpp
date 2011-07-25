#include "World.h"
#include "Car.h"
#include <irrXML.h>
#include <ode\ode.h>

#include "PhysicSceneAnimator.h"

using namespace irr;
using namespace irr::io;

CWorld * CWorld::m_instance = NULL;

CWorld* CWorld::create(irr::scene::ISceneManager * smgr, irr::io::IFileSystem * fs)
{
	return (m_instance ? m_instance : (m_instance = new CWorld(smgr, fs)));
}

CWorld::CWorld(irr::scene::ISceneManager * smgr, irr::io::IFileSystem * fs) : m_scene_manager(smgr), m_file_system(fs)
{
	video::IVideoDriver * driver = smgr->getVideoDriver();

	// create ODE world, space and contact joint group
	m_world = dWorldCreate();
	m_space = dHashSpaceCreate(0);
	m_contactgroup = dJointGroupCreate(0);
	dWorldSetGravity(m_world, 0.0f, -0.1f, 0.0f);
	
	// setup ambient lighting
	m_scene_manager->setAmbientLight(video::SColorf(0.5f, 0.5f, 0.5f));
	scene::ILightSceneNode *sun = m_scene_manager->addLightSceneNode(0, core::vector3df(10.0f, 10.0f, 10.0f));
	sun->setLightType(video::ELT_DIRECTIONAL);

	// setup skybox
	scene::ISceneNode * skybox = smgr->addSkyBoxSceneNode(driver->getTexture("../media/irrlicht2_up.jpg"),
														  driver->getTexture("../media/irrlicht2_dn.jpg"),
														  driver->getTexture("../media/irrlicht2_lf.jpg"),
														  driver->getTexture("../media/irrlicht2_rt.jpg"),
														  driver->getTexture("../media/irrlicht2_ft.jpg"),
														  driver->getTexture("../media/irrlicht2_bk.jpg"));

	// create ground
	scene::IAnimatedMesh * ground_mesh = smgr->addHillPlaneMesh("ground", core::dimension2d<f32>(5.0f, 5.0f), 
															core::dimension2d<u32>(10, 10),
															0,0.0f,
															core::dimension2d<f32>(0.0f,0.0f), 
															core::dimension2d<f32>(10.0f,10.0f));
	scene::IMeshSceneNode * ground = smgr->addMeshSceneNode(ground_mesh);
	ground->setPosition(core::vector3df(0.0f, -1.0f, 0.0f));
	ground->setMaterialTexture(0, driver->getTexture("../media/grass.jpg"));
	ground->setMaterialFlag(video::EMF_LIGHTING, false);	

	// create ground physic model
	m_ground = dCreatePlane(m_space, 0,1,0,-1);

	// create scene animator
	CPhysicSceneAnimator * animator = new CPhysicSceneAnimator(this);
	smgr->getRootSceneNode()->addAnimator(animator);
	animator->drop();
}


CWorld::~CWorld(void)
{
	// clean cars
	//while (!m_cars.empty())
	//	delete *(m_cars.begin());

	// clear ODE
	dJointGroupDestroy(m_contactgroup);
	dSpaceDestroy(m_space);
	dWorldDestroy(m_world);

	m_instance = NULL;
}

void CWorld::collusionCallback(void *data, dGeomID o1, dGeomID o2)
{
	int i,n;

	int g1 = (o1 == m_instance->m_ground);
	int g2 = (o2 == m_instance->m_ground);
	if (!(g1 ^ g2)) return;

	const int N = 10;
	dContact contact[N];
	n = dCollide(o1,o2, N, &contact[0].geom, sizeof(dContact));
	if (n > 0) {
		for (i = 0; i < n; i++) {
			contact[i].surface.mode = dContactSlip1 | dContactSlip2 | dContactSoftERP | dContactSoftCFM | dContactApprox1;
			contact[i].surface.mu = dInfinity;
			contact[i].surface.slip1 = 0.1f;
			contact[i].surface.slip2 = 0.1f;
			contact[i].surface.soft_erp = 0.5f;
			contact[i].surface.soft_cfm = 0.3f;
			dJointID c = dJointCreateContact(m_instance->m_world,m_instance->m_contactgroup,&contact[i]);
			dJointAttach(c, dGeomGetBody(contact[i].geom.g1), dGeomGetBody(contact[i].geom.g2));
		}
	}
}

CCar* CWorld::addCar(const irr::io::path &path,const irr::core::vector3df &position,const irr::core::vector3df &rotation)
{
	core::vector3df node_position;
	video::IVideoDriver *driver = m_scene_manager->getVideoDriver();
	scene::IMeshCache * cache = m_scene_manager->getMeshCache();	

	// create xml reader object
	IrrXMLReader *xml = createIrrXMLReader(path.c_str());

	if (!xml)
		return false;

	// get directory of file, this will be used during loading resources from that file
	irr::io::path directory =  path.subString(0, path.findLastChar("/",1)+1);
	irr::io::path dae_path, dae_base;

	// create new car object
	CCar * car = new CCar(this);

	// read xml elements
	while (xml->read())
	{
		if (xml->getNodeType() == EXN_ELEMENT)
		{
			if (strcmp(xml->getNodeName(), "collada") == 0)
			{
				dae_path = m_file_system->getAbsolutePath(directory + xml->getAttributeValue("path"));
				dae_base = dae_path + "#geom-";

				m_scene_manager->getMesh(dae_path);
			}
			if (strcmp(xml->getNodeName(), "clean") == 0)
			{
				// load clean car
				scene::IMesh * clean_model = cache->getMeshByName(dae_base + xml->getAttributeValue("model"));
				video::ITexture * clean_diffuse_map = driver->getTexture(directory + xml->getAttributeValue("diffuse_map"));

				sscanf(xml->getAttributeValue("position"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);

				car->setChassis(clean_model, clean_diffuse_map, node_position);

				// load glass
				scene::IMesh *glass = cache->getMeshByName(dae_base + xml->getAttributeValue("glass_model"));
				car->setGlass(glass);
			}
			else
			if (strcmp(xml->getNodeName(), "damaged") == 0)
			{
				// load damaged car
				scene::IMesh * damaged_model = m_scene_manager->getMesh(dae_base + xml->getAttributeValue("model"));
				video::ITexture * damaged_diffuse_map = driver->getTexture(directory + xml->getAttributeValue("diffuse_map"));

				car->setDamagedChassis(damaged_model, damaged_diffuse_map);
			}
			else
			if (strcmp(xml->getNodeName(), "wheels") == 0)
			{
				// load wheel
				// we give model for front and back wheels (left or right doen't matter), but they must be poses in right place
				// according the car
				scene::IMesh *wheel = m_scene_manager->getMesh(dae_base + xml->getAttributeValue("model"));
				video::ITexture *wheel_diffuse_map = driver->getTexture(directory + xml->getAttributeValue("diffuse_map"));

				car->setWheel(wheel, wheel_diffuse_map);
				
				sscanf(xml->getAttributeValue("f_l_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::FRONT_LEFT, node_position);
				
				sscanf(xml->getAttributeValue("f_r_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::FRONT_RIGHT, node_position);
				
				sscanf(xml->getAttributeValue("b_l_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::BACK_LEFT, node_position);
				
				sscanf(xml->getAttributeValue("b_r_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::BACK_RIGHT, node_position);
			}
		}
	}

	delete xml;

	car->setPosition(position);
	car->setRotation(rotation);

	// add to car list
	if (car)
		m_cars.push_back(car);

	return car;
}

void CWorld::animate(float time)
{
	dSpaceCollide(m_space, 0, &collusionCallback);
	dWorldStep(m_world, time);
	dJointGroupEmpty(m_contactgroup);
}
