#ifndef __TUVOK__H
#define __TUVOK__H

//#include "FrameBuffer.h"

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

#include <string>

class Tuvok {

public:
	Tuvok(std::string filename, bool stereo=true);
	~Tuvok();

	void init(int width, int height);
	void loadDataset(std::string filename);
	void render(const float MVLeft[16], const float PLeft[16], 
				const float MVRight[16], const float PRight[16], const float campos[3]);

	void setCamThreshold(float val) { mCamThreshold = val; }
	bool isStereo() { return mStereo; }
	void swapEye();

private:
	bool mInitialized;
	std::string mFilename;
	bool mStereo;

	AbstrRenderer* ren;
	LuaClassInstance mLuaAbstrRenderer;
	AbstrRenderer*   mRenderer;

	float mCamThreshold;
	FLOATVECTOR3 mPrevCamPos;
	unsigned int mPrevTime;
	unsigned int mIdleTime;
};


#endif
