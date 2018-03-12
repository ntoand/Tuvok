#ifndef __TUVOK__H
#define __TUVOK__H

#include <StdTuvokDefines.h>
#include <Basics/SysTools.h>
#include <Controller/Controller.h>
#include <IO/IOManager.h>
#include <Renderer/AbstrRenderer.h>
#include <Renderer/ContextIdentification.h>
#include <Renderer/GL/GLContext.h>
#include <Renderer/GL/GLFBOTex.h>
#include <Renderer/GL/GLFrameCapture.h>
#include <Renderer/GL/GLRenderer.h>
#include <LuaScripting/LuaScripting.h>
#include <LuaScripting/TuvokSpecific/LuaTuvokTypes.h>

using namespace tuvok;

class Tuvok {

public:
	Tuvok();
	~Tuvok();

	void init(int width, int height);
	void render(const float MV[16], const float P[16]);
	void capture();

private:
	bool mInitialized;
	AbstrRenderer* ren;
	bool mCapture;

	LuaClassInstance mLuaAbstrRenderer;
	AbstrRenderer*   mRenderer;


};


#endif