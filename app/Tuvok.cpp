#include "Tuvok.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

#include "cJSON.h"

using namespace std;
using namespace tuvok;

unsigned int getTime() {
	struct timeval tp;
	gettimeofday(&tp, 0);
	return (tp.tv_sec * 1000 + tp.tv_usec / 1000);
}

Tuvok::Tuvok(string filename, bool stereo, string jsonfile): mInitialized(false), mPrevTime(0), mIdleTime(0),
		mCamThreshold(0), mStereo(stereo), mColormap(0), mAlphaFactor(50)
{
	mPrevCamPos = FLOATVECTOR3(0, 0, 0);
	mFilename = filename;
	cout << "input file: " << filename << endl;

	if(jsonfile.length()> 0) 
		loadJsonOption(jsonfile);
}
	
Tuvok::~Tuvok()
{
	if(mInitialized) {
		std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
		ss->cexec(mLuaAbstrRenderer.fqName() + ".cleanup");
	    Controller::Instance().ReleaseVolumeRenderer(mLuaAbstrRenderer);

	    if(mColormap)
	    	delete mColormap;
	}
}
	
void Tuvok::init(int width, int height) {

	if(mInitialized)
		return;

	std::shared_ptr<LuaScripting> ss = Controller::Instance().LuaScript();
    mLuaAbstrRenderer = ss->cexecRet<LuaClassInstance>(
        "tuvok.renderer.new",
        //MasterController::OPENGL_RAYCASTER, false, false, false, false);
        MasterController::OPENGL_RAYCASTER_LAVA, false, false, false, false);
	mRenderer = mLuaAbstrRenderer.getRawPointer<AbstrRenderer>(ss);

    ss->cexec(mLuaAbstrRenderer.fqName() + ".loadDataset", mFilename);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".addShaderPath", "Shaders");
    ss->cexec(mLuaAbstrRenderer.fqName() + ".initialize", GLContext::Current(0));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".resize", UINTVECTOR2(width, height));
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setRendererTarget",AbstrRenderer::RT_INTERACTIVE); 

    ss->cexec(mLuaAbstrRenderer.fqName() + ".setCoordinateArrowsEnabled", true); 
    
    if(mStereo) {
    	ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoEnabled",true);
    	ss->cexec(mLuaAbstrRenderer.fqName() + ".setStereoMode",AbstrRenderer::SM_SCANLINE);
    }
    
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setConsiderPrevDepthBuffer", false);

    //setup rendering parameters 
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaIsoValue", mOption.lavaIsoValue);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaIsoWalls", mOption.lavaIsoWalls);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaIsoAlpha", mOption.lavaIsoAlpha);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaIsoSmooth", mOption.lavaIsoSmooth);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaIsoColor", mOption.lavaIsoColor);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaDensity", mOption.lavaDensity);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaPower", mOption.lavaPower);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaDensityMin", mOption.lavaDensityMin);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaDensityMax", mOption.lavaDensityMax);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaBrightness", mOption.lavaBrightness);
    ss->cexec(mLuaAbstrRenderer.fqName() + ".setLavaContrast", mOption.lavaContrast);

    //set colormap
    // Retrieve color data from Lua script class.
    LuaClassInstance trans = ss->cexecRet<LuaClassInstance>(mLuaAbstrRenderer.fqName() + ".get1DTrans");
  	shared_ptr<vector<FLOATVECTOR4> > cdata = ss->cexecRet<shared_ptr<vector<FLOATVECTOR4> > >(
          													trans.fqName() + ".getColorData");

  	if(mColormap) {
  		vector<FLOATVECTOR4> colors = mColormap->getColors();
  		for(int i=0; i < colors.size(); i++) {
  			(*cdata)[i] = colors[i];
  			(*cdata)[i][3] = colors[i][3] / mAlphaFactor;
  		}
  		// perform update
  		ss->cexec("tuvok.gpu.changed1DTrans", LuaClassInstance(), trans);
  	}

  	cout << "Trans1 size: " << (*cdata).size() << endl;
  	for(int i=0; i < 5; i++) {
  		cout << i << ": " << (*cdata)[i][0] << " " << (*cdata)[i][1] << " " << (*cdata)[i][2] << " " << (*cdata)[i][3] << endl;
  	}

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
					const float MVRight[16], const float PRight[16], 
					const float campos[3], bool alwayUpdate) {

	if(!mInitialized)
		return;

	bool lowmode = true;
	FLOATVECTOR3 currCamPos;
	if(!alwayUpdate) {
		currCamPos = FLOATVECTOR3(campos[0], campos[1], campos[2]);
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


// cJSON wrappers
static string getJsonItemString(cJSON* js, string item, string default_value="") {
    cJSON* i = cJSON_GetObjectItem(js, item.c_str());
    if(i)
        return i->valuestring;
    return default_value;
}

static double getJsonItemDouble(cJSON* js, string item, double default_value=0) {
    cJSON* i = cJSON_GetObjectItem(js, item.c_str());
    if(i)
        return i->valuedouble;
    return default_value;
}

static int getJsonItemInt(cJSON* js, string item, int default_value=0) {
    cJSON* i = cJSON_GetObjectItem(js, item.c_str());
    if(i)
        return i->valueint;
    return default_value;
}

void Tuvok::loadJsonOption(string jsonfile) {
	FILE *f;long len;char *data;
    f=fopen(jsonfile.c_str(),"rb");
    if(f == NULL){
        std::cout << "Cannot load file " << jsonfile << " default option is used instead!" << std::endl;
        return;
    }
    fseek(f,0,SEEK_END);len=ftell(f);fseek(f,0,SEEK_SET);
    data= new char[len+1];fread(data,1,len,f);fclose(f);

    cJSON *json;
    json=cJSON_Parse(data);

    if (!json) {
        cout << "Error before:" << endl << cJSON_GetErrorPtr() << endl;
    }
    else {

    	cout << "Load option from file: " << jsonfile << endl;
    	
		cJSON* objects = cJSON_GetObjectItem(json, "objects");
		int n = cJSON_GetArraySize(objects);
		if (n != 1) {
			cout << "WARNING: size of objects != 1" << endl;
			cJSON_Delete(json);
    		delete [] data;
			return;
		}
		cJSON* vol = cJSON_GetArrayItem(objects, 0);

		mOption.lavaIsoValue = getJsonItemDouble(vol, "isovalue", mOption.lavaIsoValue);

		mOption.lavaIsoWalls = getJsonItemInt(vol, "isowalls", 1);

		mOption.lavaIsoAlpha = getJsonItemDouble(vol, "isoalpha", mOption.lavaIsoAlpha);

		mOption.lavaIsoSmooth = getJsonItemDouble(vol, "isosmooth", mOption.lavaIsoSmooth);

		cJSON* color = cJSON_GetObjectItem(vol, "color");
		if(color) {
			mOption.lavaIsoColor = FLOATVECTOR3( cJSON_GetArrayItem(color, 0)->valuedouble,
					cJSON_GetArrayItem(color, 1)->valuedouble, cJSON_GetArrayItem(color, 2)->valuedouble);
			mOption.lavaIsoColor = mOption.lavaIsoColor / 255;
		}

		mOption.lavaDensity = getJsonItemDouble(vol, "density", mOption.lavaDensity);

		mOption.lavaPower = getJsonItemDouble(vol, "power", mOption.lavaPower);

		mOption.lavaDensityMin = getJsonItemDouble(vol, "densitymin", mOption.lavaDensityMin);	
		mOption.lavaDensityMax = getJsonItemDouble(vol, "densitymax", mOption.lavaDensityMax);		

		mOption.lavaBrightness = getJsonItemDouble(vol, "brightness", mOption.lavaBrightness);
		mOption.lavaBrightness = mOption.lavaBrightness < 0 ? 0 : mOption.lavaBrightness;

		mOption.lavaContrast = getJsonItemDouble(vol, "contrast", mOption.lavaContrast);

		//color map
		mColormap = new ColorMap();
		cJSON* colormaps = cJSON_GetObjectItem(json, "colourmaps");
		cJSON* colorsParent = cJSON_GetArrayItem(colormaps, 0);
		cJSON* colors = cJSON_GetObjectItem(colorsParent, "colours");
		int numColors = cJSON_GetArraySize(colors);
		for(int i = 0; i < numColors; i++) {
			cJSON* color = cJSON_GetArrayItem(colors, i);
			float pos = getJsonItemDouble(color, "position");
			string str = getJsonItemString(color, "colour");

			float r, g, b, a;
			istringstream iss(str);
			string token;
			std::getline(iss, token, '(');
			std::getline(iss, token, ',');
			r = (float)std::stoi(token) / 255.0;
			std::getline(iss, token, ',');
			g = (float)std::stoi(token) / 255.0;
			std::getline(iss, token, ',');
			b = (float)std::stoi(token) / 255.0;
			std::getline(iss, token, ',');
			a = stof(token);
			//cout << r << " " << g << " " << b << " " << a << endl;
			mColormap->addAt(FLOATVECTOR4(r,g,b,a), pos);
		}
		mColormap->calc();
    }

    cJSON_Delete(json);
    delete [] data;
}

void Tuvok::printOption() {
	cout << "======= Option =======" << endl;
	cout << "iso value: " << mOption.lavaIsoValue << endl; 
	cout << "iso walls: " << mOption.lavaIsoWalls << endl;
	cout << "iso alpha: " << mOption.lavaIsoAlpha << endl;
	cout << "iso smooth: " << mOption.lavaIsoSmooth << endl;
	cout << "iso color: " << mOption.lavaIsoColor[0] << " " << mOption.lavaIsoColor[1] << " " << mOption.lavaIsoColor[2] << endl;
	cout << "density: " << mOption.lavaDensity << endl;
	cout << "power: " << mOption.lavaPower << endl;
	cout << "density min: " << mOption.lavaDensityMin << endl;
	cout << "density max: " << mOption.lavaDensityMax << endl;
	cout << "brightness: " << mOption.lavaBrightness << endl;
	cout << "contrast: " <<	mOption.lavaContrast << endl;
}


/////////////  COLORMAP class ///////////////
#include <float.h>
#define LOG10(val) (val > FLT_MIN ? log10(val) : log10(FLT_MIN))

ColorMap::ColorMap(): mLog(false), mRange(1), mMinimum(0), mMaximum(1), mSamples(256) 
{
	mColors.resize(mSamples);
	mPrecalc.resize(mSamples);
}

ColorMap::~ColorMap()
{
	mColors.clear();
}

void ColorMap::addAt(FLOATVECTOR4 color, float pos) {
	ColorItem item(color, pos);
	mColors.push_back(item);
  	mColors[mColors.size()-1].value = HUGE_VAL;
  	mColors[mColors.size()-1].position = pos;
}

void ColorMap::calc() {
	if (!mColors.size()) return;

	if (mLog)
    	mRange = LOG10(mMaximum) - LOG10(mMinimum);
  	else
    	mRange = mMaximum - mMinimum;

	if (mLog)
		for (int cv=0; cv<mSamples; cv++)
			mPrecalc[cv] = get(pow(10, log10(mMinimum) + mRange * (float)cv/(mSamples-1)));
	else
		for (int cv=0; cv<mSamples; cv++)
			mPrecalc[cv] = get(mMinimum + mRange * (float)cv/(mSamples-1));
}

FLOATVECTOR4 ColorMap::get(float value) {
	return getFromScaled(scaleValue(value));
}

float ColorMap::scaleValue(float value) {

	float min = mMinimum, max = mMaximum;

	if (max == min) return 0.5;   // Force central value if no range
	if (value <= min) return 0.0;
	if (value >= max) return 1.0;

	if (mLog)
	{
		value = LOG10(value);
		min = LOG10(mMinimum);
		max = LOG10(mMaximum);
	}

	//Scale to range [0,1]
	return (value - min) / (max - min);
}
	
FLOATVECTOR4 ColorMap::getFromScaled(float scaledValue) {
	//printf(" scaled %f ", scaledValue);
	if (mColors.size() == 0) return FLOATVECTOR4(0, 0, 0, 0);
	if (mColors.size() == 1) return mColors[0].color;
	// Check within range
	if (scaledValue >= 1.0)
		return mColors.back().color;

	else if (scaledValue >= 0) {
		// Find the color/values our value lies between
		unsigned int i;
		for (i=0; i<mColors.size(); i++)
		{
	  		if (fabs(mColors[i].position - scaledValue) <= FLT_EPSILON)
	    		return mColors[i].color;

	  		if (mColors[i].position > scaledValue) break;
		}

		if (i==0 || i==mColors.size())  {
			cout << "Color position is not in range" << endl;
			exit(1);
		}

		// Calculate interpolation factor [0,1] between color at index and previous color
		float interpolate = (scaledValue - mColors[i-1].position) / (mColors[i].position - mColors[i-1].position);

		//Linear interpolation between mColors
		//printf(" interpolate %f above %f below %f\n", interpolate, mColors[i].position, mColors[i-1].position);
		FLOATVECTOR4 colour0, colour1;
		colour0 = mColors[i-1].color;
		colour1 = mColors[i].color;

		for (int c=0; c<4; c++)
	  		colour0[c] += (colour1[c] - colour0[c]) * interpolate;

		return colour0;
	}

	return FLOATVECTOR4(0, 0, 0, 0);
}
