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

    void initTuvok()
    {
        tv = new Tuvok();
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
            glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);
            client->getRenderer()->beginDraw3D(context);

    	    if(module->visible)
    	    { 
                if(module->tv) {
                    module->tv->init(context.viewport.width(), context.viewport.height());

                    float* MV = context.modelview.cast<float>().data();
                    float* P = context.projection.cast<float>().data();
                    module->tv->render(MV, P);
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
    ;

    def("initialize", initialize, PYAPI_RETURN_REF);
}
