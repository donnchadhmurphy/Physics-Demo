#ifndef _BULLETOPENGLAPP_H_
#define _BULLETOPENGLAPP_H_

#include <Windows.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
/*ADD*/	// includes for convex hulls
/*ADD*/	#include "BulletCollision/CollisionShapes/btConvexPolyhedron.h"
/*ADD*/	#include "BulletCollision/CollisionShapes/btShapeHull.h"

// include our custom Motion State object
#include "OpenGLMotionState.h"

#include "GameObject.h"
#include "Domino.h"
#include "PhysicsDemo.h"
#include <vector>

// a convenient typedef to reference an STL vector of GameObjects
typedef std::vector<GameObject*> GameObjects;

typedef std::vector<Domino*> Dominos;


// struct to store our raycasting results
struct RayResult {
 	btRigidBody* pBody;
 	btVector3 hitPoint;
};

class BulletOpenGLApplication {
public:
	BulletOpenGLApplication();
	~BulletOpenGLApplication();
	void Initialize();
	// FreeGLUT callbacks //
	virtual void Keyboard(unsigned char key, int x, int y);
	virtual void Special(int key, int x, int y);
	virtual void SpecialUp(int key, int x, int y);
	virtual void Idle();

	// rendering. Can be overrideen by derived classes
	virtual void RenderScene();

	// scene updating. Can be overridden by derived classes
	virtual void UpdateScene(float dt);

	// drawing functions
	void DrawBox(const btVector3 &halfSize);
	void DrawShape(btScalar* transform, const btCollisionShape* pShape, const btVector3 &color, GLfloat rotation);

	void CreateGameObject(btCollisionShape* pShape,
		const float &mass,
		const btVector3 &color = btVector3(1.0f, 1.0f, 1.0f),
		const btVector3 &initialPosition = btVector3(0.0f, 0.0f, 0.0f),
		const btVector3 &LinearConstraint = btVector3(1.0f, 1.0f, 1.0f),
		const btQuaternion &initialRotation = btQuaternion(0, 0, 1, 1));

	void InitializePhysics();

	// camera functions
	void UpdateCamera();

	void ShutdownPhysics();

	void RotateCamera(float &angle, float value);

	void ZoomCamera(float distance);

	void DestroyGameObject(btRigidBody* pBody);

	void CreateDomino(const btVector3 &initialPosition, GLfloat rotation, btQuaternion &Rotation);

	bool Raycast(const btVector3 &startPosition, const btVector3 &direction, RayResult &output, bool includeStatic = false);

    void DrawCylinder(const btScalar &radius, const btScalar &halfHeight);

	void DrawConvexHull(const btCollisionShape* shape);

	void DrawSphere(const btScalar &radius);

	void CreateObjects();

	void CheckForCollisionEvents();

	void CollisionEvent(btRigidBody * pBody0, btRigidBody * pBody1);

	void DebugFile(char* Message);

	void LoadTextures();

	btVector3 GetPickingRay(int x, int y);

	//---
	void drawSnowMan();
		/*-----*/

protected:
	// camera control
	btVector3 m_cameraPosition; // the camera's current position
	btVector3 m_cameraTarget;	 // the camera's lookAt target
	float m_nearPlane; // minimum distance the camera will render
	float m_farPlane; // farthest distance the camera will render
	btVector3 m_upVector; // keeps the camera rotated correctly
	float m_cameraDistance; // distance from the camera to its target
	float m_cameraPitch; // pitch of the camera 
	float m_cameraYaw; // yaw of the camera

	int m_screenWidth;
	int m_screenHeight;

	int reset;
	int start;

	// core Bullet components
	btBroadphaseInterface* m_pBroadphase;
	btCollisionConfiguration* m_pCollisionConfiguration;
	btCollisionDispatcher* m_pDispatcher;
	btConstraintSolver* m_pSolver;
	btDynamicsWorld* m_pWorld;

	// a simple clock for counting time
	btClock m_clock;

	// an array of our game objects
	GameObjects m_objects;

	
};
#endif
