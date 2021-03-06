#include <stdio.h>
#include <string>

#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "Pipeline.h"
#include "GlutBackend.h"
#include "Texture.h"
#include "CallBack.h"
#include "App.h"
#include "BasicLightingTechnique.h"
#include "Mesh.h"
#include "SkyBox.h"
#include "ForceGenerator.h"


#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1200

#define BOID_NUM 600


class Lab5 : public ICallbacks, public OgldevApp
{
public:



	void resetPlane()
	{
		aircraft.setPosition(0, 0, 0);
		aircraft.setOrientation(1, 0, 0, 0);
		aircraft.setVelocity(0, 0, 0);
		aircraft.setRotation(0, 0, 0);
	}

	Lab5():
		right_wing(Matrix3(0, 0, 0, -1, -0.5f, 0, 0, 0, 0),
			Matrix3(0, 0, 0, -0.995f, -0.5f, 0, 0, 0, 0),
			Matrix3(0, 0, 0, -1.005f, -0.5f, 0, 0, 0, 0),
			Vector3(-1.0f, 0.0f, 2.0f), &windspeed),

		left_wing(Matrix3(0, 0, 0, -1, -0.5f, 0, 0, 0, 0),
			Matrix3(0, 0, 0, -0.995f, -0.5f, 0, 0, 0, 0),
			Matrix3(0, 0, 0, -1.005f, -0.5f, 0, 0, 0, 0),
			Vector3(-1.0f, 0.0f, -2.0f), &windspeed),

		rudder(Matrix3(0, 0, 0, 0, 0, 0, 0, 0, 0),
			Matrix3(0, 0, 0, 0, 0, 0, 0.01f, 0, 0),
			Matrix3(0, 0, 0, 0, 0, 0, -0.01f, 0, 0),
			Vector3(2.0f, 0.5f, 0), &windspeed),

		tail(Matrix3(0, 0, 0, -1, -0.5f, 0, 0, 0, -0.1f),
			Vector3(2.0f, 0, 0), &windspeed),

		left_wing_control(0), right_wing_control(0), rudder_control(0),

		windspeed(0, 0, 0)
	{
		start = false;


		m_pLightingTechnique = NULL;
		m_pGameCamera = NULL;

		m_pAirPlaneMesh = NULL;

		m_scale = 0.0f;
		m_pSkyBox = NULL;

		m_dirLight.AmbientIntensity = 0.2f;
		m_dirLight.DiffuseIntensity = 0.8f;
		m_dirLight.Color = glm::vec3(1.0f, 1.0f, 1.0f);
		m_dirLight.Direction = glm::vec3(1.0f, -1.0f, 0.0f);

		m_persProjInfo.FOV = 60.0f;
		m_persProjInfo.Height = WINDOW_HEIGHT;
		m_persProjInfo.Width = WINDOW_WIDTH;
		m_persProjInfo.zNear = 1.0f;
		m_persProjInfo.zFar = 100.0f;

		prevTime = 0.0f;




		// Set up the aircraft rigid body.
		resetPlane();

		aircraft.setMass(2.5f);
		Matrix3 it;
		it.setBlockInertiaTensor(Vector3(2, 1, 1), 1);
		aircraft.setInertiaTensor(it);

		aircraft.setDamping(0.8f, 0.8f);

		aircraft.setAcceleration(Vector3::GRAVITY);
		aircraft.calculateDerivedData();

		aircraft.setAwake();
		aircraft.setCanSleep(false);

		registry.add(&aircraft, &left_wing);
		registry.add(&aircraft, &right_wing);
		registry.add(&aircraft, &rudder);
		registry.add(&aircraft, &tail);
	}


	virtual ~Lab5()
	{
		SAFE_DELETE(m_pLightingTechnique);
		SAFE_DELETE(m_pGameCamera);
		SAFE_DELETE(m_pAirPlaneMesh);
		SAFE_DELETE(m_pSkyBox);
	}


	bool Init()
	{
		glm::vec3 Pos(0.0f, 15.0f, 0.0f);
		glm::vec3 Target(0.0f, 0.0f, 1.0f);
		glm::vec3 Up(0.0, 1.0f, 0.0f);



		m_pGameCamera = new Camera(WINDOW_WIDTH, WINDOW_HEIGHT, Pos, Target, Up);

		m_pLightingTechnique = new BasicLightingTechnique();

		if (!m_pLightingTechnique->Init()) {
			printf("Error initializing the lighting technique\n");
			return false;
		}

		m_pLightingTechnique->Enable();
		m_pLightingTechnique->SetDirectionalLight(m_dirLight);
		m_pLightingTechnique->SetColorTextureUnit(0);

		
		m_pAirPlaneMesh = new Mesh();

		if (!m_pAirPlaneMesh->LoadMesh("Content/plane.obj")) {
			return false;
		}
		


		m_pSkyBox = new SkyBox(m_pGameCamera, m_persProjInfo);

		if (!m_pSkyBox->Init(".",
			"Content/orbital/orbital-element_ft.png",
			"Content/orbital/orbital-element_bk.png",
			"Content/orbital/orbital-element_up.png",
			"Content/orbital/orbital-element_dn.png",
			"Content/orbital/orbital-element_rt.png",
			"Content/orbital/orbital-element_lf.png")) {
			return false;
		}



		return true;
	}


	void Run()
	{
		GLUTBackendRun(this);
	}



	virtual void RenderSceneCB()
	{
		// --------------------------UPDATE---------------------------------

		float runningTime = GetRunningTime();

		// Find the duration of the last frame in seconds
		float duration = (runningTime - prevTime);
		prevTime = runningTime;

		if (duration <= 0.0f) return;


		Vector3 pos = aircraft.getPosition();
		Vector3 offset(10.0f + aircraft.getVelocity().magnitude(), 0, 0);
		offset = aircraft.getTransform().transformDirection(offset);
		//gluLookAt(pos.x + offset.x, pos.y + 5.0f, pos.z + offset.z,
		//	pos.x, pos.y, pos.z,
		//	0.0, 1.0, 0.0);

		// Start with no forces or acceleration.
		aircraft.clearAccumulators();

		// Add the propeller force
		Vector3 propulsion(-10.0f, 0, 0);
		propulsion = aircraft.getTransform().transformDirection(propulsion);
		aircraft.addForce(propulsion);

		// Add the forces acting on the aircraft.
		registry.updateForces(duration);

		// Update the aircraft's physics.
		aircraft.integrate(duration);

		// Do a very basic collision detection and response with the ground.
		pos = aircraft.getPosition();
		if (pos.y < 0.0f)
		{
			pos.y = 0.0f;
			aircraft.setPosition(pos);

			if (aircraft.getVelocity().y < -10.0f)
			{
				resetPlane();
			}
		}



		// --------------------------DISPLAY---------------------------------

		m_pGameCamera->OnRender();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_pLightingTechnique->Enable();


		Pipeline p;
		p.SetPerspectiveProj(m_persProjInfo);
		//p.SetCamera(glm::vec3(pos.x + offset.x, pos.y + 5.0f, pos.z + offset.z), glm::vec3(pos.x, pos.y, pos.z), glm::vec3(0.0, 1.0, 0.0));

		p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());


		// Set the transform matrix for the aircraft
		Matrix4 transform = aircraft.getTransform();

		Quaternion orientation = aircraft.getOrientation();
		Vector3 euler = orientation.toEulerAngle();

		glm::vec3 vPos(pos.x, pos.y, pos.z);
		//p.WorldPos(vPos);
		//p.Rotate(euler.x, euler.y, euler.z);
		//p.Scale(0.1, 0.1, 0.1);
		
		float array[16];
		transform.fillGLArray(array);
		glm::mat4 trs = glm::make_mat4(array);

		m_pLightingTechnique->SetWVP(p.GetVPTrans() * trs);
		m_pLightingTechnique->SetWorldMatrix(trs);
		m_pAirPlaneMesh->Render();
		
	


		m_pSkyBox->Render();
		glutSwapBuffers();
	}


	void KeyboardCB(OGLDEV_KEY OgldevKey, OGLDEV_KEY_STATE State)
	{
		switch (OgldevKey) {
		case OGLDEV_KEY_ESCAPE:
		case OGLDEV_KEY_Q:
			GLUTBackendLeaveMainLoop();
			break;
		case OGLDEV_KEY_q:
			rudder_control += 0.005f;
			break;

		case OGLDEV_KEY_e:
			rudder_control -= 0.005f;
			break;

		case OGLDEV_KEY_w:
			left_wing_control -= 0.005f;
			right_wing_control -= 0.005f;
			break;

		case OGLDEV_KEY_s:
			left_wing_control += 0.005f;
			right_wing_control += 0.005f;
			break;

		case OGLDEV_KEY_a:
			left_wing_control -= 0.005f;
			right_wing_control += 0.005f;
			break;

		case OGLDEV_KEY_d:
			left_wing_control += 0.005f;
			right_wing_control -= 0.005f;
			break;

		case OGLDEV_KEY_x:
			left_wing_control = 0.0f;
			right_wing_control = 0.0f;
			rudder_control = 0.0f;
			break;

		case OGLDEV_KEY_r:
			resetPlane();
			break;
		default:
			m_pGameCamera->OnKeyboard(OgldevKey);
		}


		// Make sure the controls are in range
		if (left_wing_control < -1.0f) left_wing_control = -1.0f;
		else if (left_wing_control > 1.0f) left_wing_control = 1.0f;
		if (right_wing_control < -1.0f) right_wing_control = -1.0f;
		else if (right_wing_control > 1.0f) right_wing_control = 1.0f;
		if (rudder_control < -1.0f) rudder_control = -1.0f;
		else if (rudder_control > 1.0f) rudder_control = 1.0f;

		// Update the control surfaces
		left_wing.setControl(left_wing_control);
		right_wing.setControl(right_wing_control);
		rudder.setControl(rudder_control);
	}


	virtual void PassiveMouseCB(int x, int y)
	{
		m_pGameCamera->OnMouse(x, y);
	}

private:
	BasicLightingTechnique* m_pLightingTechnique;
	Camera* m_pGameCamera;
	float m_scale;
	DirectionalLight m_dirLight;
	Mesh* m_pAirPlaneMesh;
	SkyBox* m_pSkyBox;
	PersProjInfo m_persProjInfo;
	float prevTime;

	// Flight Sim
	AeroControl left_wing;
	AeroControl right_wing;
	AeroControl rudder;
	Aero tail;
	RigidBody aircraft;
	ForceRegistry registry;

	Vector3 windspeed;

	float left_wing_control;
	float right_wing_control;
	float rudder_control;

	bool start;
};


int main(int argc, char** argv)
{
	GLUTBackendInit(argc, argv, true, false);

	if (!GLUTBackendCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, false, "Lab - 5")) {
		return 1;
	}

	Lab5* pApp = new Lab5();

	if (!pApp->Init()) {
		return 1;
	}

	pApp->Run();

	delete pApp;

	return 0;
}