uniform sampler2D texRayExit; ///< the backface (or ray exit point) texture in texcoords
uniform sampler2D texRayExitPos; ///< the backface (or ray exit point) texture in eyecoords
uniform float fTransScale;       ///< scale for 1D Transfer function lookup
uniform float fStepScale;        ///< opacity correction quotient
uniform vec3 vVoxelStepsize;     ///< Stepsize (in texcoord) to get to the next voxel
uniform vec2 vScreensize;        ///< the size of the screen in pixels
uniform float fRayStepsize;      ///< stepsize along the ray
uniform vec3 vLightAmbient;
uniform vec3 vLightDiffuse;
uniform vec3 vLightSpecular;
uniform vec3 vLightDir;
uniform vec3 vDomainScale;

//lava
uniform sampler1D texTrans; ///< the 1D Transfer function
uniform float fLavaIsoValue;
uniform bool bLavaIsoWalls;
uniform float fLavaIsoSmooth;
uniform vec4 vLavaIsoColor;
uniform float fLavaDensity;
uniform float fLavaPower;
uniform float fLavaDensityMin;
uniform float fLavaDensityMax;
uniform float fLavaBrightness;
uniform float fLavaContrast;

varying vec3 vEyePos;

  
vec4 sampleVolume(vec3 coords);

vec3 ComputeNormal(vec3 vCenter, vec3 StepSize, vec3 DomainScale);
vec3 Lighting(vec3 vPosition, vec3 vNormal, vec3 vLightAmbient,
              vec3 vLightDiffuse, vec3 vLightSpecular, vec3 vLightDir);

vec4 UnderCompositing(vec4 src, vec4 dst);


vec4 bit_width(const vec3 tex_pos, const float tf_scale)
{
  float fVolumVal = sampleVolume(tex_pos).x;
  return texture1D(texTrans, fVolumVal * tf_scale);
}


void main(void)
{
  // compute the coordinates to look up the previous pass
  vec2 vFragCoords = vec2(gl_FragCoord.x / vScreensize.x , gl_FragCoord.y / vScreensize.y);

  // compute the ray parameters
  vec3  vRayEntry    = texture2D(texRayExitPos, vFragCoords).xyz;
  vec3  vRayExit     = vEyePos;

  vec3  vRayEntryTex = (gl_TextureMatrix[0] * vec4(vRayEntry,1.0)).xyz;
  vec3  vRayExitTex  = (gl_TextureMatrix[0] * vec4(vRayExit,1.0)).xyz;
  vec3  vRayDir      = vRayExit - vRayEntry;
  
  float fRayLength = length(vRayDir);
  vRayDir /= fRayLength;

  // compute the maximum number of steps before the domain is left
  int iStepCount = int(fRayLength/fRayStepsize)+1;

  vec3  fRayInc    = vRayDir*fRayStepsize;
  vec3  vRayIncTex = (vRayExitTex-vRayEntryTex)/(fRayLength/fRayStepsize);

  // do the actual raycasting
  vec4  vColor = vec4(0.0,0.0,0.0,0.0);
  vec3  vCurrentPosTex = vRayEntryTex;
  vec3  vCurrentPos    = vRayEntry;

  bool inside = false;
  float stepSize = 1.732 / 256;//fLavaSamples;
  float T = 1.0;

  for (int i = 0;i<iStepCount;i++) {

    if(T < 0.01) break;

    //vec4 lut_v = bit_width(vCurrentPosTex, fTransScale);
    float density = sampleVolume(vCurrentPosTex).x;

    if(vLavaIsoColor.a > 0.0 && 
      ( (!inside && density >= fLavaIsoValue) || (inside && density < fLavaIsoValue) )) {

      inside = !inside;
      //find closer to exact position by iteration
      float exact;
      float a = i + 0.5;
      float b = a - 1.0;
      vec3 pos, posTex;
      for(int j=0; j < 5; j++) {
        exact = (b+a) * 0.5;
        pos = vRayEntry + exact * fRayInc;
        posTex = vRayEntryTex + exact * vRayIncTex;
        density = sampleVolume(posTex).x;
        if(density - fLavaIsoValue < 0.0)
          b = exact;
        else 
          a = exact;
      }

      if(bLavaIsoWalls) {
        vec4 value = vec4(vLavaIsoColor.rgb, 1.0);
        vec3 normal = ComputeNormal(vCurrentPosTex, fLavaIsoSmooth*vVoxelStepsize, vDomainScale);
        vec3 light_color = Lighting(pos, normal, vLightAmbient,
                              vLightDiffuse * value.xyz, vLightSpecular, vLightDir);
        vec4 stepColor = vec4(light_color.x, light_color.y, light_color.z, vLavaIsoColor.a);
        vColor = UnderCompositing(stepColor, vColor);
        //vColor.rgb += T * vLavaIsoColor.a * light_color.rgb;
        T *= (1.0 - vLavaIsoColor.a);
      }// bLavaIsoWalls

      //if (vColor.a >= 0.99) break;
    }

    if(fLavaDensity > 0.0) {

      //density = (density - uRange.x) / range;
      //density = clamp(density, 0.0, 1.0);

      if(density < fLavaDensityMin || density > fLavaDensityMax) {
        vCurrentPos    += fRayInc;
        vCurrentPosTex += vRayIncTex;
        continue;
      }
      density = pow(density, fLavaPower); 

      vec4 lut_v = texture1D(texTrans, density * fTransScale);
      
      vec3 normal = ComputeNormal(vCurrentPosTex, vVoxelStepsize, vDomainScale);
      vec3 light_color = Lighting(vCurrentPos, normal, vLightAmbient,
                              vLightDiffuse * lut_v.xyz, vLightSpecular, vLightDir);
      // opacity correction
      lut_v.a = 1.0 - pow(1.0 - lut_v.a, fStepScale);
      vec4 stepColor = vec4(light_color.x, light_color.y, light_color.z, lut_v.a);
      
      // lavaVR shader
      //stepColor *= fLavaDensity * stepSize; //length(fRayInc);
      //vColor.rgb += T * stepColor.rgb;

      vColor = UnderCompositing(stepColor, vColor);
      T *= (1.0 - stepColor.a);
    }
    
    vCurrentPos    += fRayInc;
    vCurrentPosTex += vRayIncTex;

    //if (vColor.a >= 0.99) break;
  }

  // apply brightness, saturation & contrast as in LavaVR shader
  vColor += fLavaBrightness;
  const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);
  vec3 AvgLumin = vec3(0.5, 0.5, 0.5);
  vec3 intensity = vec3(dot(vColor.rgb, LumCoeff));
  vColor.rgb = mix(intensity, vColor.rgb, 1); //uSaturation);
  vColor.rgb = mix(AvgLumin, vColor.rgb, fLavaContrast);
  vColor.a = 1.0 - T;

  gl_FragColor = vColor;
}
