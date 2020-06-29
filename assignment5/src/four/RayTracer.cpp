#include "raytracer.hpp"

#include "args.hpp"
#include "Camera.h"
#include "hit.hpp"
#include "lights.hpp"
#include "material.hpp"
#include "objects.hpp"
#include "ray.hpp"
#include "SceneParser.h"

#define EPSILON 0.001f

using namespace std;
using namespace FW;

namespace {

// Compute the mirror direction for the incoming direction, given the surface normal.
Vec3f mirrorDirection(const Vec3f& normal, const Vec3f& incoming) {
	// YOUR CODE HERE (R8)
	// Pay attention to the direction which things point towards, and that you only
	// pass in normalized vectors.
	Vec3f refle = incoming - 2 * dot(incoming, normal) * normal;
	return refle;
}

bool transmittedDirection(const Vec3f& normal, const Vec3f& incoming, 
		float index_i, float index_t, Vec3f& transmitted) {
	// YOUR CODE HERE (EXTRA)
	// Compute the transmitted direction for the incoming direction, given the surface normal
	// and indices of refraction. Pay attention to the direction which things point towards!
	// You should only pass in normalized vectors!
	// The function should return true if the computation was successful, and false
	// if the transmitted direction can't be computed due to total internal reflection.
	
	float dotto = -(FW::dot(normal, incoming));
	bool inside = false;
	if (dotto < 0.0f) {
		index_t = 1.0f;
		inside = true;
	}
	float refrx = index_i / index_t;
	float sini = refrx * refrx * (1.0f - dotto * dotto);
	if (sini > 1.0f) {
		return false;
	}
	float cos_T = FW::sqrt(1.0f - sini);
	if (inside) {
		transmitted = (refrx * incoming + (refrx * dotto + cos_T) * normal).normalized();
	}
	else {
		transmitted = (refrx * incoming + (refrx * dotto - cos_T) * normal).normalized();
	}
	return true;
}

} // namespace

Vec3f RayTracer::traceRay(Ray& ray, float tmin, int bounces, float refr_index, Hit& hit, FW::Vec3f debug_color) const {
	// initialize a hit to infinitely far away
	hit = Hit(FLT_MAX);

	// Ask the root node (the single "Group" in the scene) for an intersection.
	bool intersect = false;
	if(scene_.getGroup())
		intersect = scene_.getGroup()->intersect(ray, hit, tmin);

	// Write out the ray segment if visualizing (for debugging purposes)
	if (debug_trace)
		debug_rays.push_back(RaySegment(ray.origin, ray.direction.normalized()*min(100.0f,hit.t), hit.normal, debug_color));

	// if the ray missed, we return the background color.
	if (!intersect)
		return scene_.getBackgroundColor();
	
	Material* m = hit.material;
	assert(m != nullptr);

	// get the intersection point and normal.
	Vec3f normal = hit.normal;
	Vec3f point = ray.pointAtParameter(hit.t);

	// YOUR CODE HERE (R1)
	// Apply ambient lighting using the ambient light of the scene
	// and the diffuse color of the material.


	Vec3f answer = scene_.getAmbientLight() * m->diffuse_color(point);





	// YOUR CODE HERE (R4 & R7)
	// For R4, loop over all the lights in the scene and add their contributions to the answer.
	// If you wish to include the shadow rays in the debug visualization, insert the segments
	// into the debug_rays list similar to what is done to the primary rays after intersection.
	// For R7, if args_.shadows is on, also shoot a shadow ray from the hit point to the light
	// to confirm it isn't blocked; if it is, ignore the contribution of the light.
	
	int num = scene_.getNumLights();
	for (int i = 0; i < num; i++) {
		auto luce = scene_.getLight(i);
		float distance;
		Vec3f incident_intensity;
		Vec3f dir_to_light;
		luce->getIncidentIllumination(ray.pointAtParameter(hit.t), dir_to_light, incident_intensity, distance);
		
		
		if (args_.shadows) {
			Ray indietro = Ray(point , dir_to_light);
			auto culo = scene_.getGroup();
			Hit hitti;
			if (culo->intersect(indietro, hitti, tmin + 0.001)) {
				continue;
			}
		}
		answer += m->shade(ray, hit, dir_to_light, incident_intensity, args_.shade_back);
			
	}
	bool inside = FW::dot(normal, ray.direction) > 0;
	// are there bounces left?
	if (bounces >= 1) {
		// reflection, but only if reflective coefficient > 0!
		if (m->reflective_color(point).length() > 0.0f) {
			// YOUR CODE HERE (R8)
			// Generate and trace a reflected ray to the ideal mirror direction and add
			// the contribution to the result. Remember to modulate the returned light
			// by the reflective color of the material of the hit point.
			Ray riflesso = Ray(point,mirrorDirection(hit.normal, ray.direction));
			Hit hitti;
			answer += traceRay(riflesso , tmin+0.01, bounces-1, refr_index, hitti, debug_color) * m->reflective_color(point);
			
			

		}

		// refraction, but only if surface is transparent!
		if (m->transparent_color(point).length() > 0.0f) {
			// YOUR CODE HERE (EXTRA)
			// Generate a refracted direction and trace the ray. For this, you need
			// the index of refraction of the object. You should consider a ray going through
			// the object "against the normal" to be entering the material, and a ray going
			// through the other direction as exiting the material to vacuum (refractive index=1).
			// (Assume rays always start in vacuum, and don't worry about multiple intersecting
			// refractive objects!) Remembering this will help you figure out which way you
			// should use the material's refractive index. Remember to modulate the result
			// with the material's refractiveColor().
			// REMEMBER you need to account for the possibility of total internal reflection as well
			float refr_material = m->refraction_index(point);
			Vec3f transmitted;
			bool b = transmittedDirection(normal, ray.direction, refr_index, refr_material, transmitted);
			Hit new_hit;
			if (b) {
				if (inside) {
					refr_material = 1.0f;
				}
				Ray new_ray(point, transmitted);
				Vec3f refractedColor = traceRay(new_ray, tmin + EPSILON, bounces - 1, refr_material, new_hit, debug_color);
				answer += refractedColor * m->transparent_color(point);
			}
			else {
				Ray new_ray(point, mirrorDirection(-normal, ray.direction));
				Vec3f reflectedColor = traceRay(new_ray, tmin + EPSILON, bounces - 1, refr_material, new_hit, debug_color);
				answer += reflectedColor * m->reflective_color(point);
			}
			
		}
	}
	return answer;
}
