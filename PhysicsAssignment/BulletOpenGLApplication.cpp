#include "BulletOpenGLApplication.h"

// Some constants for 3D math and the camera speed
#define RADIANS_PER_DEGREE 0.01745329f
#define CAMERA_STEP_SIZE 5.0f

GLfloat h;

BulletOpenGLApplication::BulletOpenGLApplication() 
:
m_cameraPosition(0.0f, 130.0f, 0.0f),
m_cameraTarget(0.0f, 0.0f, 0.0f),
m_cameraDistance(24.0f),
m_cameraPitch(20.0f),
m_cameraYaw(70.0f),
m_upVector(0.0f, 1.0f, 0.0f),
m_nearPlane(1.0f),
m_farPlane(1000.0f),
m_pBroadphase(0),
m_pCollisionConfiguration(0),
m_pDispatcher(0),
m_pSolver(0),
m_pWorld(0)
{
}

BulletOpenGLApplication::~BulletOpenGLApplication() {
	delete m_pWorld;
	delete m_pSolver;
	delete m_pBroadphase;
	delete m_pDispatcher;
	delete m_pCollisionConfiguration;
}

void BulletOpenGLApplication::Initialize() {
	// this function is called inside glutmain() after
	// creating the window, but before handing control
	// to FreeGLUT

	// create some floats for our ambient, diffuse, specular and position
	GLfloat ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f }; // dark grey
	GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat specular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; // white
	GLfloat position[] = { 5.0f, 10.0f, 1.0f, 0.0f };
	
	// set the ambient, diffuse, specular and position for LIGHT0
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glEnable(GL_LIGHTING); // enables lighting
	glEnable(GL_LIGHT0); // enables the 0th light
	glEnable(GL_COLOR_MATERIAL); // colors materials when lighting is enabled
		
	// enable specular lighting via materials
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMateriali(GL_FRONT, GL_SHININESS, 15);
	
	// enable smooth shading
	glShadeModel(GL_SMOOTH);
	
	// enable depth testing to be 'less than'
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// set the backbuffer clearing color to a lightish blue
	glClearColor(0.6, 0.65, 0.85, 0);

	// create the collision configuration
	m_pCollisionConfiguration = new btDefaultCollisionConfiguration();
	// create the dispatcher
	m_pDispatcher = new btCollisionDispatcher(m_pCollisionConfiguration);
	// create the broadphase
	m_pBroadphase = new btDbvtBroadphase();
	// create the constraint solver
	m_pSolver = new btSequentialImpulseConstraintSolver();
	// create the world
	m_pWorld = new btDiscreteDynamicsWorld(m_pDispatcher, m_pBroadphase, m_pSolver, m_pCollisionConfiguration);

	// create a ground plane
	CreateGameObject(new btBoxShape(btVector3(1,50,50)), 0, btVector3(0.2f, 0.6f, 0.6f), btVector3(0.0f, 0.0f, 0.0f));

	// create our scene's physics objects
	CreateObjects();

	reset = 0;
	start = 0;
}

void BulletOpenGLApplication::Keyboard(unsigned char key, int x, int y) {

	switch(key) {
	// if r is pressed
	case 'r':
		if(reset == 0)
		{
			// reset = 1; so the physics world doesnt try update the domino bodies while deleteing them
			reset = 1;

			// remove domino body from world and remove domino from list
			for(int i = 0; i < dominos.size(); i++)
			{
				m_pWorld->removeRigidBody(dominos.at(i)->GetRigidBody());
				dominos.at(i)->~Domino();
			}
		}
		break;
	}
}

void BulletOpenGLApplication::Idle() {
	// this function is called frequently, whenever FreeGlut
	// isn't busy processing its own events. It should be used
	// to perform any updating and rendering tasks

	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	// get the time since the last iteration
	float dt = m_clock.getTimeMilliseconds();
	// reset the clock to 0
	m_clock.reset();
	// update the scene (convert ms to s)
	UpdateScene(dt / 1000.0f);

	// update the camera
	UpdateCamera();

	// render the scene
	RenderScene();

	// swap the front and back buffers
	glutSwapBuffers();
}

void BulletOpenGLApplication::UpdateCamera() {
	// exit in erroneous situations
	if (m_screenWidth == 0 && m_screenHeight == 0)
		return;
	
	// select the projection matrix
	glMatrixMode(GL_PROJECTION);
	// set it to the matrix-equivalent of 1
	glLoadIdentity();
	// determine the aspect ratio of the screen
	float aspectRatio = m_screenWidth / (float)m_screenHeight;
	// create a viewing frustum based on the aspect ratio and the
	// boundaries of the camera
	glFrustum (-aspectRatio * m_nearPlane, aspectRatio * m_nearPlane, -m_nearPlane, m_nearPlane, m_nearPlane, m_farPlane);
	// the projection matrix is now set

	// select the view matrix
	glMatrixMode(GL_MODELVIEW);
	// set it to '1'
	glLoadIdentity();

	// our values represent the angles in degrees, but 3D 
	// math typically demands angular values are in radians.
	float pitch = m_cameraPitch * RADIANS_PER_DEGREE;
	float yaw = m_cameraYaw * RADIANS_PER_DEGREE;

	// create a quaternion defining the angular rotation 
	// around the up vector
	btQuaternion rotation(m_upVector, yaw);

	// set the camera's position to 0,0,0, then move the 'z' 
	// position to the current value of m_cameraDistance.
	btVector3 cameraPosition(0,0,0);
	cameraPosition[2] = -m_cameraDistance;

	// create a Bullet Vector3 to represent the camera 
	// position and scale it up if its value is too small.
	btVector3 forward(cameraPosition[0], cameraPosition[1], cameraPosition[2]);
	if (forward.length2() < SIMD_EPSILON) {
		forward.setValue(1.f,0.f,0.f);
	}

	// figure out the 'right' vector by using the cross 
	// product on the 'forward' and 'up' vectors
	btVector3 right = m_upVector.cross(forward);

	// create a quaternion that represents the camera's roll
	btQuaternion roll(right, - pitch);

	// turn the rotation (around the Y-axis) and roll (around 
	// the forward axis) into transformation matrices and 
	// apply them to the camera position. This gives us the 
	// final position
	cameraPosition = btMatrix3x3(rotation) * btMatrix3x3(roll) * cameraPosition;

	// save our new position in the member variable, and 
	// shift it relative to the target position (so that we 
	// orbit it)
	m_cameraPosition[0] = cameraPosition.getX();
	m_cameraPosition[1] = cameraPosition.getY();
	m_cameraPosition[2] = cameraPosition.getZ();
	m_cameraPosition += m_cameraTarget;

	// create a view matrix based on the camera's position and where it's
	// looking
	gluLookAt(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2], m_cameraTarget[0], m_cameraTarget[1], m_cameraTarget[2], m_upVector.getX(), m_upVector.getY(), m_upVector.getZ());
	// the view matrix is now set
}

void BulletOpenGLApplication::DrawBox(const btVector3 &halfSize) {
	
	float halfWidth = halfSize.x();
	float halfHeight = halfSize.y();
	float halfDepth = halfSize.z();

	// create the vertex positions
	btVector3 vertices[8]={	
	btVector3(halfWidth,halfHeight,halfDepth),
	btVector3(-halfWidth,halfHeight,halfDepth),
	btVector3(halfWidth,-halfHeight,halfDepth),	
	btVector3(-halfWidth,-halfHeight,halfDepth),	
	btVector3(halfWidth,halfHeight,-halfDepth),
	btVector3(-halfWidth,halfHeight,-halfDepth),	
	btVector3(halfWidth,-halfHeight,-halfDepth),	
	btVector3(-halfWidth,-halfHeight,-halfDepth)};

	// create the indexes for each triangle, using the 
	// vertices above. Make it static so we don't waste 
	// processing time recreating it over and over again
	static int indices[36] = {
		0,1,2,
		3,2,1,
		4,0,6,
		6,0,2,
		5,1,4,
		4,1,0,
		7,3,1,
		7,1,5,
		5,4,7,
		7,4,6,
		7,2,3,
		7,6,2};

	// start processing vertices as triangles
	glBegin (GL_TRIANGLES);

	// increment the loop by 3 each time since we create a 
	// triangle with 3 vertices at a time.

	for (int i = 0; i < 36; i += 3) {
		// get the three vertices for the triangle based
		// on the index values set above
		// use const references so we don't copy the object
		// (a good rule of thumb is to never allocate/deallocate
		// memory during *every* render/update call. This should 
		// only happen sporadically)
		const btVector3 &vert1 = vertices[indices[i]];
		const btVector3 &vert2 = vertices[indices[i+1]];
		const btVector3 &vert3 = vertices[indices[i+2]];

		// create a normal that is perpendicular to the 
		// face (use the cross product)
		btVector3 normal = (vert3-vert1).cross(vert2-vert1);
		normal.normalize ();

		// set the normal for the subsequent vertices
		glNormal3f(normal.getX(),normal.getY(),normal.getZ());

		// create the vertices
		glVertex3f (vert1.x(), vert1.y(), vert1.z());
		glVertex3f (vert2.x(), vert2.y(), vert2.z());
		glVertex3f (vert3.x(), vert3.y(), vert3.z());
	}

	// stop processing vertices
	glEnd();
}

void BulletOpenGLApplication::RenderScene() {
	// create an array of 16 floats (representing a 4x4 matrix)
	btScalar transform[16];

	// as long as we arent trying to delete the objects you can draw them
	if(reset == 0)
	{
		for(int i = 0; i < dominos.size(); i++)
		{
			dominos.at(i)->GetTransform(transform);
			DrawShape(transform, dominos.at(i)->GetShape(), dominos.at(i)->GetColor(), dominos.at(i)->rotation);
		}

		for(int i = 0; i < m_objects.size(); i++)
		{
			m_objects.at(i)->GetTransform(transform);
			DrawShape(transform, m_objects.at(i)->GetShape(), m_objects.at(i)->GetColor(), 0.0f);
		}
	}
}

void BulletOpenGLApplication::UpdateScene(float dt) {

	// as long as we arent trying to delete the objects
	if(reset == 0)
	{
		// check if the world object exists
		if (m_pWorld) {
			// step the simulation through time. This is called
			// every update and the amount of elasped time was 
			// determined back in ::Idle() by our clock object.
			m_pWorld->stepSimulation(dt);
		}

		// if the first domino hasnt already tipped over and started the chain reaction
		CheckForCollisionEvents();

		if(start == 0)
		{
			// apply a force to the first domino, starting the chain reaction
			dominos.at(0)->GetRigidBody()->applyCentralForce(btVector3(0, 0, 7));
		}
	}

	// if we have deleted all the dominos
	if(reset == 1)
	{
		//re create them (but this doesnt work, it just breaks, cant figure out why)
		CreateObjects();
		reset = 0;
	}
}

void BulletOpenGLApplication::DrawShape(btScalar* transform, const btCollisionShape* pShape, const btVector3 &color, GLfloat rotation) {
	// set the color
	glColor3f(color.x(), color.y(), color.z());

	// push the matrix stack
	glPushMatrix();
	glMultMatrixf(transform);

	// make a different draw call based on the object type
	switch(pShape->getShapeType()) {
		// an internal enum used by Bullet for boxes
	case BOX_SHAPE_PROXYTYPE:
		{
			// assume the shape is a box, and typecast it
			const btBoxShape* box = static_cast<const btBoxShape*>(pShape);
			// get the 'halfSize' of the box
			btVector3 halfSize = box->getHalfExtentsWithMargin();
	
			// rotate the dominos based on their rotation
			glRotatef(rotation, 1.0f, 0.0f, 0.0f);

			DrawBox(halfSize);
			break;
		}

		case CYLINDER_SHAPE_PROXYTYPE:
		{
			// assume the object is a cylinder
			const btCylinderShape* pCylinder = static_cast<const btCylinderShape*>(pShape);
			// get the relevant data
			float radius = pCylinder->getRadius();
			float halfHeight = pCylinder->getHalfExtentsWithMargin()[1];
			// draw the cylinder
			DrawCylinder(radius,halfHeight);

		break;
		}
	default:
		// unsupported type
		break;
	}

	// pop the stack
	glPopMatrix();
}

void BulletOpenGLApplication::CreateGameObject(btCollisionShape* pShape, const float &mass, const btVector3 &color, const btVector3 &initialPosition, const btQuaternion &initialRotation) {
	// create a new game object
	GameObject* pObject = new GameObject(pShape, mass, color, initialPosition, initialRotation);

	// push it to the back of the list
	m_objects.push_back(pObject);

	// check if the world object is valid
	if (m_pWorld) {
		// add the object's rigid body to the world
		m_pWorld->addRigidBody(pObject->GetRigidBody());
	}
}

void BulletOpenGLApplication::CreateDomino(const btVector3 &initialPosition, GLfloat rotation) {
	// create a new game object
	Domino* domino = new Domino(initialPosition, rotation);

	// push it to the back of the list
	dominos.push_back(domino);

	//positions.push_back(initialPosition);

	// check if the world object is valid
	if (m_pWorld) {
		// add the object's rigid body to the world
		m_pWorld->addRigidBody(domino->GetRigidBody());
	}
}

void BulletOpenGLApplication::CreateObjects() {

	float x, z, y;
	z = -17.0f;
	x = 0.0f;
	y = 0.0f;

	// the initial idea was to have a cool looking domino setup, spirals, staircases ect
	// this relies on being able to rotate the dominos so you can have them in more than just a straight line, you need to
	// rotate them so they can curve and go in circles among other things
	// however, we couldnt get the dominos to rotate correctly, they would rotate, but act and fall as if not rotated
	// becuase of this we were very limited in what we could do in terms of 'domino setup' so just showed a few dominos falling over
	GLfloat rotation = 0.0f;

	// set up first 6 dominos
	for (int i = 0; i < 6; i++)
	{
		CreateDomino(btVector3(x, y, z), rotation);
		z += 1.5;
	}

	// create a blue cylinder
	CreateGameObject(new btCylinderShape(btVector3(1,2.0,1)), 2.0, btVector3(0.0f, 0.0f, 8.0f), btVector3(x, y, z));

	z += 5;
	x = -1;
	int x2 = 1;

	// set up next 12 dominos in 2 lines
	for (int i = 0; i < 6; i++)
	{
		CreateDomino(btVector3(x, y, z), rotation);
		CreateDomino(btVector3(x2, y, z), rotation);
		z += 1.5;
	}
}

void BulletOpenGLApplication::CheckForCollisionEvents() {

	// iterate through all of the manifolds in the dispatcher
	for (int i = 0; i < m_pDispatcher->getNumManifolds(); ++i) {
		
		// get the manifold
		btPersistentManifold* pManifold = m_pDispatcher->getManifoldByIndexInternal(i);
		
		// ignore manifolds that have 
		// no contact points.
		if (pManifold->getNumContacts() > 0) {
			// get the two rigid bodies involved in the collision
			const btRigidBody* pBody0 = static_cast<const btRigidBody*>(pManifold->getBody0());
			const btRigidBody* pBody1 = static_cast<const btRigidBody*>(pManifold->getBody1());

			CollisionEvent((btRigidBody*)pBody0, (btRigidBody*)pBody1);
		}
	}
}

void BulletOpenGLApplication::CollisionEvent(btRigidBody * pBody0, btRigidBody * pBody1) 
{
	// if one of the collided dominos is the first
	if(pBody0 == dominos.at(0)->GetRigidBody() || pBody1 == dominos.at(0)->GetRigidBody())
	{
		//if one of the collided dominos id the second
		if(pBody0 == dominos.at(1)->GetRigidBody() || pBody1 == dominos.at(1)->GetRigidBody())
		{
			// stop tipping over the first (applying the force to it)
			start = 1;
		}
	}
}

void BulletOpenGLApplication::DrawCylinder(const btScalar &radius, const btScalar &halfHeight) {
/*ADD*/		static int slices = 15;
/*ADD*/		static int stacks = 10;
/*ADD*/		// tweak the starting position of the
/*ADD*/		// cylinder to match the physics object
/*ADD*/		glRotatef(-90.0, 1.0, 0.0, 0.0);
/*ADD*/		glTranslatef(0.0, 0.0, -halfHeight);
/*ADD*/		// create a quadric object to render with
/*ADD*/		GLUquadricObj *quadObj = gluNewQuadric();
/*ADD*/		// set the draw style of the quadric
/*ADD*/		gluQuadricDrawStyle(quadObj, (GLenum)GLU_FILL);
/*ADD*/		gluQuadricNormals(quadObj, (GLenum)GLU_SMOOTH);
/*ADD*/		// create a disk to cap the cylinder
/*ADD*/		gluDisk(quadObj, 0, radius, slices, stacks);
/*ADD*/		// create the main hull of the cylinder (no caps)
/*ADD*/		gluCylinder(quadObj, radius, radius, 2.f*halfHeight, slices, stacks);
/*ADD*/		// shift the position and rotation again
/*ADD*/		glTranslatef(0.0f, 0.0f, 2.f*halfHeight);
/*ADD*/		glRotatef(-180.0f, 0.0f, 1.0f, 0.0f);
/*ADD*/		// draw the cap on the other end of the cylinder
/*ADD*/		gluDisk(quadObj, 0, radius, slices, stacks);
/*ADD*/		// don't need the quadric anymore, so remove it
/*ADD*/		// to save memory
/*ADD*/		gluDeleteQuadric(quadObj);
/*ADD*/	}