# version 120

/** The maximum number of metaballs that can be passed to this shader. */
#define MAX_SPHERES 200

#define INF 1.0 / 0.0
#define EPSILON 0.01
#define MAX_REFLECTIONS 1

/** Container for sphere, may not be full. */
uniform vec4 spheres[MAX_SPHERES];

/** The actual number of spheres. */
uniform int sphere_count;

/** The camera origin. */
uniform vec3 origin;
uniform mat4 camera_matrix;

/**
 * Calculates the intersections with the given sphere,
 * storing it in passed argument.
 *
 * @param ro The origin of the cast ray.
 * @param dir The direction of the cast ray.
 * @param index The index of the sphere to intersect.
 * @param intersections The structures for storing the intersection results.
 * @return The number of intersections found.
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
			return true;
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
			return true;
		}
	}
	return false;
}

bool findIntersection(in vec3 ro, in vec3 rd, out vec3 pos, out vec3 normal) {
	float min_t = INF;
	int inter_i = -1;
	for (int i = 0; i < sphere_count; ++i) {
		float itr_t;
		if (findSphereIntersection(ro, rd, i, itr_t)
				&& itr_t > 0 && itr_t < min_t) {
			min_t = itr_t;
			inter_i = i;
		}
	}
	if (inter_i == -1) {
		return false;
	}

	pos = ro + rd * min_t;
	normal = normalize(pos - spheres[inter_i].xyz);
	return true;
}

vec3 ambient = vec3(0.025, 0.025, 0.05);

vec3 colourForIntersection(in vec3 rd, in vec3 pos, in vec3 normal) {
	vec3 light_pos = vec3(0, 0, -600);
	vec3 light_dir = normalize(light_pos - pos);

	vec3 colour = ambient;

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
	return colour;
}

void main()
{
	vec3 pixel_in_world = (vec4(gl_FragCoord.xy, 0, 1) * camera_matrix).xyz;

	vec3 ro = origin;
	vec3 rd = normalize(pixel_in_world - origin);
	
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

	gl_FragColor = vec4(final_colour, 1);
}