/***********************************************************************
LocalWaterTool - Tool class to locally add or remove water from an
augmented reality sandbox.
Copyright (c) 2012-2013 Oliver Kreylos

This file is part of the Augmented Reality Sandbox (SARndbox).

The Augmented Reality Sandbox is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Augmented Reality Sandbox is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Augmented Reality Sandbox; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

#include "LocalWaterTool.h"

#include <Misc/FunctionCalls.h>
#include <GL/gl.h>
#include <GL/Extensions/GLARBVertexProgram.h>
#include <GL/GLGeometryWrappers.h>
#include <GL/GLTransformationWrappers.h>
#include <Vrui/Vrui.h>
#include <Vrui/ToolManager.h>
#include <Vrui/DisplayState.h>

#include "WaterTable2.h"
#include "Sandbox.h"


/***************************************
Static elements of class LocalWaterTool:
***************************************/

LocalWaterToolFactory* LocalWaterTool::factory=0;

/*******************************
Methods of class LocalWaterTool:
*******************************/

LocalWaterToolFactory* LocalWaterTool::initClass(Vrui::ToolManager& toolManager)
	{
	/* Create the tool factory: */
	factory=new LocalWaterToolFactory("LocalWaterTool","Manage Water Locally",0,toolManager);
	
	/* Set up the tool class' input layout: */
	factory->setNumButtons(2);
	factory->setButtonFunction(0,"Rain");
	factory->setButtonFunction(1,"Dry");
	//factory->setButtonFunction(2,"LocalWaterTool");
	
	/* Register and return the class: */
	toolManager.addClass(factory,Vrui::ToolManager::defaultToolFactoryDestructor);
	return factory;
	}

LocalWaterTool::LocalWaterTool(const Vrui::ToolFactory* factory,const Vrui::ToolInputAssignment& inputAssignment)
	:Vrui::Tool(factory,inputAssignment),
	 addWaterFunction(0),
	 adding(0.0f)
	{
	}

LocalWaterTool::~LocalWaterTool(void)
	{
	}

void LocalWaterTool::initialize(void)
	{
	/* Register a render function with the water table: */
	if(application->waterTable!=0)
		{
		addWaterFunction=Misc::createFunctionCall(this,&LocalWaterTool::addWater);
		application->waterTable->addRenderFunction(addWaterFunction);
		}
	}

void LocalWaterTool::deinitialize(void)
	{
	/* Unregister the render function from the water table: */
	if(application->waterTable!=0)
		application->waterTable->removeRenderFunction(addWaterFunction);
	delete addWaterFunction;
	addWaterFunction=0;
	}

const Vrui::ToolFactory* LocalWaterTool::getFactory(void) const
	{
	return factory;
	}

void LocalWaterTool::buttonCallback(int buttonSlotIndex,Vrui::InputDevice::ButtonCallbackData* cbData)
	{
	GLfloat waterAmount=application->rainStrength;
	if(!cbData->newButtonState)
		waterAmount=-waterAmount;
	if(buttonSlotIndex==1)
		waterAmount=-waterAmount;
	adding+=waterAmount;
	}

void LocalWaterTool::initContext(GLContextData& contextData) const
	{
	/* Initialize the required OpenGL extensions: */
	GLARBVertexProgram::initExtension();
	}

void LocalWaterTool::glRenderActionTransparent(GLContextData& contextData) const
	{
	glPushAttrib(GL_ENABLE_BIT|GL_POLYGON_BIT);
	
	/* Go to navigational coordinates: */
	glPushMatrix();
	glLoadMatrix(Vrui::getDisplayState(contextData).modelviewNavigational);
	
	/* Get the current rain disk position and size in camera coordinates: */
	Vrui::Point rainPos=Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0));
	Vrui::Scalar rainRadius=Vrui::getPointPickDistance()*Vrui::Scalar(0);
	
	/* Construct the rain cylinder: */
	Vrui::Vector z=application->waterTable->getBaseTransform().inverseTransform(Vrui::Vector(0,0,1));
	Vrui::Vector x=Geometry::normal(z);
	Vrui::Vector y=Geometry::cross(z,x);
	x.normalize();
	y.normalize();
	
	/* Set the rain cylinder's material: */
	GLfloat diffuseCol[4]={0.0f,0.0f,1.0f,0.333f};
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,diffuseCol);
	
	/* Render the back faces of the rain cylinder: */
	glCullFace(GL_FRONT);
	glBegin(GL_QUAD_STRIP);
	for(int i=0;i<=32;++i)
		{
		Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
		glNormal(x*Math::cos(angle)+y*Math::sin(angle));
		glVertex(rainPos+x*(Math::cos(angle)*rainRadius)+y*(Math::sin(angle)*rainRadius));
		glVertex(rainPos+x*(Math::cos(angle)*rainRadius)+y*(Math::sin(angle)*rainRadius)-z*Vrui::Scalar(50));
		}
	glEnd();
	
	/* Render the front faces of the rain cylinder: */
	glCullFace(GL_BACK);
	glBegin(GL_QUAD_STRIP);
	for(int i=0;i<=32;++i)
		{
		Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
		glNormal(x*Math::cos(angle)+y*Math::sin(angle));
		glVertex(rainPos+x*(Math::cos(angle)*rainRadius)+y*(Math::sin(angle)*rainRadius));
		glVertex(rainPos+x*(Math::cos(angle)*rainRadius)+y*(Math::sin(angle)*rainRadius)-z*Vrui::Scalar(50));
		}
	glEnd();
	glBegin(GL_POLYGON);
	glNormal(z);
	for(int i=0;i<32;++i)
		{
		Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
		glVertex(rainPos+x*(Math::cos(angle)*rainRadius)+y*(Math::sin(angle)*rainRadius));
		}
	glEnd();
	
	glPopMatrix();
	glPopAttrib();
	}

void LocalWaterTool::addWater(GLContextData& contextData) const
	{
	if(adding!=0.0f)
		{
		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_CULL_FACE);
		
		/* Get the current rain disk position and size in camera coordinates: */
//ERIC: Set rain position here!
		Vrui::Point rainPosMain=Vrui::Point(45.4913,-50.3619,-124.993);//Main Dam
	//	Vrui::Point rainPosBridge=Vrui::Point(23.26, 0.03,-144.31); //otherSide Of bridge
		Vrui::Point rainPosEdge=Vrui::Point(-1.8459, -51.5691,-125.446); //start of river
	//	Vrui::Point rainPosBack=Vrui::Point(34.01, 9.57,-143.11); //Back area
		//Vrui::Point rainPosLeftBridge=Vrui::Point(-0.55, 5.1,-136.34); //Back area
		
		//Vrui::Point rainPos=Vrui::getInverseNavigationTransformation().transform(getButtonDevicePosition(0));
		Vrui::Scalar rainRadius=Vrui::getPointPickDistance()*Vrui::Scalar(1);
		
		/* Render the rain disk: */
		Vrui::Vector z=application->waterTable->getBaseTransform().inverseTransform(Vrui::Vector(0,0,1));
		Vrui::Vector x=Geometry::normal(z);
		Vrui::Vector y=Geometry::cross(z,x);
		x*=rainRadius/Geometry::mag(x);
		y*=rainRadius/Geometry::mag(y);
		
		glVertexAttrib1fARB(1,adding/application->waterSpeed);


		//Start audio
//ERIC:TODO move to different location or button press
		// if(system("pgrep xplayer")) 
		// {
		// 	int tst = system("xdg-open ./etc/FloodAudio.mp3");
		// }

		glBegin(GL_POLYGON);
		for(int i=0;i<3000;++i)//750
			{
			Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
			glVertex(rainPosMain+x*Math::cos(angle)+y*Math::sin(angle));
			}
		glEnd();
		// glBegin(GL_POLYGON);
		// for(int i=0;i<250;++i)//150
		// 	{
		// 		Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
		// 		glVertex(rainPosBridge+x*Math::cos(angle)+y*Math::sin(angle));
		// 	}
		// glEnd();


		// glBegin(GL_POLYGON);
		// for(int i=0;i<850;++i)
		// 	{
		// 		Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
		// 		glVertex(rainPosBack+x*Math::cos(angle)+y*Math::sin(angle));

		// 	}
		// glEnd();

		glBegin(GL_POLYGON);
		for(int i=0;i<2000;++i)//550
			{
				Vrui::Scalar angle=Vrui::Scalar(2)*Math::Constants<Vrui::Scalar>::pi*Vrui::Scalar(i)/Vrui::Scalar(32);
				glVertex(rainPosEdge+x*Math::cos(angle)+y*Math::sin(angle));

			}
		glEnd();
		
		glPopAttrib();
		}
	}
