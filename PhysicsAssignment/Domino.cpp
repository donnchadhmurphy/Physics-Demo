#include "Domino.h"
Domino::Domino(const btVector3 &initialPosition, GLfloat rotation2) {
	
	mass = 5;
	initialosition = initialPosition;

	rotation = rotation2;

	m_pShape = new btBoxShape(btVector3(1.0f, 0.5f, 0.1f));
	
	// store the color
	m_color = btVector3(1.0f, 0.2f, 0.2f);

	const btQuaternion &initialRotation = btQuaternion(0,0,1,1);

	// create the initial transform
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(initialosition);
	transform.setRotation(initialRotation);

	// create the motion state from the
	// initial transform
	m_pMotionState = new OpenGLMotionState(transform);

	// calculate the local inertia
	btVector3 localInertia(0,0,0);

	// objects of infinite mass can't
	// move or rotate
	if (mass != 0.0f)
		m_pShape->calculateLocalInertia(mass, localInertia);

	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, m_pMotionState, m_pShape, localInertia);
	
	// create the rigid body
	m_pBody = new btRigidBody(cInfo);
}

Domino::~Domino() {
	delete m_pBody;
	delete m_pMotionState;
	delete m_pShape;
}
