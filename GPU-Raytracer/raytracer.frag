# version 120

/** The maximum number of metaballs that can be passed to this shader. */
#define MAX_LIGHTS      5
#define MAX_SPHERES     5
#define MAX_PLANES      5
#define MAX_REFLECTIONS 2

#define INF             1.0 / 0.0
#define EPSILON         0.01

/** Contains for lights, may not be full. */
uniform vec4 lights[MAX_LIGHTS];
/** The actual number of lights. */
uniform int light_count;

/** Container for sphere, may not be full. */
uniform vec4 spheres[MAX_SPHERES];
/** The actual number of spheres. */
uniform int sphere_count;

/** Contains for planes, may not be full. */
uniform vec4 planes[MAX_PLANES];
/** The actual number of planes. */
uniform int plane_count;

/** The camera origin. */
uniform vec3 origin;
uniform mat4 camera_matrix;

/**
 * Calculates the intersection with the given plane,
 * storing it in the passed argument.
 *
 * @param ro The origin of the cast ray.
 * @param dir The direction of the cast ray.
 * @param index The index of the plane to intersect.
 * @param t The t value of the intersection in the ray's parametric equation.
 * @return Whether an intersection was found.
 */
 bool findPlaneIntersection(in vec3 ro, in vec3 dir, in int index, out float t) {
	vec4 plane = planes[index];

    t = -(dot(plane.xyz, ro) + plane.w) / dot(plane.xyz, dir);
    return t > 0.0;
 }

/**
 * Calculates the intersections with the given sphere,
 * storing it in the passed argument.
 *
 * @param ro The origin of the cast ray.
 * @param dir The direction of the cast ray.
 * @param index The index of the sphere to intersect.
 * @param t The t value of the intersection in the ray's parametric equation.
 * @return Whether an intersection was found.
 */
bool findSphereIntersection(in vec3 ro, in vec3 dir, in int index, out float t) {
	vec3 fc = ro - spheres[index].xyz;

	float a = dot(dir, dir);
	float b = 2.0 * dot(fc, dir);
	float c = dot(fc, fc) - spheres[index].w*spheres[index].w;

	if (a == 0) {
		if (b == 0) {
			return false;
		} else {
			t = -c / b;
			return t > 0.0;
		}
	} else {
		float det = b*b - 4.0 * a*c;
		if (det <= 0.0) {
			return false;
		} else {
			float q = -(b + sign(b)*sqrt(det)) / 2.0;
			t = q / a;

			if (q != 0) {
				t = min(t, c / q);
			}
			return t > 0.0;
		}
	}
	return false;
}

bool findIntersection(in vec3 ro, in vec3 rd, out vec3 pos, out vec3 normal) {
	float min_t = INF;
	for (int i = 0; i < sphere_count; ++i) {
		float itr_t;
		if (findSphereIntersection(ro, rd, i, itr_t) && itr_t < min_t) {
			min_t = itr_t;
			pos = ro + rd * min_t;
			normal = normalize(pos - spheres[i].xyz);
		}
	}
	for (int i = 0; i < plane_count; ++i) {
		float itr_t;
		if (findPlaneIntersection(ro, rd, i, itr_t) && itr_t < min_t) {
			min_t = itr_t;
			pos = ro + rd * min_t;
			normal = planes[i].xyz;
		}
	}
	return min_t != INF;
}

vec3 ambient = vec3(0.025, 0.025, 0.05);

vec3 colourForIntersection(in vec3 rd, in vec3 pos, in vec3 normal) {
	vec3 colour = ambient;
	for (int i = 0; i < light_count; ++i) {
		vec3 light_pos = lights[i].xyz;
		vec3 light_dir = normalize(light_pos - pos);

		vec3 l_pos, l_normal;
		if (!findIntersection(light_pos, -1 * light_dir, l_pos, l_normal)
				|| distance(light_pos, l_pos) >= distance(light_pos, pos) - EPSILON) {
			float shininess = 200.0;

			float lambertian = max(dot(light_dir, normal), 0.0);
			float specular = 0.0;
			if (lambertian > 0.0) {			
				vec3 refl_dir = reflect(-light_dir, normal);
				float spec_angle = max(dot(refl_dir, -rd), 0.0);
				specular = pow(spec_angle, shininess/4.0);
			}

			colour += lambertian * vec3(0.25, 0.25, 0.5)
				+ specular * vec3(1.0, 1.0, 1.0);
		}
	}
	return colour;
}

vec3 castRay(vec3 ro, vec3 rd) {
	vec3 final_colour = vec3(0.0);
	float colour_frac = 1.0;
	for (int i = 0; i < MAX_REFLECTIONS + 1; ++i) {
		vec3 pos, normal;
		if (!findIntersection(ro, rd, pos, normal)) {
			break;
		}

		vec3 colour = colourForIntersection(rd, pos, normal);
		final_colour += colour * colour_frac;

		// Set parameters for next iteration.
		colour_frac = colour_frac * 0.2;
		ro = pos + normal * EPSILON;
		rd = normalize(reflect(rd, normal));
	}
	return final_colour;
}

void main()
{
	vec3 pixel_in_world = (vec4(gl_FragCoord.xy, 0, 1) * camera_matrix).xyz;
	vec3 rd = normalize(pixel_in_world - origin);

	gl_FragColor = vec4(castRay(origin, rd), 1);
}