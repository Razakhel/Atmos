struct Buffers {
  sampler2D depth;
  sampler2D color;
};

in vec2 fragTexcoords;

layout(std140) uniform uboCameraMatrices {
  mat4 viewMat;
  mat4 invViewMat;
  mat4 projectionMat;
  mat4 invProjectionMat;
  mat4 viewProjectionMat;
  vec3 cameraPos;
};

uniform Buffers uniSceneBuffers;
uniform vec3 uniEarthCenter;
uniform float uniEarthRadius;
uniform float uniAtmosphereRadius;
uniform vec3 uniDirToSun;
uniform int uniScatterPointCount;
uniform int uniOpticalDepthSampleCount;
uniform float uniDensityFalloff;
uniform vec3 uniScatteringCoeffs;

layout(location = 0) out vec4 fragColor;

vec3 computeViewPosFromDepth(vec2 texcoords, float depth) {
  vec4 projPos = vec4(vec3(texcoords, depth) * 2.0 - 1.0, 1.0);
  vec4 viewPos = invProjectionMat * projPos;

  return viewPos.xyz / viewPos.w;
}

struct RaySphereInfo {
  float distToSphere;
  float distThroughSphere;
};

#define MAX_FLOAT 3.402823466e+38

RaySphereInfo computeRaySphereIntersection(vec3 sphereCenter, float sphereRadius, vec3 rayOrigin, vec3 rayDir) {
  vec3 offset = rayOrigin - sphereCenter;

  // Quadratic equation: b² - 4ac
  // Here, 'a' is simply 1 since we assume rayDir to be normalized, hence its length should be equal to 1
  float b = 2.0 * dot(offset, rayDir);
  float c = dot(offset, offset) - sphereRadius * sphereRadius;
  float discriminant = b * b - 4.0 * c;

  RaySphereInfo res;

  // Intersection count:
  //   - 0 if d  < 0;
  //   - 1 if d == 0
  //   - 2 if d  > 0
  if (discriminant > 0.0) {
    float sqDiscr         = sqrt(discriminant);
    float distToSphereFar = (-b + sqDiscr) / 2.0;

    if (distToSphereFar >= 0.0) {
      float distToSphereNear = max(0.0, (-b - sqDiscr) / 2.0);
      return RaySphereInfo(distToSphereNear, distToSphereFar - distToSphereNear);
    }
  }

  return RaySphereInfo(MAX_FLOAT, 0.0);
}

float computeLocalDensity(vec3 samplePoint) {
  float height       = length(samplePoint - uniEarthCenter) - uniEarthRadius;
  float height01     = height / uniAtmosphereRadius;
  float localDensity = exp(-height01 * uniDensityFalloff) * (1.0 - height01);

  return localDensity;
}

float computeOpticalDepth(vec3 rayOrigin, vec3 rayDir, float rayLength) {
  vec3 samplePoint   = rayOrigin;
  float stepSize     = rayLength / float(uniOpticalDepthSampleCount - 1);
  float opticalDepth = 0.0;

  for (int i = 0; i < uniOpticalDepthSampleCount; ++i) {
    float localDensity = computeLocalDensity(samplePoint);

    opticalDepth += localDensity * stepSize;
    samplePoint  += rayDir * stepSize;
  }

  return opticalDepth;
}

vec3 computeScatteredLight(vec3 rayOrigin, vec3 rayDir, float rayLength, vec3 originalColor) {
  vec3 scatterPoint      = rayOrigin;
  float stepSize         = rayLength / float(uniScatterPointCount - 1);
  float atmosphereExtent = uniEarthRadius + uniAtmosphereRadius;

  vec3 scatteredLight    = vec3(0.0);
  float viewOpticalDepth = 0.0;

  for (int i = 0; i < uniScatterPointCount; ++i) {
    float sunRayLength = computeRaySphereIntersection(uniEarthCenter, atmosphereExtent, scatterPoint, uniDirToSun).distThroughSphere;

    float sunOpticalDepth = computeOpticalDepth(scatterPoint, uniDirToSun, sunRayLength);
    viewOpticalDepth      = computeOpticalDepth(scatterPoint, -rayDir, stepSize * float(i));
    vec3 transmittance    = exp(-(sunOpticalDepth + viewOpticalDepth) * uniScatteringCoeffs);

    float localDensity = computeLocalDensity(scatterPoint);

    scatteredLight += localDensity * transmittance * uniScatteringCoeffs * stepSize;
    scatterPoint   += rayDir * stepSize;
  }

  float originalColorTransmittance = exp(-viewOpticalDepth);
  return originalColor * originalColorTransmittance + scatteredLight;
}

void main() {
  float depth = texture(uniSceneBuffers.depth, fragTexcoords).r;

  vec3 viewPos  = computeViewPosFromDepth(fragTexcoords, depth);
  vec3 worldPos = vec3(invViewMat * vec4(viewPos, 1.0));

  vec3 viewDir = normalize(worldPos - cameraPos);

  float atmosphereExtent = uniEarthRadius + uniAtmosphereRadius;
  RaySphereInfo atmosphereIntersection = computeRaySphereIntersection(uniEarthCenter, atmosphereExtent, cameraPos, viewDir);

  float linearDepth = -viewPos.z;
  float distThroughAtmosphere = min(atmosphereIntersection.distThroughSphere, linearDepth - atmosphereIntersection.distToSphere);

  vec3 color = texture(uniSceneBuffers.color, fragTexcoords).rgb;

  if (distThroughAtmosphere > 0.0) {
    const float epsilon  = 0.0001;
    vec3 atmospherePoint = cameraPos + viewDir * (atmosphereIntersection.distToSphere + epsilon);
    vec3 scatteredLight  = computeScatteredLight(atmospherePoint, viewDir, distThroughAtmosphere - epsilon * 2.0, color);

    fragColor = vec4(scatteredLight, 1.0);
  } else {
    fragColor = vec4(color, 1.0);
  }
}
