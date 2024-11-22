#version 420 core

in vec3 fragPos;
in vec3 dir;
out vec4 outColor;

uniform mat4 invProjMatrix;
uniform mat4 invViewMatrix;
uniform vec2 screen_size;

uniform sampler2D depth_map;
#define MAX_VOLUME_MARCH_STEPS 100
#define ABSORPTION_COEFFICIENT 1.0
#define NUM_OCTAVES 5
#define LARGE_NUMBER 1e20
#define MAX_SDF_SPHERE_STEPS 20
#define MARCH_MULTIPLIER 0.1
#define ABSORPTION_CUTOFF 0.25

layout(std140, binding = 0) uniform FogData {
    vec4 center_and_radius; // vec3 center, float radius (w)
    vec4 color;
    float iTime;
    int iFrame;
};


float BeerLambert(float absorption_coeff, float distance_traveled) {
    return exp(-absorption_coeff * distance_traveled);
}

float sd_smooth_union(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1)/k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}
float sd_sphere(vec3 sample_pos, vec3 center, float radius) {
    float distance = length(center - sample_pos) - radius;
    return distance;
}
float sdPlane( vec3 p ) {
	return p.y;
}


float hash1( float n ) {
    return fract( n*17.0*fract( n*0.3183099 ) );
}
float noise( in vec3 x ) {
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    
    float n = p.x + 317.0*p.y + 157.0*p.z;
    
    float a = hash1(n+0.0);
    float b = hash1(n+1.0);
    float c = hash1(n+317.0);
    float d = hash1(n+318.0);
    float e = hash1(n+157.0);
	float f = hash1(n+158.0);
    float g = hash1(n+474.0);
    float h = hash1(n+475.0);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z);
}

const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );
float fbm_4( in vec3 x ) {
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for(int i=min(0, iFrame); i<4; i++)
    {
        float n = noise(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
	return a;
}

float QueryVolumetricDistanceField(vec3 pos) {
    // test volume
    vec3 fbm_coord = (pos + 2.0 * vec3(iTime, 0.0, iTime)) / 3.5;
    //float sdf_value = sd_sphere(pos, vec3(-8.0, 2.0 + 20.0 * sin(iTime), -1.0), 5.6);
    //sdf_value = sd_smooth_union(sdf_value, sd_sphere(pos, vec3(8.0, 8.0 + 12.0 * cos(iTime), 3.0), 5.6), 3.0);
    //sdf_value = sd_smooth_union(sdf_value, sd_sphere(pos, vec3(5.0 * sin(iTime), 3.0, 0.0), 8.0), 3.0) + 7.0 * fbm_4(fbm_coord / 3.2);
    //return sdf_value;

    return sd_sphere(pos, center_and_radius.xyz, center_and_radius.w) + fbm_4(fbm_coord / 3.2);
}
float GetFogDensity(vec3 position) {
    float sdf_value = QueryVolumetricDistanceField(position);
    const float max_sdf_multiplier = 1.0;
    bool inside_sdf = sdf_value < 0.0;
    float sdf_multiplier = inside_sdf ? min(abs(sdf_value), max_sdf_multiplier) : 0.0;
    return sdf_multiplier;
}
float IntersectVolumetric(in vec3 rayOrigin, in vec3 rayDirection, float maxT) {
    float t = 0.0f;
    for(int i=0; i<MAX_SDF_SPHERE_STEPS; i++ ) {
	    float result = QueryVolumetricDistanceField(rayOrigin+rayDirection*t);
        t += result;
        if(t>maxT) break;
    }
    return t;
}
float GetLightVisibility(vec3 ray_origin, vec3 ray_dir, float max_t, int max_steps, float march_size) {
    float t = 0.0;
    float light_visibility = 1.0;
    for(int i = 0; i < max_steps; i++) {
        t += march_size;
        if(t > max_t) {
            break;
        }
        vec3 position = ray_origin + t * ray_dir;
        if(QueryVolumetricDistanceField(position) < 0.0) {
            light_visibility *= BeerLambert(ABSORPTION_COEFFICIENT, march_size);
        }
    }
    return light_visibility;
}

vec3 GetWorldPos(float depth) {
    vec2 screen_pos = 2.0 * gl_FragCoord.xy / screen_size - vec2(1.0, 1.0);
    vec4 clip_space = vec4(screen_pos, depth, 1.0);
    vec4 view_dir_proj = invProjMatrix * clip_space;
    view_dir_proj = view_dir_proj / view_dir_proj.w;
    view_dir_proj.w = 1.0;
    view_dir_proj = invViewMatrix * view_dir_proj;
    return (view_dir_proj / view_dir_proj.w).xyz;
}

void GetSphereDepth(vec3 pos, vec3 ndir, out float t1, out float t2) {
    t1 = 0.0;
    t2 = 0.0;
    vec3 to_pos = pos - center_and_radius.xyz;
    float b = 2.0 * dot(to_pos, ndir);
    float c = dot(to_pos, to_pos) - center_and_radius.w * center_and_radius.w;
    float discriminant = b * b - 4 * c;
    if(discriminant < 0.0) {
        return;
    }
    t1 = (-b - sqrt(discriminant)) / 2.0;
    t2 = (-b + sqrt(discriminant)) / 2.0;
    if(t1 < 0.0 && t2 < 0.0) {
        t1 = 0.0;
        t2 = 0.0;
        return;
    }

    t1 = max(t1, 0.0);
    t2 = max(t2, 0.0);

}
float IntersectDepthMap() {
    vec2 screen_pos = gl_FragCoord.xy / screen_size;
    float depth_buffer_val = texture(depth_map, screen_pos).x;
    vec3 end_world_pos = GetWorldPos(depth_buffer_val * 2.0 - 1.0);
    return length(end_world_pos - fragPos);
}
float GetFog(vec3 pos, vec3 ndir) {
    vec2 screen_pos = gl_FragCoord.xy / screen_size;
    float depth_buffer_val = texture(depth_map, screen_pos).x;
    vec3 end_world_pos = GetWorldPos(depth_buffer_val * 2.0 - 1.0);
    float max_value = length(end_world_pos - pos);

    float t1,t2;
    GetSphereDepth(pos, ndir, t1, t2);
    t1 = min(t1, max_value);
    t2 = min(t2, max_value);
    vec3 p1 = t1 * ndir + pos;
    vec3 p2 = t2 * ndir + pos;

    //vec3 diff = (p2 - p1) / 5.0;
    //vec3 light_dir = normalize(vec3(-1.0, -1.0, 0.0));
    //for(int i = 0; i < 5; i++) {
    //    vec3 intermediate_pos = diff * (i + 1) + pos;
    //    float it1,it2;
    //    GetSphereDepth(intermediate_pos, -light_dir, it1,it2);
    //}


    
    float distance = abs(t2 - t1) / (2.0 * center_and_radius.w);
    return distance;
}

vec4 RenderFog(vec3 ray_origin, vec3 ray_dir) {
    float depth = IntersectDepthMap();
    vec3 opaque_color = vec3(0.0);

    float volumeDepth = IntersectVolumetric(ray_origin, ray_dir, depth);
    float opaqueVisiblity = 1.0f;
    vec3 volumetricColor = vec3(0.0f);
    if(volumeDepth >= 0.0) {
        const vec3 volumeAlbedo = vec3(0.8);
        const float marchSize = 0.6f * MARCH_MULTIPLIER * volumeDepth * 0.1f;
        float distanceInVolume = 0.0f;
        float signedDistance = 0.0;
        for(int i = 0; i < MAX_VOLUME_MARCH_STEPS; i++)
        {
            volumeDepth += max(marchSize, signedDistance);
            if(volumeDepth > depth || opaqueVisiblity < ABSORPTION_CUTOFF) break;
            
            vec3 position = ray_origin + volumeDepth*ray_dir;

            signedDistance = QueryVolumetricDistanceField(position);
			if(signedDistance < 0.0f)
            {
                distanceInVolume += marchSize;
                float previousOpaqueVisiblity = opaqueVisiblity;
                //opaqueVisiblity *= BeerLambert(ABSORPTION_COEFFICIENT * GetFogDensity(position, signedDistance), marchSize);
                opaqueVisiblity *= BeerLambert(ABSORPTION_COEFFICIENT * GetFogDensity(position), marchSize);
                float absorptionFromMarch = previousOpaqueVisiblity - opaqueVisiblity;
                vec3 ambient_light_color = vec3(0.2, 0.2, 0.2);

                //
                //float lightVolumeDepth = 0.0f;
                //vec3 lightDirection = normalize(vec3(-1.0, -1.0, 0.0));
                //vec3 lightColor = vec3(1.0, 1.0, 1.0); 

                //const float lightMarchSize = 0.65f * MARCH_MULTIPLIER;
                //float lightVisiblity = GetLightVisiblity(position, lightDirection, lightDistance, MAX_VOLUME_LIGHT_MARCH_STEPS, lightMarchSize);
                //volumetricColor += absorptionFromMarch * lightVisiblity * volumeAlbedo * lightColor;

                volumetricColor += absorptionFromMarch * volumeAlbedo * ambient_light_color * color.xyz;
            }
        }
    }
    else {
        volumetricColor = vec3(0.4, 0.4, 0.4);
        opaqueVisiblity = 0.0;
    }
    return vec4(min(volumetricColor, 1.0f), 1.0 - opaqueVisiblity);
}

void main() {
    vec3 ndir = normalize(dir);
    //float fog = GetFog(fragPos, ndir);
    //outColor = vec4(color.xyz, fog);
    vec4 fog_color = RenderFog(fragPos, ndir);
    outColor = fog_color;
}

