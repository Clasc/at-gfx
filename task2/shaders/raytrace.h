#define MAX_STACK_SIZE 32

//Todo(task2):
// Define your own BVH/Triangle structs and buffers.
// Implement an AABB and Triangle intersection test.
// struct IterativeNode {
//     int childrenStart;
//     int childCount;
//     int triangleCount;
//     int triangleStart;
//     vec3 aabbMin;
//     vec3 aabbMax;
// };

bool CastVisRay(vec3 origin, vec3 target) {
	// Todo(task2): Implement raycasting that only decides if the ray hits anything (return true) or not (return false).
	return false;
}


bool CastSurfaceRay(vec3 origin, vec3 target, out SurfacePoint point) {
	// Todo(task2): Implement raycasting that decides if the ray hits anything (return true) or not (return false) and
	// also returns information about the point it hit.
	point.Position = vec3(0, 0, 0);   
	point.Normal = vec3(0, 1, 0);     
	point.Albedo = vec4(0, 0, 0, 1);  
    point.Metalness = 0.5;            
    point.Roughness = 0.5;            

	return false;
}

bool RayHitsAABB(vec3 p0, vec3 p1, vec3 bmin, vec3 bmax){
	// vec3 box_center = (bmin + bmax) * 0.5; 
	// vec3 halflength_extents = bmax - box_center; 
	// vec3 segment_mid = (p0 + p1) * 0.5;
	// vec3 segment_halflength_vec = p1 - segment_mid; // Segment halflength vector
	// segment_mid = segment_mid - box_center; // Translate box and segment to origin
	// // Try world coordinate axes as separating axes
	// float adx = abs(d.x);
	// if (abs(segment_mid.x) > halflength_extents.x + adx) return false;
	// float ady = abs(d.y);
	// if (abs(segment_mid.y) > halflength_extents.y + ady) return false;
	// float adz = abs(d.z);
	// if (abs(segment_mid.z) > halflength_extents.z + adz) return false;
	// // Add in an epsilon term to counteract arithmetic errors when segment is
	// // (near) parallel to a coordinate axis (see text for detail)
	// adx += EPSILON; ady += EPSILON; adz += EPSILON;
	// // Try cross products of segment direction vector with coordinate axes
	// if (abs(segment_mid.y * segment_halflength_vec.z - m.z * segment_halflength_vec.y) > halflength_extents.y * adz + halflength_extents.z * ady) return false;
	// if (abs(segment_mid.z * segment_halflength_vec.x - m.x * segment_halflength_vec.z) > halflength_extents.x * adz + halflength_extents.z * adx) return false;
	// if (abs(segment_mid.x * segment_halflength_vec.y - m.y * segment_halflength_vec.x) > halflength_extents.x * ady + halflength_extents.y * adx) return false;
	// // No separating axis found; segment must be overlapping AABB
	return true;
}

bool RayHitsTriangle(vec3 p, vec3 q, vec3 a, vec3 b, vec3 c, inout float u, inout float v, inout float w, inout float t) {
	// vec3 ab = b - a;
	// vec3 ac = c - a;
	// vec3 qp = p - q;
	// // Compute triangle normal. Can be precalculated or cached if intersecting multiple segments against the same triangle
	// vec3 n = cross(ab, ac);
	// // Compute denominator d. If d <= 0, segment is parallel to or points away from triangle, so exit early
	// float d = dot(qp, n);
	// if (d <= 0.0) return false;
	// // Compute intersection t value of pq with plane of triangle. A ray intersects iff 0 <= t.
	// // Segment intersects iff 0 <= t <= 1. Delay dividing by d until intersection has been found to pierce triangle
	// vec3 ap = p - a;
	// t = dot(ap, n);
	// if (t < 0.0) return false;
	// if (t > d) return false;
	// // Compute barycentric coordinate components and test if within bounds
	// vec3 e = cross(qp, ap);
	// v = dot(ac, e);
	// if (v < 0.0 || v > d) return false;
	// w = -dot(ab, e);
	// if (w < 0.0 || v + w > d) return false;
	// // Segment/ray intersects triangle. Perform delayed division and compute the last barycentric coordinate component
	// float ood = 1.0 / d;
	// t *= ood;
	// v *= ood;
	// w *= ood;
	// u = 1.0 - v - w;
	 return true;
}