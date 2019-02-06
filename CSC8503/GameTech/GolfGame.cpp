#include "GolfGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"

#include "../CSC8503Common/PositionConstraint.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it 

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh = new OGLMesh("cube.msh");
	cubeMesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	cubeMesh->UploadToGPU();

	sphereMesh = new OGLMesh("sphere.msh");
	sphereMesh->SetPrimitiveType(GeometryPrimitive::Triangles);
	sphereMesh->UploadToGPU();

	basicTex = (OGLTexture*)TextureLoader::LoadAPITexture("grass.jpg");  //("checkerboard.png");
	basicTex2 = (OGLTexture*)TextureLoader::LoadAPITexture("wood.png");
	basicTex3 = (OGLTexture*)TextureLoader::LoadAPITexture("ball.jpg");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete basicTex;
	delete basicTex2;
	delete basicTex3;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {

	DecideState();

	if (physics->reachedGoal) {
		renderer->DrawString("YOU WIN!!!!", Vector2(10, 500));
	}
	
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(10, 40));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(10, 40));
	}

	SelectObject();
	MoveSelectedObject();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	if (currentLevel==1) {
		testNodes.clear();
		TestPathfinding();
		DisplayPathfinding();
	}

	Debug::FlushRenderables();
	renderer->Render();

	if (SpinningWall != nullptr)
		SpinningWall->GetPhysicsObject()->AddTorque(Vector3(0, 10000, 0)); 

	if (physics->resetlevel){
		InitWorld();
		InitCamera();
	}
	
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_R)) {
		InitWorld(); //We can reset the simulation at any time with R
		InitCamera();
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::KEYBOARD_G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_F8)) {
		world->ShuffleObjects(false);
	}
	//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KEYBOARD_LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 5000, 0)); //100
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -5000, 0)); //-100
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(100, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -100));
		}

		if (Window::GetKeyboard()->KeyDown(KEYBOARD_DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 100));
		}
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_1)) {
		currentLevel = 1;
		InitWorld();
		InitCamera();
	}

	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_2)) {
		currentLevel = 2;
		InitWorld();
		InitCamera();
	}
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_B)) {
		resetCamera();
	}
}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(3.0f);
	world->GetMainCamera()->SetFarPlane(4200.0f);
	world->GetMainCamera()->SetPitch(-35.0f);
	world->GetMainCamera()->SetYaw(320.0f);
	world->GetMainCamera()->SetPosition(Vector3(-800, 120, 800)); 
}

void TutorialGame::resetCamera() {
	Camera* camera = world->GetMainCamera();
	camera->SetPosition(CurrentSphere->GetTransform().GetWorldPosition()+Vector3(100,100,100));
	camera->SetPitch(-35.0f);
	camera->SetYaw(50.0f);
}

void TutorialGame::level1() {
	BridgeConstraintTest();

	CurrentSphere = AddSphereToWorld(Vector3(-650, -60, 650), 15.0f);   //Vector3(-650, -60, 650)
	CurrentSphere->SetName("ball");

	Goal=AddCubeToWorld(Vector3(730, -85, -710), Vector3(20, 10, 20), 0.0f);
	Goal->SetName("goal");

	AddCubeToWorld(Vector3(720, -50, -710), Vector3(2, 30, 2), 0.0f);
	AddCubeToWorld(Vector3(710, -30, -710), Vector3(8, 5, 2), 0.0f); // flag
	
	AddFloorToWorld(Vector3(10, -100, 1));
	AddWallToWorld(Vector3(10, -40, 790), Vector3(800, 50, 10));// walls position -80 height
	AddWallToWorld(Vector3(10, -40, -790), Vector3(800, 50, 10));

	AddWallToWorld(Vector3(800, -40, 0), Vector3(10, 50, 780)); //up and down walls
	AddWallToWorld(Vector3(-780, -40, 0), Vector3(10, 50, 780));

	AddWallToWorld(Vector3(400, -50, 480), Vector3(10, 40, 300)); 
	AddWallToWorld(Vector3(-400, -50, -480), Vector3(10, 20, 300));

	AddWallToWorld(Vector3(100, -50, 440), Vector3(300, 40, 10));
	AddWallToWorld(Vector3(500, -50, -500), Vector3(300, 40, 10));

	Robot = AddCubeToWorld(Vector3(650, -50, 650), Vector3(15, 15, 15), 1.0f);
	Robot->SetName("robot");
}

void TutorialGame::level2() {
	CurrentSphere=AddSphereToWorld(Vector3(-650, -60, 650), 5.0f);  //(-650, -60, 650), 5.0f);
	CurrentSphere->SetName("ball");

	Goal = AddCubeToWorld(Vector3(730, -85, -710), Vector3(20, 10, 20), 0.0f);
	Goal->SetName("goal");

	AddCubeToWorld(Vector3(720, -50, -710), Vector3(2, 30, 2), 0.0f);
	AddCubeToWorld(Vector3(710, -30, -710), Vector3(8, 5, 2), 0.0f); // flag

	AddFloorToWorld(Vector3(10, -100, 1));
	AddWallToWorld(Vector3(10, -40, 790), Vector3(800, 50, 10));// walls position -80 height
	AddWallToWorld(Vector3(10, -40, -790), Vector3(800, 50, 10));

	AddWallToWorld(Vector3(800, -40, 0), Vector3(10, 50, 780)); //up and down walls
	AddWallToWorld(Vector3(-780, -40, 0), Vector3(10, 50, 780));

	AddWallToWorld(Vector3(10, -50, -100), Vector3(550, 40, 10)); //z=20 space between walls
	AddWallToWorld(Vector3(240, -50, 100), Vector3(550, 40, 10));

	AddWallToWorld(Vector3(10, -50, 350), Vector3(550, 40, 10));
	AddWallToWorld(Vector3(-100, -50, -350), Vector3(510, 40, 10));

	AddWallToWorld(Vector3(10, -50, 550), Vector3(550, 40, 10));
	AddWallToWorld(Vector3(10, -50, -550), Vector3(550, 40, 10));

	
	AddWallToWorld(Vector3(550, -50, 0), Vector3(10, 40, 90)); //plaina xwrismata

	AddWallToWorld(Vector3(150, -50, 500), Vector3(10, 40, 40));

	AddWallToWorld(Vector3(-250, -50, -670), Vector3(10, 40, 110));

	AddWallToWorld(Vector3(-450, -50, -225), Vector3(10, 40, 115));

	SpinningWall = AddSpinToWorld(Vector3(10, -50, -450), Vector3(2, 35, 80), 0.1f); //spinning wall
	SpinningWall->SetName("spinningWall");
}

enum ChangeLevel {stage1, stage2};

ChangeLevel stages = stage1;

void TutorialGame::DecideState() {
	switch (stages) {
	case stage1: 
		if (physics->reachedGoal) {
			currentLevel = 2;
			InitWorld();
			InitCamera();
			stages = stage2;
		}
		break;
	case stage2:
		if (physics->reachedGoal) {
			currentLevel = 1;
			InitWorld();
			InitCamera();
			stages = stage1;
		}
		break;
	}

}

void TutorialGame::TestPathfinding() {
	NavigationGrid grid("Grid.txt");

	NavigationPath outPath;

	int scale = 260;
	Vector3 offset = Vector3(scale * 3.5, 0, scale * 3.5);
	
	Vector3 startPos = Robot->GetTransform().GetWorldPosition() +offset;
	Vector3 endPos = CurrentSphere->GetTransform().GetWorldPosition() +offset;

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos - offset);
	}

	if (found && testNodes.size()>1) {
		Vector3 a = testNodes[0];
		Vector3 b = Robot->GetTransform().GetWorldPosition();
		Vector3 d = testNodes[1];

		Vector3 c = d - b;

		c.Normalise();

		Robot->GetPhysicsObject()->AddForce(c*1000);
	}
}

void TutorialGame:: DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];

		Debug::DrawLine(a, b, Vector4(0, 1, 0, 1));
	}
}


void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();
	selectionObject = nullptr;
	SpinningWall = nullptr;
	Robot = nullptr;

	
	
	if (currentLevel==1) level1();
	else if (currentLevel==2) level2();
	physics->resetlevel = false;
	physics->reachedGoal = false;

	

	//InitCubeGridWorld(1, 1, 50.0f, 50.0f, Vector3(10, 10, 10));

	//InitCubeGridWorld(5, 5, 50.0f, 50.0f, Vector3(10, 10, 10));
	//InitSphereGridWorld(1, 1, 50.0f, 50.0f,10.0f);                                 

	//InitSphereGridWorld(1, 1, 50.0f, 50.0f, 10.0f);          // no collision

	//InitCubeGridWorld(w,1, 1, 50.0f, 50.0f, Vector3(10, 10, 10));
	//InitCubeGridWorld(w, 1, 1, 50.0f, 50.0f, Vector3(8, 8, 8));

	//InitSphereCollisionTorqueTest(w);
	//InitCubeCollisionTorqueTest(w);

	//InitSphereGridWorld( 1, 1, 50.0f, 50.0f, 10.0f);
	//BridgeConstraintTest(w);
	//InitGJKWorld(w);

	//DodgyRaycastTest(w);
	//InitGJKWorld(w);
	//InitSphereAABBTest();
	//SimpleGJKTest(w);
	//SimpleAABBTest2();

	//InitSphereCollisionTorqueTest(w);
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(800, 10, 800);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform().SetWorldScale(floorSize);
	floor->GetTransform().SetWorldPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddWallToWorld(const Vector3& position, Vector3 dimensions) {
	GameObject* wall = new GameObject();

	Vector3	wallSize = Vector3(dimensions); //Vector3(1000, 10, 10);
	AABBVolume* volume = new AABBVolume(wallSize);
	wall->SetBoundingVolume((CollisionVolume*)volume);
	wall->GetTransform().SetWorldScale(wallSize);
	wall->GetTransform().SetWorldPosition(position);

	wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, basicTex2, basicShader));
	wall->SetPhysicsObject(new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume()));

	wall->GetPhysicsObject()->SetInverseMass(0);
	wall->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(wall);

	return wall;
}
/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);

	
	sphere->SetBoundingVolume((CollisionVolume*)volume);
	sphere->GetTransform().SetWorldScale(sphereSize);
	sphere->GetTransform().SetWorldPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex3, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex2, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddSpinToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	OBBVolume* volume = new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform().SetWorldPosition(position);
	cube->GetTransform().SetWorldScale(dimensions);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex2, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
	cube->GetPhysicsObject()->SetDenygravity(true);


	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}


void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			//Vector3 position = Vector3(x * colSpacing, radius, z * rowSpacing);
			Vector3 position = Vector3(-930, -60, 930);
			//Vector3& cubeDims = Vector3(5, 5, 5);
			AddSphereToWorld(position, radius);
		}
	}
	
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 10.0f;
	Vector3 cubeDims = Vector3(10, 10, 10);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, cubeDims.y, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
	AddFloorToWorld(Vector3(0, -100, 0));   //
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {

}

void TutorialGame::InitSphereCollisionTorqueTest() {
	AddSphereToWorld(Vector3(15, 0, 0), 10.0f);
	AddSphereToWorld(Vector3(-25, 0, 0), 10.0f);
	AddSphereToWorld(Vector3(-50, 0, 0), 10.0f);

	AddCubeToWorld(Vector3(-50, 0, -50), Vector3(60, 10, 10), 10.0f);

	AddFloorToWorld(Vector3(0, -100, 0));
}

void TutorialGame::InitCubeCollisionTorqueTest() {
	Vector3 cubeSize(10, 10, 10);
	AddCubeToWorld(Vector3(15, 0, 0), cubeSize, 10.0f);
	AddCubeToWorld(Vector3(-25, 0, 0), cubeSize, 10.0f);
	AddCubeToWorld(Vector3(-50, 0, 0), cubeSize, 10.0f);

	AddCubeToWorld(Vector3(-50, 0, -50), Vector3(60, 10, 10), 10.0f);

	AddFloorToWorld(Vector3(0, -100, 0));
}

void TutorialGame::InitSphereAABBTest() {
	Vector3 cubeSize(10, 10, 10);

	AddCubeToWorld(Vector3(0, 0, 0), cubeSize, 10.0f);
	AddSphereToWorld(Vector3(2, 0, 0), 5.0f, 10.0f);
}

void TutorialGame::InitGJKWorld() {
	Vector3 dimensions(20, 2, 10);
	float inverseMass = 10.0f;

	for (int i = 0; i < 2; ++i) {
		GameObject* cube = new GameObject();

		OBBVolume* volume = new OBBVolume(dimensions);

		cube->SetBoundingVolume((CollisionVolume*)volume);

		cube->GetTransform().SetWorldPosition(Vector3(0, 0, 0));
		cube->GetTransform().SetWorldScale(dimensions);

		if (i == 1) {
			cube->GetTransform().SetLocalOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1, 0, 0), 90.0f));
		}

		cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
		cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

		cube->GetPhysicsObject()->SetInverseMass(inverseMass);
		cube->GetPhysicsObject()->InitCubeInertia();

		world->AddGameObject(cube);
	}
}

void TutorialGame::BridgeConstraintTest() {
	Vector3 cubeSize = Vector3(5, 5, 20);

	float invCubeMass = 5; // how heavy the middle pieces are
	int numLinks = 8;
	float maxDistance = 30; // constraint distance 30
	float cubeDistance = 20; // distance between links 20

	Vector3 startPos = Vector3(0, 0, 0); 

	GameObject * start = AddCubeToWorld(startPos + Vector3(-150, -40, -200), cubeSize, 0);  //Vector3(-650, -60, 650) ball
	GameObject * end = AddCubeToWorld(startPos + Vector3(-400, -20, -200), cubeSize, 0); //Vector3((numLinks + 2)* cubeDistance, -50, 0), cubeSize, 0);

	GameObject * previous = start;

	for (int i = 0; i < numLinks; ++i) {
		GameObject * block = AddCubeToWorld(startPos + Vector3((i + 1) *cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint * constraint = new PositionConstraint(previous,
			block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint * constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::SimpleGJKTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);
	Vector3 wallDimensions = Vector3(50, 2, 50);

	GameObject* fallingCube = AddCubeToWorld(Vector3(0, 20, 0), dimensions, 10.0f);
	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);

	delete fallingCube->GetBoundingVolume();
	delete newFloor->GetBoundingVolume();

	fallingCube->SetBoundingVolume((CollisionVolume*)new OBBVolume(dimensions));
	newFloor->SetBoundingVolume((CollisionVolume*)new OBBVolume(floorDimensions));

}

void TutorialGame::SimpleAABBTest() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(100, 2, 100);

	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);
	GameObject* fallingCube = AddCubeToWorld(Vector3(10, 20, 0), dimensions, 10.0f);
}

void TutorialGame::SimpleAABBTest2() {
	Vector3 dimensions		= Vector3(5, 5, 5);
	Vector3 floorDimensions = Vector3(8, 2, 8);

	GameObject* newFloor	= AddCubeToWorld(Vector3(0, 0, 0), floorDimensions, 0.0f);
	GameObject* fallingCube = AddCubeToWorld(Vector3(8, 20, 0), dimensions, 10.0f);
}

/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KEYBOARD_Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(10, 0));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::MOUSE_LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1)); //(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(10, 0));
		renderer->DrawString("Press 1 for level 1", Vector2(10, 600));
		renderer->DrawString("Press 2 for level 2", Vector2(10, 550));
		renderer->DrawString("Press R to reset the level", Vector2(10, 500));
		renderer->DrawString("Press B to find the ball", Vector2(10, 450));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20)); //Draw debug text at 10,20
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 1000.0f;  //100.0f

	renderer->DrawString("Score: " + std::to_string(score), Vector2(10, 650));

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::MOUSE_RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
				++score;
			}
		}
	}
}