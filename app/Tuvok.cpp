#include "Tuvok.h"

#include <cstdlib>
#include <iostream>

using namespace std;
using namespace tuvok;

Tuvok::Tuvok(): mInitialized(false), mCapture(false), mUpdate(true)
{

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

	string uvf_file = "data/foot_tiffs_2048_3.uvf";

	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
    mLuaAbstrRenderer = ss->cexecRet<LuaClassInstance>(
        "tuvok.renderer.new",
        //MasterController::OPENGL_SBVR, false, false, false, false);
        MasterController::OPENGL_RAYCASTER, false, false, false, false);
	mRenderer = mLuaAbstrRenderer.getRawPointer<AbstrRenderer>(ss);

    ss->cexec(mLuaAbstrRenderer.fqName() + ".loadDataset", uvf_file);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".addShaderPath", "Shaders");
    ss->cexec(mLuaAbstrRenderer.fqName() + ".initialize", GLContext::Current(0));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".resize", UINTVECTOR2(width, height));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setRendererTarget",AbstrRenderer::RT_INTERACTIVE); 
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoEnabled",false);  

    //ss->cexec(mLuaAbstrRenderer.fqName() + ".paint");

    ss->cexec(mLuaAbstrRenderer.fqName() + ".setConsiderPrevDepthBuffer", false);
    
    mInitialized = true;
}

void Tuvok::capture() {
	mCapture = true;
}

void Tuvok::render(const float MV[16], const float P[16]) {
	if(!mInitialized)
		return;

	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
	//ss->cexec(mLuaAbstrRenderer.fqName() + ".setViewPos", FLOATVECTOR3(pos[0], pos[1], pos[2]));
	//ss->cexec(mLuaAbstrRenderer.fqName() + ".setViewDir", FLOATVECTOR3(dir[0], dir[1], dir[2]));
	FLOATMATRIX4 mv_mat = FLOATMATRIX4(MV);
	FLOATMATRIX4 p_mat = FLOATMATRIX4(P);
	if(mUpdate) {
		ss->cexec(mLuaAbstrRenderer.fqName() + ".setUserMatrices", mv_mat, p_mat, mv_mat, p_mat, mv_mat, p_mat);
		mUpdate = false;	
	}
	//ss->cexec(mLuaAbstrRenderer.fqName() + ".scheduleCompleteRedraw");
	ss->cexec(mLuaAbstrRenderer.fqName() + ".paint");
	
	if(mCapture) {
		cout << "Capture a frame to test.png" << endl;
		GLRenderer* glren = dynamic_cast<GLRenderer*>(mRenderer);
		GLFBOTex* fbo = glren->GetLastFBO();
		GLTargetBinder bind(&Controller::Instance());

		GLFrameCapture fc;
		fc.CaptureSingleFrame("test.png", fbo);


		FLOATVECTOR3 center = ss->cexecRet<FLOATVECTOR3>(mLuaAbstrRenderer.fqName() + ".getVolumeAABBCenter");
		cout << "center: " << center[0] << " " << center[1] << " " << center[2] << endl;
		FLOATVECTOR3 extend = ss->cexecRet<FLOATVECTOR3>(mLuaAbstrRenderer.fqName() + ".getVolumeAABBExtents");
		cout << "extend: " << extend[0] << " " << extend[1] << " " << extend[2] << endl;

		mCapture = false;
	}
}