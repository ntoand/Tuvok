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
#include <vector>

struct ColorItem {
	FLOATVECTOR4 color;
	float position;
	float value;
	ColorItem(): value(0), position(0) {}
	ColorItem(FLOATVECTOR4 c, float pos): value(0) {
		color = c;
		position = pos;
	}
};

class ColorMap {

private:
	std::vector<ColorItem> mColors;
	bool mLog;
	float mRange;
	float mMinimum, mMaximum;
	int mSamples;

	std::vector<FLOATVECTOR4> mPrecalc;

public:
	ColorMap();
	~ColorMap();

	void addAt(FLOATVECTOR4 color, float pos);
	void calc();
	std::vector<FLOATVECTOR4> getColors() { return mPrecalc; }

private:
	float scaleValue(float value);
	FLOATVECTOR4 getFromScaled(float scaledValue);
	FLOATVECTOR4 get(float value);

};

struct OPTION {
	float lavaIsoValue;
    bool lavaIsoWalls;
    float lavaIsoAlpha;
    float lavaIsoSmooth;
    FLOATVECTOR3 lavaIsoColor;
    float lavaDensity;
    float lavaPower;
    float lavaDensityMin;
    float lavaDensityMax;
    float lavaBrightness;
    float lavaContrast;
    OPTION() {
    	lavaIsoValue = 0.45; 
		lavaIsoWalls = true;
		lavaIsoAlpha = 1;
		lavaIsoSmooth = 0.5045783843331688;
		lavaIsoColor = FLOATVECTOR3(1, 0.9626, 0.89);
		lavaDensity = 5;
		lavaPower = 1;
		lavaDensityMin = 0;
		lavaDensityMax = 1;
		lavaBrightness = 0;
		lavaContrast = 1;
    }
};


class Tuvok {

public:
	Tuvok(std::string filename, bool stereo=false, std::string jsonfile="");
	~Tuvok();

	void init(int width, int height);
	void loadDataset(std::string filename);
	void render(const float MVLeft[16], const float PLeft[16], 
				const float MVRight[16], const float PRight[16], 
				const float campos[3], bool alwayUpdate = false);

	void setCamThreshold(float val) { mCamThreshold = val; }
	bool isStereo() { return mStereo; }
	void swapEye();
	void setAlphaFactor(float val) { mAlphaFactor = val; }

	void printOption();

private:
	void loadJsonOption(std::string filename);

private:
	OPTION mOption;
	ColorMap* mColormap;
	bool mInitialized;
	std::string mFilename;
	bool mStereo;
	float mAlphaFactor;

	AbstrRenderer* ren;
	LuaClassInstance mLuaAbstrRenderer;
	AbstrRenderer*   mRenderer;

	float mCamThreshold;
	FLOATVECTOR3 mPrevCamPos;
	unsigned int mPrevTime;
	unsigned int mIdleTime;
};


#endif
