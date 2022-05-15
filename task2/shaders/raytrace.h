#define MAX_STACK_SIZE 32

//Todo(task2):
// Define your own BVH/Triangle structs and buffers.
// Implement an AABB and Triangle intersection test.


bool
CastVisRay(vec3 origin, vec3 target) {
	// Todo(task2): Implement raycasting that only decides if the ray hits anything (return true) or not (return false).
	return false;
}


bool
CastSurfaceRay(vec3 origin, vec3 target, out SurfacePoint point) {
	// Todo(task2): Implement raycasting that decides if the ray hits anything (return true) or not (return false) and
	// also returns information about the point it hit.
	point.Position = vec3(0, 0, 0);   // The hit position of the ray in worldspace.
	point.Normal = vec3(0, 1, 0);     // The normal of the triangle that it hit.
	point.Albedo = vec4(0, 0, 0, 1);  // The albedo color and transparency of the point that was hit.
    point.Metalness = 0.5;            // The metalness of the point that was hit.
    point.Roughness = 0.5;            // The roughness of the point that was hit.

	return false;
}