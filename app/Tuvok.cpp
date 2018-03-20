#include "Tuvok.h"

#include <cstdlib>
#include <iostream>

using namespace std;
using namespace tuvok;

unsigned int getTime() {
	struct timeval tp;
	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000 + tp.tv_usec / 1000);
}

Tuvok::Tuvok(string filename, bool stereo): mInitialized(false), mPrevTime(0), mIdleTime(0),
		mCamThreshold(0), mStereo(stereo)
{
	mPrevCamPos = FLOATVECTOR3(0, 0, 0);
	mFilename = filename;
}
	
Tuvok::~Tuvok()
{
	if(mInitialized) {
		std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
		ss->cexec(mLuaAbstrRenderer.fqName() + ".cleanup");
	    Controller::Instance().ReleaseVolumeRenderer(mLuaAbstrRenderer);
	}
}
	
void Tuvok::init(int width, int height) {

	if(mInitialized)
		return;

	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
    mLuaAbstrRenderer = ss->cexecRet<LuaClassInstance>(
        "tuvok.renderer.new",
        //MasterController::OPENGL_SBVR, false, false, false, false);
        MasterController::OPENGL_RAYCASTER, false, false, false, false);
	mRenderer = mLuaAbstrRenderer.getRawPointer<AbstrRenderer>(ss);

    ss->cexec(mLuaAbstrRenderer.fqName() + ".loadDataset", mFilename);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".addShaderPath", "Shaders");
    ss->cexec(mLuaAbstrRenderer.fqName() + ".initialize", GLContext::Current(0));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".resize", UINTVECTOR2(width, height));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setRendererTarget",AbstrRenderer::RT_INTERACTIVE); 
    if(mStereo) {
    	ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoEnabled",true);
    	ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoMode",AbstrRenderer::SM_SCANLINE);
    }
    
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setConsiderPrevDepthBuffer", false);

    mInitialized = true;
}

void Tuvok::swapEye() {
	if(!mInitialized)
		return;
	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
	bool b = ss->cexecRet<bool>(mLuaAbstrRenderer.fqName() + ".getStereoEyeSwap");
	ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoEyeSwap",!b);
}

void Tuvok::render(const float MVLeft[16], const float PLeft[16], 
					const float MVRight[16], const float PRight[16], const float campos[3]) {

	if(!mInitialized)
		return;

	bool lowmode = true;
	FLOATVECTOR3 currCamPos = FLOATVECTOR3(campos[0], campos[1], campos[2]);
	float len = (currCamPos - mPrevCamPos).sqLength();
	//cout << len << " " << std::flush;
	if(len > mCamThreshold || mPrevTime == 0) {
		mPrevTime = getTime();
		mIdleTime = 0;
	}
	else {
		mIdleTime += getTime() - mPrevTime;
		if(mIdleTime > 1000)
			lowmode = false;
	}

	mPrevCamPos = currCamPos;

	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
	
	if(lowmode) {
		FLOATMATRIX4 mvLeft = FLOATMATRIX4(MVLeft);
		FLOATMATRIX4 pLeft = FLOATMATRIX4(PLeft);
		if(mStereo) {
			FLOATMATRIX4 mvRight = FLOATMATRIX4(MVRight);
			FLOATMATRIX4 pRight = FLOATMATRIX4(PRight);
			ss->cexec(mLuaAbstrRenderer.fqName() + ".setUserMatrices", mvLeft, pLeft, 
						mvLeft, pLeft, mvRight, pRight);
		}
		else {
			ss->cexec(mLuaAbstrRenderer.fqName() + ".setUserMatrices", mvLeft, pLeft, 
						mvLeft, pLeft, mvLeft, pLeft);
		}
	}
	
	ss->cexec(mLuaAbstrRenderer.fqName() + ".paint");
}

