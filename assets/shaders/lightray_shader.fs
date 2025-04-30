#version 330 core

uniform sampler2D uOcclusionTexture;
in vec2 vUv;

uniform vec2 uScreenSpaceSunPos;

uniform float uDensity;
uniform float uWeight;
uniform float uDecay;
uniform float uExposure;
uniform int uNumSamples;

vec3 godrays(
        float density,
        float weight,
        float decay,
        float exposure,
        int numSamples,
        sampler2D occlusionTexture,
        vec2 screenSpaceLightPos,
        vec2 uv
        ) {
    vec3 fragColor = vec3(0.0,0.0,0.0);

    vec2 deltaTextCoord = vec2( uv - screenSpaceLightPos.xy );

    vec2 textCoo = uv.xy ;
    deltaTextCoord *= (1.0 /  float(numSamples)) * density;
    float illuminationDecay = 1.0;


    for(int i=0; i < 100 ; i++){
        if(numSamples < i) {
            break;
        }

        textCoo -= deltaTextCoord;
        vec3 samp = texture2D(occlusionTexture, textCoo).xyz;
        samp *= illuminationDecay * weight;
        fragColor += samp;
        illuminationDecay *= decay;
    }

    fragColor *= exposure;

    return fragColor;
}



void main() {
    vec3 fragColor = godrays(
            uDensity,
            uWeight,
            uDecay,
            uExposure,
            uNumSamples,
            uOcclusionTexture,
            uScreenSpaceSunPos,
            vUv
            );

    gl_FragColor = vec4(fragColor , 1.0);
}
