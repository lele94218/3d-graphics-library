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
#include "PhysicsEngine.h"
#include "BoundingSphere.h"
#include "SimulationFlock.h"


#define WINDOW_WIDTH  1920
#define WINDOW_HEIGHT 1200

#define BOID_NUM 600


class Lab4 : public ICallbacks, public OgldevApp
{
public:

	Lab4()
	{
		start = false;

		m_pFlock = NULL;

		m_pLightingTechnique = NULL;
		m_pGameCamera = NULL;

		m_pBoidMesh = NULL;

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
	}


	virtual ~Lab4()
	{
		SAFE_DELETE(m_pLightingTechnique);
		SAFE_DELETE(m_pGameCamera);
		SAFE_DELETE(m_pBoidMesh);
		SAFE_DELETE(m_pSkyBox);
	}


	bool Init()
	{
		glm::vec3 Pos(0.0f, 15.0f, -30.0f);
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

		m_pBoidMesh = new Mesh();

		if (!m_pBoidMesh->LoadMesh("Content/boid.obj")) {
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

		/* Add Flock */
		m_pFlock = new Flock();

		for (int i = 0; i < BOID_NUM; ++i) {
			m_pFlock->AddBoid(Boid(0.0f, 0.0f, 0.0f));
		}
		

		return true;
	}


	void Run()
	{
		GLUTBackendRun(this);
	}


	virtual void RenderSceneCB()
	{

		m_pGameCamera->OnRender();

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		m_pLightingTechnique->Enable();

		Pipeline p;
		p.SetPerspectiveProj(m_persProjInfo);

		float RunningTime = GetRunningTime();
		if (start) {
			m_pFlock->Run();
		}

		for (unsigned int i = 0; i < m_pFlock->GetBoidNum(); ++i) {
			Boid boid = m_pFlock->GetBoid(i);

			p.Scale(0.3f, 0.3f, 0.3f);
			p.Rotate(boid.getRotation());
			p.WorldPos(boid.getPosition());
			p.SetCamera(m_pGameCamera->GetPos(), m_pGameCamera->GetTarget(), m_pGameCamera->GetUp());
			m_pLightingTechnique->SetWVP(p.GetWVPTrans());
			m_pLightingTechnique->SetWorldMatrix(p.GetWorldTrans());
			m_pBoidMesh->Render();
		}

		m_pSkyBox->Render();

		glutSwapBuffers();
	}


	void KeyboardCB(OGLDEV_KEY OgldevKey, OGLDEV_KEY_STATE State)
	{
		switch (OgldevKey) {
		case OGLDEV_KEY_ESCAPE:
		case OGLDEV_KEY_q:
			GLUTBackendLeaveMainLoop();
			break;
		case OGLDEV_KEY_f:
			start = true;
			break;
		default:
			m_pGameCamera->OnKeyboard(OgldevKey);
		}
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
	Mesh* m_pBoidMesh;
	SkyBox* m_pSkyBox;
	PersProjInfo m_persProjInfo;
	Flock* m_pFlock;
	float prevTime;

	bool start;
};


int main(int argc, char** argv)
{
	GLUTBackendInit(argc, argv, true, false);

	if (!GLUTBackendCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, false, "Lab - 4")) {
		return 1;
	}

	Lab4* pApp = new Lab4();

	if (!pApp->Init()) {
		return 1;
	}

	pApp->Run();

	delete pApp;

	return 0;
}