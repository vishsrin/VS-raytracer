struct point {
	float x;
	float y;
	float z;
};

struct position {
	float x;
	float y;
	float z;
};

struct color {
	int r;
	int g;
	int b;
};

struct specularity {
	float r;
	float g;
	float b;
};

struct diffuse {
	float r;
	float g;
	float b;
};

struct sphere {
	struct point pos;
	
	float radius;
	
	struct diffuse diff;
	
	struct specularity spec;
	
	float shininess;
};

struct quad {
	struct point pt1;
	struct point pt2;
	struct point pt3;
	struct point pt4;
};

struct vector {
	float x;
	float y;
	float z;
};

struct ray {
	struct point pt;
	struct vector vec;
	int numBounces;
};

struct intersection {
	float t;
	struct point point;
	struct vector normal;
	struct diffuse diff;
	struct specularity spec;
	float shininess;
};

struct light {
	struct point pos;
	struct diffuse diff;
	struct specularity spec;
};

struct triangle {
	struct point pt1;
	struct point pt2;
	struct point pt3;
	struct vector v12; // pt 2 - pt1
	struct vector v13; //pt 3 - pt 1
	struct vector normal;
	struct diffuse diff;
	struct specularity spec;
	float shininess;
};

void buildCornellBox();

void readFile(FILE *);

float readRad(char *);

struct point readPosition(char *);

struct specularity readSpec(char *);

struct diffuse readDiff(char *);

struct triangle createTriangle(struct point, struct point, struct point);

struct vector vector_subtract(struct vector, struct vector);

struct vector vector_multiply(struct vector, float);

struct color add_colors(struct color, struct color);

float vectorLength(struct vector);

struct vector normalize(struct vector);

struct intersection find_first_intersection(struct ray, int, struct triangle * , int , struct sphere *);

struct intersection ray_sphere_intersection(struct ray, struct sphere);

struct intersection ray_triangle_intersection(struct ray, struct triangle);

float dot_product(struct vector, struct vector);

struct vector cross_product(struct vector, struct vector);

float sign(struct point, struct point, struct point);

float determinant(struct vector, struct vector, struct vector);

struct color calc_all_lighting(struct intersection, struct ray, struct light *);

struct color calc_lighting(struct intersection, struct ray, struct light);

