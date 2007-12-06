/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btBulletDynamicsCommon.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "GLDebugDrawer.h"
#include "ConcaveConvexcastDemo.h"
#include "GL_ShapeDrawer.h"
#include "GlutStuff.h"

//#define USE_PARALLEL_DISPATCHER 1
#ifdef USE_PARALLEL_DISPATCHER
#include "../../Extras/BulletMultiThreaded/SpuGatheringCollisionDispatcher.h"
#include "../../Extras/BulletMultiThreaded/Win32ThreadSupport.h"
#include "../../Extras/BulletMultiThreaded/SpuNarrowPhaseCollisionTask/SpuGatheringCollisionTask.h"
#endif//USE_PARALLEL_DISPATCHER




static btVector3*	gVertices=0;
static int*	gIndices=0;
static btBvhTriangleMeshShape* trimeshShape =0;
static btRigidBody* staticBody = 0;
static float waveheight = 5.f;

const float TRIANGLE_SIZE=8.f;


/* Scrolls back and forth over terrain */
#define NUMRAYS_IN_BAR 100
class btConvexcastBatch
{
public:
	btVector3 source[NUMRAYS_IN_BAR];
	btVector3 dest[NUMRAYS_IN_BAR];
	btVector3 direction[NUMRAYS_IN_BAR];
	btVector3 hit[NUMRAYS_IN_BAR];
	btVector3 normal[NUMRAYS_IN_BAR];

	int frame_counter;
	int ms;
	int sum_ms;
	int sum_ms_samples;
	int min_ms;
	int max_ms;

	btClock frame_timer;

	btScalar dx;
	btScalar min_x;
	btScalar max_x;
	btScalar min_y;
	btScalar max_y;
	btScalar sign;

	btVector3  boxShapeHalfExtents;
	btBoxShape boxShape;

	btConvexcastBatch () : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
	}

	btConvexcastBatch (bool unused, btScalar ray_length, btScalar min_z, btScalar max_z, btScalar min_y = -10, btScalar max_y = 10) : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		boxShapeHalfExtents = btVector3(1.0, 1.0, 1.0);
		boxShape = btBoxShape(boxShapeHalfExtents);
		frame_counter = 0;
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
		dx = 10.0;
		min_x = -40;
		max_x = 20;
		this->min_y = min_y;
		this->max_y = max_y;
		sign = 1.0;
		btScalar dalpha = 2*SIMD_2_PI/NUMRAYS_IN_BAR;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btScalar z = (max_z-min_z)/NUMRAYS_IN_BAR * i + min_z;
			source[i] = btVector3(min_x, max_y, z);
			dest[i] = btVector3(min_x + ray_length, min_y, z);
			normal[i] = btVector3(1.0, 0.0, 0.0);
		}
	}

	btConvexcastBatch (btScalar ray_length, btScalar z, btScalar min_y = -1000, btScalar max_y = 10) : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		boxShapeHalfExtents = btVector3(1.0, 1.0, 1.0);
		boxShape = btBoxShape(boxShapeHalfExtents);
		frame_counter = 0;
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
		dx = 10.0;
		min_x = -40;
		max_x = 20;
		this->min_y = min_y;
		this->max_y = max_y;
		sign = 1.0;
		btScalar dalpha = 2*SIMD_2_PI/NUMRAYS_IN_BAR;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btScalar alpha = dalpha * i;
			// rotate around by alpha degrees y 
			btQuaternion q(btVector3(0.0, 1.0, 0.0), alpha);
			direction[i] = btVector3(1.0, 0.0, 0.0);
			direction[i] = q * direction[i];
			direction[i] = direction[i];
			source[i] = btVector3(min_x, max_y, z);
			dest[i] = source[i] + direction[i] * ray_length;
			dest[i][1] = min_y;
			normal[i] = btVector3(1.0, 0.0, 0.0);
		}
	}

	void move (btScalar dt)
	{
		if (dt > (1.0/60.0))
			dt = 1.0/60.0;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			source[i][0] += dx * dt * sign;
			dest[i][0] += dx * dt * sign;
		}
		if (source[0][0] < min_x)
			sign = 1.0;
		else if (source[0][0] > max_x)
			sign = -1.0;
	}

	void cast (btCollisionWorld* cw)
	{
		frame_timer.reset ();
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btCollisionWorld::ClosestConvexResultCallback cb(source[i], dest[i]);
			
			cw->convexTest (&boxShape, source[i], dest[i], cb);
			if (cb.HasHit ())
			{
				hit[i] = cb.m_hitPointWorld;
				normal[i] = cb.m_hitNormalWorld;
				normal[i].normalize ();
			} else {
				hit[i] = dest[i];
				normal[i] = btVector3(1.0, 0.0, 0.0);
			}

		}
		ms += frame_timer.getTimeMilliseconds ();
		frame_counter++;
		if (frame_counter > 50)
		{
			min_ms = ms < min_ms ? ms : min_ms;
			max_ms = ms > max_ms ? ms : max_ms;
			sum_ms += ms;
			sum_ms_samples++;
			btScalar mean_ms = (btScalar)sum_ms/(btScalar)sum_ms_samples;
			printf("%d rays in %d ms %d %d %f\n", NUMRAYS_IN_BAR * frame_counter, ms, min_ms, max_ms, mean_ms);
			ms = 0;
			frame_counter = 0;
		}
	}


	void drawCube (const btVector3& com)
	{
		glPushMatrix ();
			glTranslatef (com[0], com[1], com[2]);
			glScalef (2.0 * boxShapeHalfExtents[0], 2.0 * boxShapeHalfExtents[1], 2.0 * boxShapeHalfExtents[2]);
			glutSolidCube (1.0);
		glPopMatrix ();
	}

	void draw ()
	{
		glDisable (GL_LIGHTING);
		glColor3f (0.0, 1.0, 0.0);
		glBegin (GL_LINES);
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			glVertex3f (source[i][0], source[i][1], source[i][2]);
			glVertex3f (hit[i][0], hit[i][1], hit[i][2]);
		}
		glColor3f (1.0, 1.0, 1.0);
		glBegin (GL_LINES);
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			glVertex3f (hit[i][0], hit[i][1], hit[i][2]);
			glVertex3f (hit[i][0] + normal[i][0], hit[i][1] + normal[i][1], hit[i][2] + normal[i][2]);
		}
		glEnd ();
		glColor3f (0.0, 1.0, 1.0);
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			drawCube (hit[i]);
		}
		glEnable (GL_LIGHTING);
	}
};


static btConvexcastBatch convexcastBatch;

///User can override this material combiner by implementing gContactAddedCallback and setting body0->m_collisionFlags |= btCollisionObject::customMaterialCallback;
inline btScalar	calculateCombinedFriction(float friction0,float friction1)
{
	btScalar friction = friction0 * friction1;

	const btScalar MAX_FRICTION  = 10.f;
	if (friction < -MAX_FRICTION)
		friction = -MAX_FRICTION;
	if (friction > MAX_FRICTION)
		friction = MAX_FRICTION;
	return friction;

}

inline btScalar	calculateCombinedRestitution(float restitution0,float restitution1)
{
	return restitution0 * restitution1;
}



static bool CustomMaterialCombinerCallback(btManifoldPoint& cp,	const btCollisionObject* colObj0,int partId0,int index0,const btCollisionObject* colObj1,int partId1,int index1)
{

	float friction0 = colObj0->getFriction();
	float friction1 = colObj1->getFriction();
	float restitution0 = colObj0->getRestitution();
	float restitution1 = colObj1->getRestitution();

	if (colObj0->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK)
	{
		friction0 = 1.0;//partId0,index0
		restitution0 = 0.f;
	}
	if (colObj1->getCollisionFlags() & btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK)
	{
		if (index1&1)
		{
			friction1 = 1.0f;//partId1,index1
		} else
		{
			friction1 = 0.f;
		}
		restitution1 = 0.f;
	}

	cp.m_combinedFriction = calculateCombinedFriction(friction0,friction1);
	cp.m_combinedRestitution = calculateCombinedRestitution(restitution0,restitution1);

	//this return value is currently ignored, but to be on the safe side: return false if you don't calculate friction
	return true;
}

extern ContactAddedCallback		gContactAddedCallback;

	const int NUM_VERTS_X = 30;
	const int NUM_VERTS_Y = 30;
	const int totalVerts = NUM_VERTS_X*NUM_VERTS_Y;

void	ConcaveConvexcastDemo::setVertexPositions(float waveheight, float offset)
{
	int i;
	int j;

	for ( i=0;i<NUM_VERTS_X;i++)
	{
		for (j=0;j<NUM_VERTS_Y;j++)
		{
			gVertices[i+j*NUM_VERTS_X].setValue((i-NUM_VERTS_X*0.5f)*TRIANGLE_SIZE,
				//0.f,
				waveheight*sinf((float)i+offset)*cosf((float)j+offset),
				(j-NUM_VERTS_Y*0.5f)*TRIANGLE_SIZE);
		}
	}
}

void ConcaveConvexcastDemo::keyboardCallback(unsigned char key, int x, int y)
{
	if (key == 'g')
	{
		m_animatedMesh = !m_animatedMesh;
		if (m_animatedMesh)
		{
			staticBody->setCollisionFlags( staticBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			staticBody->setActivationState(DISABLE_DEACTIVATION);
		} else
		{
			staticBody->setCollisionFlags( staticBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
			staticBody->forceActivationState(ACTIVE_TAG);
		}
	}

	DemoApplication::keyboardCallback(key,x,y);

}

void	ConcaveConvexcastDemo::initPhysics()
{
	#define TRISIZE 10.f

     gContactAddedCallback = CustomMaterialCombinerCallback;

#define USE_TRIMESH_SHAPE 1
#ifdef USE_TRIMESH_SHAPE

	int vertStride = sizeof(btVector3);
	int indexStride = 3*sizeof(int);

	
	const int totalTriangles = 2*(NUM_VERTS_X-1)*(NUM_VERTS_Y-1);

	gVertices = new btVector3[totalVerts];
	gIndices = new int[totalTriangles*3];

	int i;


	setVertexPositions(waveheight,0.f);

	int index=0;
	for ( i=0;i<NUM_VERTS_X-1;i++)
	{
		for (int j=0;j<NUM_VERTS_Y-1;j++)
		{
			gIndices[index++] = j*NUM_VERTS_X+i;
			gIndices[index++] = j*NUM_VERTS_X+i+1;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i+1;

			gIndices[index++] = j*NUM_VERTS_X+i;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i+1;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i;
		}
	}

	m_indexVertexArrays = new btTriangleIndexVertexArray(totalTriangles,
		gIndices,
		indexStride,
		totalVerts,(btScalar*) &gVertices[0].x(),vertStride);

	bool useQuantizedAabbCompression = true;

//comment out the next line to read the BVH from disk (first run the demo once to create the BVH)
#define SERIALIZE_TO_DISK 1
#ifdef SERIALIZE_TO_DISK
	trimeshShape  = new btBvhTriangleMeshShape(m_indexVertexArrays,useQuantizedAabbCompression);
	m_collisionShapes.push_back(trimeshShape);
	
	
	///we can serialize the BVH data 
	void* buffer = 0;
	int numBytes = trimeshShape->getOptimizedBvh()->calculateSerializeBufferSize();
	buffer = btAlignedAlloc(numBytes,16);
	bool swapEndian = false;
	trimeshShape->getOptimizedBvh()->serialize(buffer,numBytes,swapEndian);
	FILE* file = fopen("bvh.bin","wb");
	fwrite(buffer,1,numBytes,file);
	fclose(file);
	btAlignedFree(buffer);
	


#else

	trimeshShape  = new btBvhTriangleMeshShape(m_indexVertexArrays,useQuantizedAabbCompression,false);

	char* fileName = "bvh.bin";

	FILE* file = fopen(fileName,"rb");
	int size=0;
	btOptimizedBvh* bvh = 0;

	if (fseek(file, 0, SEEK_END) || (size = ftell(file)) == EOF || fseek(file, 0, SEEK_SET)) {        /* File operations denied? ok, just close and return failure */
		printf("Error: cannot get filesize from %s\n", fileName);
		exit(0);
	} else
	{

		fseek(file, 0, SEEK_SET);

		int buffersize = size+btOptimizedBvh::getAlignmentSerializationPadding();

		void* buffer = btAlignedAlloc(buffersize,16);
		int read = fread(buffer,1,size,file);
		fclose(file);
		bool swapEndian = false;
		bvh = btOptimizedBvh::deSerializeInPlace(buffer,buffersize,swapEndian);
	}

	trimeshShape->setOptimizedBvh(bvh);

#endif

	btCollisionShape* groundShape = trimeshShape;
	
#else
	btCollisionShape* groundShape = new btBoxShape(btVector3(50,3,50));
	m_collisionShapes.push_back(groundShape);

#endif //USE_TRIMESH_SHAPE

	m_collisionConfiguration = new btDefaultCollisionConfiguration();

#ifdef USE_PARALLEL_DISPATCHER

#ifdef USE_WIN32_THREADING

	int maxNumOutstandingTasks = 4;//number of maximum outstanding tasks
	Win32ThreadSupport* threadSupport = new Win32ThreadSupport(Win32ThreadSupport::Win32ThreadConstructionInfo(
								"collision",
								processCollisionTask,
								createCollisionLocalStoreMemory,
								maxNumOutstandingTasks));
#else
///todo other platform threading
///Playstation 3 SPU (SPURS)  version is available through PS3 Devnet
///Libspe2 SPU support will be available soon
///pthreads version
///you can hook it up to your custom task scheduler by deriving from btThreadSupportInterface
#endif

	m_dispatcher = new	SpuGatheringCollisionDispatcher(threadSupport,maxNumOutstandingTasks,m_collisionConfiguration);
#else
	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);
#endif//USE_PARALLEL_DISPATCHER


	btVector3 worldMin(-1000,-1000,-1000);
	btVector3 worldMax(1000,1000,1000);
	m_broadphase = new btAxisSweep3(worldMin,worldMax);
	m_solver = new btSequentialImpulseConstraintSolver();
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);
#ifdef USE_PARALLEL_DISPATCHER
	m_dynamicsWorld->getDispatchInfo().m_enableSPU=true;
#endif //USE_PARALLEL_DISPATCHER
	
	float mass = 0.f;
	btTransform	startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0,-2,0));

	btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
	m_collisionShapes.push_back(colShape);

	{
		for (int i=0;i<10;i++)
		{
			//btCollisionShape* colShape = new btCapsuleShape(0.5,2.0);//boxShape = new btSphereShape(1.f);
			startTransform.setOrigin(btVector3(2*i,10,1));
			localCreateRigidBody(1, startTransform,colShape);
		}
	}

	startTransform.setIdentity();
	staticBody = localCreateRigidBody(mass, startTransform,groundShape);

	staticBody->setCollisionFlags(staticBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

	//enable custom material callback
	staticBody->setCollisionFlags(staticBody->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	convexcastBatch = btConvexcastBatch (40.0, 0.0, -10.0);
	//convexcastBatch = btConvexcastBatch (true, 40.0, -50.0, 50.0);
}

void ConcaveConvexcastDemo::clientMoveAndDisplay()
{
	 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	float dt = m_clock.getTimeMicroseconds() * 0.000001f;
	m_clock.reset();

	if (m_animatedMesh)
	{
		static float offset=0.f;
		offset+=0.01f;

		setVertexPositions(waveheight,offset);

		trimeshShape->refitTree();

		//clear all contact points involving mesh proxy. Note: this is a slow/unoptimized operation.
		m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(staticBody->getBroadphaseHandle(),getDynamicsWorld()->getDispatcher());
	}

	m_dynamicsWorld->stepSimulation(dt);
	
	convexcastBatch.move (dt);
	convexcastBatch.cast (m_dynamicsWorld);
	renderme();
	convexcastBatch.draw ();
    glFlush();
    glutSwapBuffers();

}




void ConcaveConvexcastDemo::displayCallback(void) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	renderme();
	convexcastBatch.draw ();
    glFlush();
    glutSwapBuffers();
}



void	ConcaveConvexcastDemo::exitPhysics()
{



	//cleanup in the reverse order of creation/initialization

	//remove the rigidbodies from the dynamics world and delete them
	int i;
	for (i=m_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
	{
		btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		m_dynamicsWorld->removeCollisionObject( obj );
		delete obj;
	}

	//delete collision shapes
	for (int j=0;j<m_collisionShapes.size();j++)
	{
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}

	//delete dynamics world
	delete m_dynamicsWorld;

	if (m_indexVertexArrays)
		delete m_indexVertexArrays;

	//delete solver
	delete m_solver;

	//delete broadphase
	delete m_broadphase;

	//delete dispatcher
	delete m_dispatcher;

	delete m_collisionConfiguration;

	
}




