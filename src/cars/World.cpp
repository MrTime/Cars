#include "World.h"
#include "Car.h"
#include <irrXML.h>
#include <ode\ode.h>

#include "PhysicSceneAnimator.h"

using namespace irr;
using namespace irr::io;
using namespace scene;
using namespace core;

/**
*	Perform computing physics
*/
class CPhysicSceneAnimator :
	public irr::scene::ISceneNodeAnimator
{
private:
	CWorld *m_world;

public:
	CPhysicSceneAnimator(CWorld *world) : m_world(world) {}

	//! Animates a scene node.
	/** \param node Node to animate.
	\param timeMs Current time in milli seconds. */
	virtual void animateNode(irr::scene::ISceneNode* node, irr::u32 timeMs) {
		m_world->animate((float)timeMs * 0.00001f);
	}

	//! Creates a clone of this animator.
	/** Please note that you will have to drop
	(IReferenceCounted::drop()) the returned pointer after calling this. */
	virtual ISceneNodeAnimator* createClone(irr::scene::ISceneNode* node,
			irr::scene::ISceneManager* newManager=0) {
		return (new CPhysicSceneAnimator(m_world));
	}
};


CWorld * CWorld::m_instance = NULL;

class GroundShaderCallBack : public video::IShaderConstantSetCallBack
{
public:

        virtual void OnSetConstants(video::IMaterialRendererServices* services,
                        s32 userData)
        {
                video::IVideoDriver* driver = services->getVideoDriver();
               
                // set clip matrix
                core::matrix4 worldViewProj;
                worldViewProj = driver->getTransform(video::ETS_PROJECTION);
                worldViewProj *= driver->getTransform(video::ETS_VIEW);
                worldViewProj *= driver->getTransform(video::ETS_WORLD);

                services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);
                
                // set index map steps variables
				vector3df indexSteps(16.0f, 16.0f,0.0f);
				services->setVertexShaderConstant("index_steps", &indexSteps.X, 2);
                
				// set map steps variables
				vector3df steps(4.0f, 4.0f,0.0f);
				services->setVertexShaderConstant("steps", &steps.X, 2);      

				// set samplers
				int sampler = 0;
				services->setVertexShaderConstant("indexMap", (const f32*)&sampler, 1);   

				sampler = 1;
				services->setVertexShaderConstant("map", (const f32*)&sampler, 1);   
        }
};

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
	scene::IAnimatedMesh * ground_mesh = smgr->addHillPlaneMesh("ground", core::dimension2d<f32>(50.0f, 50.0f), 
															core::dimension2d<u32>(1, 1),
															0,0.0f,
															core::dimension2d<f32>(0.0f,0.0f), 
															core::dimension2d<f32>(1.0f,1.0f));

	// create shader material
	createGroundMaterial();

	// load road texture
	video::ITexture * road_map = driver->getTexture("../media/general_map.png");
	
	scene::IMeshSceneNode * ground = smgr->addMeshSceneNode(ground_mesh);
	ground->setPosition(core::vector3df(0.0f, -1.0f, 0.0f));
	ground->setMaterialTexture(0, driver->getTexture("../media/map.png"));
	ground->setMaterialTexture(1, driver->getTexture("../media/general_map.png"));
	ground->setMaterialFlag(video::EMF_LIGHTING, false);
	ground->setMaterialFlag(video::EMF_TRILINEAR_FILTER, false);
	ground->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	ground->setMaterialType((video::E_MATERIAL_TYPE)m_ground_material);

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

void CWorld::createGroundMaterial()
{
	video::IGPUProgrammingServices * gpu = m_scene_manager->getVideoDriver()->getGPUProgrammingServices();

	GroundShaderCallBack * gc = new GroundShaderCallBack();

	m_ground_material = gpu->addHighLevelShaderMaterialFromFiles(m_file_system->getAbsolutePath("../media/ground.vert"),"main",video::EVST_VS_1_1,
																 m_file_system->getAbsolutePath("../media/ground.frag"),"main",video::EPST_PS_2_0,
																 gc, video::EMT_SOLID);
	gc->drop();
}

void CWorld::collusionCallback(void *data, dGeomID o1, dGeomID o2)
{
	int i,n;

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
	float mass;
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

				sscanf(xml->getAttributeValue("mass"), "%f", &mass);

				car->setChassis(clean_model, clean_diffuse_map, node_position, mass);

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

				sscanf(xml->getAttributeValue("mass"), "%f", &mass);

				car->setWheel(wheel, wheel_diffuse_map);
				
				sscanf(xml->getAttributeValue("f_l_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::FRONT_LEFT, node_position, mass);
				
				sscanf(xml->getAttributeValue("f_r_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::FRONT_RIGHT, node_position, mass);
				
				sscanf(xml->getAttributeValue("b_l_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::BACK_LEFT, node_position, mass);
				
				sscanf(xml->getAttributeValue("b_r_wheel"), "%f %f %f", &node_position.X, &node_position.Y, &node_position.Z);
				car->addWheel(CCar::BACK_RIGHT, node_position, mass);
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
