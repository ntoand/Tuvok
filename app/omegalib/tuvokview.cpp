#include <omega.h>
#include <omegaGl.h>
#include <iostream>

#include "../Tuvok.h"

using namespace std;
using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class TovokViewRenderModule : public EngineModule
{
public:
    TovokViewRenderModule() :
        EngineModule("TovokViewRenderModule"), tv(0), visible(true)
    {
    	
    }

    virtual void initializeRenderer(Renderer* r);

    virtual void update(const UpdateContext& context)
    {
        // After a frame all render passes had a chance to update their
        // textures. reset the raster update flag.
       
    }
    
    virtual void dispose()
    {
    	if(tv)
            delete tv;
    }

    void initTuvok(string filename, bool stereo)
    {
        tv = new Tuvok(filename, stereo);
    }

    void setCamThreshold(const float val)
    {
        if(tv)
            tv->setCamThreshold(val);
    }

    void swapEye()
    {
        if(tv)
            tv->swapEye();
    }

    Tuvok* tv;
    bool visible;
};

///////////////////////////////////////////////////////////////////////////////
class TuvokViewRenderPass : public RenderPass
{
public:
    TuvokViewRenderPass(Renderer* client, TovokViewRenderModule* prm) : 
        RenderPass(client, "TuvokViewRenderPass"), 
        module(prm) {}
    
    virtual void initialize()
    {
        RenderPass::initialize();
    }

    virtual void render(Renderer* client, const DrawContext& context)
    {
    	if(context.task == DrawContext::SceneDrawTask)
        {
            glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );
                // | GL_PROJECTION | GL_MODELVIEW);
            client->getRenderer()->beginDraw3D(context);

    	    if(module->visible)
    	    { 
                if(module->tv) {
                    module->tv->init(context.viewport.width(), context.viewport.height());

                    Camera* cam = context.camera;

                    // camera pos
                    Vector3f cp = context.camera->getPosition();
                    float campos[3] = {cp[0], cp[1], cp[2]};

                    if(module->tv->isStereo()) {
                        // calculate view and projection matrices for left eye
                        DrawContext tmp = context;
                        tmp.eye = DrawContext::EyeLeft;
                        tmp.updateTransforms(cam->getHeadTransform(), cam->getViewTransform(), 
                            cam->getEyeSeparation(), cam->getNearZ(), cam->getFarZ());
                        float* MVLeft = tmp.modelview.cast<float>().data();
                        float* PLeft = tmp.projection.cast<float>().data();

                        // calculate view and projection matrices for right eye
                        DrawContext tmp2 = context;
                        tmp2.eye = DrawContext::EyeRight;
                        tmp2.updateTransforms(cam->getHeadTransform(), cam->getViewTransform(), 
                            cam->getEyeSeparation(), cam->getNearZ(), cam->getFarZ());
                        float* MVRight = tmp2.modelview.cast<float>().data();
                        float* PRight = tmp2.projection.cast<float>().data();

                        module->tv->render(MVLeft, PLeft, MVRight, PRight, campos);
                    }
                    else {
                        float* MV = context.modelview.cast<float>().data();
                        float* P = context.projection.cast<float>().data();
                        module->tv->render(MV, P, MV, P, campos);
                    }

                }
    			
    		    if(oglError) return;
    	    }
            
            client->getRenderer()->endDraw();
            glPopAttrib();
        }
        
    }

private:
    TovokViewRenderModule* module;

};

///////////////////////////////////////////////////////////////////////////////
void TovokViewRenderModule::initializeRenderer(Renderer* r)
{
    r->addRenderPass(new TuvokViewRenderPass(r, this));
}

///////////////////////////////////////////////////////////////////////////////
TovokViewRenderModule* initialize()
{
    TovokViewRenderModule* prm = new TovokViewRenderModule();
    ModuleServices::addModule(prm);
    prm->doInitialize(Engine::instance());
    return prm;
}

///////////////////////////////////////////////////////////////////////////////
// Python API
#include "omega/PythonInterpreterWrapper.h"
BOOST_PYTHON_MODULE(tuvokview)
{
    //
    PYAPI_REF_BASE_CLASS(TovokViewRenderModule)
    PYAPI_METHOD(TovokViewRenderModule, initTuvok)
    PYAPI_METHOD(TovokViewRenderModule, setCamThreshold)
    PYAPI_METHOD(TovokViewRenderModule, swapEye)
    ;

    def("initialize", initialize, PYAPI_RETURN_REF);
}
