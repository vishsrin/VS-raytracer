#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include "shapes.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>


struct color background_color;
struct triangle tris[24]; //array of all triangle objects in scene
struct sphere spheres[24]; //array of all sphere objects in scnee
struct light lights[24]; //array of all light objects in scene
int numtris; //stores number of triangles in scene
int numspheres; //... number of spheres
int numlights; //... number of lights

int maxBounces; //max raytracing depth (number of light bounces

int pixel_width; //outp resolution width, pixel_width = 800 --> 800x800px output

#define self_ref_marg 0.001 //small length 

int main() {
	numtris = 0;
	numspheres = 0;


	FILE *f = fopen("outp.ppm", "w");
	if (f == NULL) {
		printf("Error opening file!\n");
		exit(1);
	}

	pixel_width = 0;

	FILE *f2 = fopen("scene.txt", "r");

	readFile(f2); //initialize scene variables

	fclose(f2);	
	
	int screensize_coords = 4; //camera box is 4 coordinate units wide
	float resolution = pixel_width / screensize_coords; //pixels [width] per unit of screenspace plane
	float coordinate_delta = screensize_coords / resolution; //amount to increase coords to get next pixel

	fprintf(f, "P3 %d %d %d\n", pixel_width, pixel_width, 255); //initialize ppm file

	struct color backgroundcolor = background_color;

	struct point origin;
	origin.x = 0;
	origin.y = 0;
	origin.z = 0;

	struct color full_image[pixel_width][pixel_width]; //array to store color of each pixel

	int maxColorVal = 0; //maximum individual channel value for full image


	for (int Ypixel = 0; Ypixel < pixel_width; Ypixel++) {
		for (int Xpixel = 0; Xpixel < pixel_width; Xpixel++) { //go through all pixels
			struct point screen_intersection; //point at which camera ray intersects screen plane
			screen_intersection.x = Xpixel / resolution - screensize_coords / 2;
			screen_intersection.y = -1 * Ypixel / resolution + screensize_coords / 2;
			screen_intersection.z = 1;

			struct vector ray_unnormalized; //un normalized ray that goes through set pixel
			ray_unnormalized.x = screen_intersection.x;
			ray_unnormalized.y = screen_intersection.y;
			ray_unnormalized.z = screen_intersection.z;
			
			struct vector rayvec = normalize(ray_unnormalized); //normalize vector
			struct ray ray; //turn into ray object
			ray.pt = origin;
			ray.vec = rayvec;
			ray.numBounces = 0;
		
			struct intersection intr = find_first_intersection(ray, numtris, tris, numspheres, spheres); //find intersection of ray with nearest object
			
			if (intr.point.z > 0) { //if object is in front of camera
				struct color pixelColor = calc_all_lighting(intr, ray, lights); //calculate lighting color of object at that point
				full_image[Xpixel][Ypixel] = pixelColor; //save color into array 
				
			} else {
				full_image[Xpixel][Ypixel] = backgroundcolor; //if no intersection or object behind camera, make pixel background color
			}

			struct color pixelColor = full_image[Xpixel][Ypixel];
			//find maximum individual color channel value
			if (pixelColor.r > maxColorVal) {
				maxColorVal = pixelColor.r;
			}
			
			if (pixelColor.g > maxColorVal) {
				maxColorVal = pixelColor.g;
			}

			if (pixelColor.b > maxColorVal) {
				maxColorVal = pixelColor.b;
			} 
		}
	}
		
	//scale all color values so that the maximum color channel value becomes 255
	for (int Ypixel = 0; Ypixel < pixel_width; Ypixel++) {
		for (int Xpixel = 0; Xpixel < pixel_width; Xpixel++) {
			struct color pixelColor = full_image[Xpixel][Ypixel];
			float scaling = 255.0 / maxColorVal;
			pixelColor.r = (int)(pixelColor.r * scaling);
			pixelColor.g = (int)(pixelColor.g * scaling);
			pixelColor.b = (int)(pixelColor.b * scaling);
			//printf("after: %d %d %d\n", pixelColor.r, pixelColor.g, pixelColor.b);
			fprintf(f, "%d %d %d\n", pixelColor.r, pixelColor.g, pixelColor.b);
		}
	} 

	fclose(f);

}

void readFile(FILE * fp ) {
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    const char whitespace[] = " \f\n\r\t\v";

    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        char *stripped = line + strspn(line, whitespace);
        if (!(stripped[0] == '/' && stripped[1] == '/')) {
		char delimiter[] = " ";
		char *firstWord, *remainder, *context;

		int inputLength = strlen(line);
		char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
		strncpy(inputCopy, line, inputLength);

		firstWord = strtok_r (inputCopy, delimiter, &context);
		if (strcmp(firstWord, "BACKGROUND") == 0) {
			int arg1 = atoi(strtok_r (NULL, delimiter, &context));
			int arg2 = atoi(strtok_r (NULL, delimiter, &context));
			int arg3 = atoi(strtok_r (NULL, delimiter, &context));
			remainder = context;

			background_color.r = arg1;
			background_color.g = arg2;
			background_color.b = arg3;
		} else if (strcmp(firstWord, "MAXDEPTH") == 0) {
			int arg1 = atoi(strtok_r (NULL, delimiter, &context));
			maxBounces = arg1;
		} else if (strcmp(firstWord, "LIGHT\n") == 0) {
			read = getline(&line, &len, fp);
			struct point pos = readPosition(line);
			read = getline(&line, &len, fp);
			struct diffuse diff = readDiff(line);
			read = getline(&line, &len, fp);
			struct specularity spec = readSpec(line);
			
			struct light light;
			light.pos = pos;
			light.diff = diff;
			light.spec = spec;
			lights[numlights] = light;
			numlights++;
		} else if (strcmp(firstWord, "SPHERE\n") == 0) {
			read = getline(&line, &len, fp);
			struct point pos = readPosition(line);
			read = getline(&line, &len, fp);
			float radius = readRad(line);
			read = getline(&line, &len, fp);
			struct diffuse diff = readDiff(line);
			read = getline(&line, &len, fp);
			struct specularity spec = readSpec(line);
			read = getline(&line, &len, fp);
			float shininess = readRad(line);
	
			struct sphere sphere;
			sphere.pos = pos;
			sphere.radius = radius;
			sphere.diff = diff;
			sphere.spec = spec;
			sphere.shininess = shininess;

			spheres[numspheres] = sphere;
			numspheres++;
		
		} else if (strcmp(firstWord, "QUAD\n") == 0) {
			read = getline(&line, &len, fp);
			struct point pt1 = readPosition(line);
			read = getline(&line, &len, fp);
			struct point pt2 = readPosition(line);
			read = getline(&line, &len, fp);
			struct point pt3 = readPosition(line);
			read = getline(&line, &len, fp);
			struct diffuse diff = readDiff(line);
			read = getline(&line, &len, fp);
			struct specularity spec = readSpec(line);
			read = getline(&line, &len, fp);
			float shin = readRad(line);
		
			struct triangle tri;
			if ((pt1.y < 0 && pt2.y < 0 && pt3.y < 0) || (pt1.z < 0 && pt2.z < 0 && pt3.z < 0)) {
				tri = createTriangle(pt1, pt3, pt2);
			} else {	
				 tri = createTriangle(pt1, pt2, pt3);
			}
			tri.diff = diff;
			tri.spec = spec;
			tri.shininess = shin;

			struct point pt4;

			pt4.x = pt1.x + tri.v12.x + tri.v13.x;
			pt4.y = pt1.y + tri.v12.y + tri.v13.y;
			pt4.z = pt1.z + tri.v12.z + tri.v13.z;

			struct triangle tri2;
			if ((pt1.y < 0 && pt2.y < 0 && pt3.y < 0) || (pt1.z < 0 && pt2.z < 0 && pt3.z < 0)){
				tri2 = createTriangle(pt2, pt3, pt4);
			} else {
				tri2 = createTriangle(pt3, pt2, pt4);
			}
			tri2.diff = diff;
			tri2.spec = spec;
			tri2.shininess = shin;

			tris[numtris] = tri;
			numtris++;

			tris[numtris] = tri2;
			numtris++;
		} else if (strcmp(firstWord, "RESOLUTION") == 0) {
			int arg1 = atoi(strtok_r (NULL, delimiter, &context));
			pixel_width = arg1;
		}
		if (inputCopy) {
			free(inputCopy);
		}
	}
    }

    fclose(fp);
    if (line) {
        free(line);
    }
}


float readRad(char * line) {

	char delimiter[] = " ";
	char *firstWord, *arg1, *arg2, *arg3, *remainder, *context;

	int inputLength = strlen(line);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, line, inputLength);
	firstWord = strtok_r (inputCopy, delimiter, &context);
	arg1 = strtok_r (NULL, delimiter, &context);

	return atof(arg1);
}

struct point readPosition(char * line) {
	struct point outp;

	char delimiter[] = " ";
	char *firstWord, *arg1, *arg2, *arg3, *remainder, *context;

	int inputLength = strlen(line);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, line, inputLength);
	firstWord = strtok_r (inputCopy, delimiter, &context);
	arg1 = strtok_r (NULL, delimiter, &context);
	arg2 = strtok_r (NULL, delimiter, &context);
	arg3 = strtok_r (NULL, delimiter, &context);

	outp.x = atof(arg1);
	outp.y = atof(arg2);
	outp.z = atof(arg3);

	return outp;
}

struct specularity readSpec(char * line) {
	struct specularity outp;

	char delimiter[] = " ";
	char *firstWord, *arg1, *arg2, *arg3, *remainder, *context;

	int inputLength = strlen(line);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, line, inputLength);
	firstWord = strtok_r (inputCopy, delimiter, &context);
	arg1 = strtok_r (NULL, delimiter, &context);
	arg2 = strtok_r (NULL, delimiter, &context);
	arg3 = strtok_r (NULL, delimiter, &context);

	outp.r = atof(arg1);
	outp.g = atof(arg2);
	outp.b = atof(arg3);

	return outp;
}

struct diffuse readDiff(char * line) {
	struct diffuse outp;

	char delimiter[] = " ";
	char *firstWord, *arg1, *arg2, *arg3, *remainder, *context;

	int inputLength = strlen(line);
	char *inputCopy = (char*) calloc(inputLength + 1, sizeof(char));
	strncpy(inputCopy, line, inputLength);
	firstWord = strtok_r (inputCopy, delimiter, &context);
	arg1 = strtok_r (NULL, delimiter, &context);
	arg2 = strtok_r (NULL, delimiter, &context);
	arg3 = strtok_r (NULL, delimiter, &context);

	outp.r = atof(arg1);
	outp.g = atof(arg2);
	outp.b = atof(arg3);

	return outp;
}

void buildCornellBox() {
	float boxSize = 150;

	struct diffuse wallDiff;
	wallDiff.r = 0.7;
	wallDiff.g = 0.7;
	wallDiff.b = 0.7;

	struct diffuse green;
	green.r = 0.0;
	green.g = 1.0;
	green.b = 0.0;
	
	struct diffuse red;
	red.r = 1.0;
	red.g = 0.0;
	red.b = 0.0;

	struct specularity wallSpec;
	wallSpec.r = 0.0;
	wallSpec.g = 0.0;
	wallSpec.b = 0.0;

	float wallShin = 5.0;

	struct diffuse ballDiff;
	ballDiff.r = 0.8;
	ballDiff.g = 0.1;
	ballDiff.b = 0.9;

	struct specularity ballSpec;
	ballSpec.r = 1.0;
	ballSpec.g = 1.0;
	ballSpec.b = 1.0;

	float ballShin = 100.0;

	struct point pt1; //back left up
	struct point pt2; //front left up
	struct point pt3; //front right up
	struct point pt4; //back right up
	struct point pt5; //back left down
	struct point pt6; //front left down
	struct point pt7; //front right down
	struct point pt8; //back right down

	pt1.x = -1 * boxSize;
	pt1.y = boxSize;
	pt1.z = -1 * boxSize;

	pt2.x = -1 * boxSize;
	pt2.y = boxSize;
	pt2.z = boxSize;

	pt3.x = boxSize;
	pt3.y = boxSize;
	pt3.z = boxSize;
	
	pt4.x = boxSize;
	pt4.y = boxSize;
	pt4.z = -1 * boxSize;

	pt5.x = -1 * boxSize;
	pt5.y = -1 * boxSize;
	pt5.z = -1 * boxSize;

	pt6.x = -1 * boxSize;
	pt6.y = -1 * boxSize;
	pt6.z = boxSize;

	pt7.x = boxSize;
	pt7.y = -1 * boxSize;
	pt7.z = boxSize;

	pt8.x = boxSize;
	pt8.y = -1 * boxSize;
	pt8.z = -1 * boxSize;

	//printf("pt3: %f %f %f\n",pt3.x,pt3.y,pt3.z);
	//printf("pt4: %f %f %f\n",pt4.x, pt4.y, pt4.z);
	//printf("pt7: %f %f %f\n",pt7.x, pt7.y, pt7.z);
	//printf("pt8: %f %f %f\n", pt8.x, pt8.y, pt8.z);

	
	struct triangle tri1 = createTriangle(pt1, pt5, pt6);
	tri1.diff = wallDiff;
	tri1.spec = wallSpec;
	tri1.shininess = wallShin;
	struct triangle tri2 = createTriangle(pt1, pt6, pt2);;
	tri2.diff = wallDiff;
	tri2.spec = wallSpec;
	tri2.shininess = wallShin;

	struct triangle tri3 = createTriangle(pt1, pt4, pt8);
	struct triangle tri4 = createTriangle(pt1, pt8, pt5);;
	tri3.diff = wallDiff;
	tri3.spec = wallSpec;
	tri3.shininess = ballShin;;
	tri4.diff = wallDiff;
	tri4.spec = wallSpec;
	tri4.shininess = ballShin;

	struct triangle tri5 = createTriangle(pt3, pt7, pt8);
	struct triangle tri6 = createTriangle(pt3, pt8, pt4);;
	tri5.diff = wallDiff;
	tri5.spec = wallSpec;
	tri5.shininess = wallShin;;
	tri6.diff = wallDiff;
	tri6.spec = wallSpec;
	tri6.shininess = wallShin;

	struct triangle tri7 = createTriangle(pt2, pt6, pt7);
	struct triangle tri8 = createTriangle(pt2, pt7, pt3);
	tri7.diff = wallDiff;
	tri7.spec = wallSpec;
	tri7.shininess = wallShin;;
	tri8.diff = wallDiff;
	tri8.spec = wallSpec;
	tri8.shininess = wallShin;

	struct triangle tri9 = createTriangle(pt6, pt5, pt8);
	struct triangle tri10 = createTriangle(pt6, pt8, pt7);
	tri9.diff = wallDiff;
	tri9.spec = wallSpec;
	tri9.shininess = wallShin;
	tri10.diff = wallDiff;
	tri10.spec = wallSpec;
	tri10.shininess = wallShin;	

	struct triangle tri11 = createTriangle(pt1, pt2, pt4);
	struct triangle tri12 = createTriangle(pt2, pt3, pt4);
	tri11.diff = wallDiff;
	tri11.spec = wallSpec;
	tri11.shininess = wallShin;
	tri12.diff = wallDiff;
	tri12.spec = wallSpec;
	tri12.shininess = wallShin;
	
	struct sphere sphere1;
	struct point spherepos;
	spherepos.x = 75;
	spherepos.y = 60;
	spherepos.z = 80;
	sphere1.pos = spherepos;
	sphere1.radius = 20;
	sphere1.diff = ballDiff;
	sphere1.spec = ballSpec;
	sphere1.shininess = ballShin;
	spheres[0] = sphere1;

	struct sphere sphere2;
	sphere2.pos.x = -110;
	sphere2.pos.y = -20;
	sphere2.pos.z = 80;
	sphere2.radius = 20;
	sphere2.diff = ballDiff;
	sphere2.spec = ballSpec;
	sphere2.shininess = ballShin;
	spheres[1] = sphere2;
	numspheres = 2;

	struct sphere sphere3;
	sphere3.pos.x = 0;
	sphere3.pos.y = 0;
	sphere3.pos.z = 80;
	sphere3.radius = 20;
	sphere3.diff = ballDiff;
	sphere3.spec = ballSpec;
	sphere3.shininess = ballShin;
	spheres[2] = sphere3;
	numspheres = 3;

	struct light light1;
	light1.pos.x = -55;
	light1.pos.y = 55;
	light1.pos.z = 55;

	light1.diff.r = 0.6;
	light1.diff.g = 0.6;
	light1.diff.b = 0.6;
	
	light1.spec.r = 1.0;
	light1.spec.g = 1.0;
	light1.spec.b = 1.0;

	lights[0] = light1;
	numlights = 1;

	struct light light2;
	light2.pos.x = 60;
	light2.pos.y = -120;
	light2.pos.z = 120;

	light2.diff.r = 0.0;
	light2.diff.g = 1.0;
	light2.diff.b = 0.0;
	
	light2.spec.r = 0.4;
	light2.spec.g = 0.4;
	light2.spec.b = 0.4;

	lights[1] = light2;
	
	struct light light3;
	light3.pos.x = 0;
	light3.pos.y = 0;
	light3.pos.z = 20;

	light3.diff.r = 0.0;
	light3.diff.g = 0.0;
	light3.diff.b = 1.0;
	
	light3.spec.r = 0.4;
	light3.spec.g = 0.4;
	light3.spec.b = 0.4;

	lights[2] = light3;
	numlights = 3;
	tris[0] = tri1;
	tris[1] = tri2;
	tris[2] = tri3;
	tris[3] = tri4;
	tris[4] = tri5;
	tris[5] = tri6;
	tris[6] = tri7;
	tris[7] = tri8;
	tris[8] = tri9;
	tris[9] = tri10;
	tris[10] = tri11;
	tris[11] = tri12;
	
	numtris = 2;
}

struct triangle createTriangle(struct point pt1, struct point pt2, struct point pt3) {
	struct triangle outp;
	outp.pt1 = pt1;
	outp.pt2 = pt2;
	outp.pt3 = pt3;

	outp.v12.x = pt2.x - pt1.x;
	outp.v12.y = pt2.y - pt1.y;
	outp.v12.z = pt2.z - pt1.z;

	outp.v13.x = pt3.x - pt1.x;
	outp.v13.y = pt3.y - pt1.y;
	outp.v13.z = pt3.z - pt1.z;

	struct vector outpcross = cross_product(outp.v13, outp.v12);
	//printf("outpcross: %f %f %f", outpcross.x, outpcross.y, outpcross.y);
	outp.normal = normalize(outpcross);
	return outp;
}

	
struct vector vector_subtract(struct vector v1, struct vector v2) {
	struct vector vout;
	vout.x = v1.x - v2.x;
	vout.y = v1.y - v2.y;
	vout.z = v1.z - v2.z;
	return vout;
}

struct vector vector_multiply(struct vector vin, float m) {
	struct vector vout;
	vout.x = vin.x * m;
	vout.y = vin.y * m;
	vout.z = vin.z * m;
	return vout;
}

struct vector normalize(struct vector inp) {
	float vLength = 0;
	vLength = vectorLength(inp);
	struct vector outp;
	outp.x = inp.x / vLength;
	outp.y = inp.y / vLength;
	outp.z = inp.z / vLength;
	return outp;
}

float vectorLength(struct vector inp) {
	float vectorLength = 0;
	vectorLength = sqrt(inp.x * inp.x + inp.y * inp.y + inp.z * inp.z);
	return vectorLength;
}

float dot_product(struct vector v1, struct vector v2) {
	return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
}

struct vector cross_product(struct vector v1, struct vector v2) {
	struct vector outp;
	outp.x = v1.y * v2.z - v1.z * v2.y;
	outp.y = -1 * (v1.x * v2.z - v1.z * v2.x);
	outp.z = v1.x * v2.y - v1.y * v2.x;
	return outp;
}

float determinant(struct vector col1, struct vector col2, struct vector col3) {
	return -1 * dot_product(cross_product(col1, col3), col2);
}
struct intersection find_first_intersection(struct ray ray, int numtris, struct triangle tris[], int numsphs, struct sphere spheres[]) {
	struct intersection intr;
	intr.point.z = -1;
	intr.t = -2;

	
	struct intersection test_intr;
	for (int i = 0; i < numtris; i++) {
		test_intr = ray_triangle_intersection(ray, tris[i]);
		if (test_intr.t > 0.0 && intr.t < 0) {
			intr = test_intr;
			intr.spec = tris[i].spec;
			intr.diff = tris[i].diff;
			intr.shininess = tris[i].shininess;
			//printf("int tri: %f %f %f\n",intr.point.x, intr.point.y, intr.point.z);
		} else if (test_intr.t > 0.00 && test_intr.t < intr.t) {
			intr = test_intr;
			intr.spec = tris[i].spec;
			intr.diff = tris[i].diff;
			intr.shininess = tris[i].shininess;
			//printf("int tri2: %f %f %f\n",intr.point.x, intr.point.y, intr.point.z);
		}
	}
	
	for (int i = 0; i < numsphs; i++) {
		if (i == 1) {
			//printf("checking second sphere\n");
		}
		test_intr = ray_sphere_intersection(ray, spheres[i]);
		if (test_intr.t > self_ref_marg && intr.t < 0) {
			intr = test_intr;
			intr.spec = spheres[i].spec;
			intr.diff = spheres[i].diff;
			intr.shininess = spheres[i].shininess;
			//printf("int sph %d: %f %f %f\n", i, intr.point.x, intr.point.y, intr.point.z);
		} else if (test_intr.t > self_ref_marg && test_intr.t < intr.t) {
			intr = test_intr;
			intr.spec = spheres[i].spec;
			intr.diff = spheres[i].diff;
			intr.shininess = spheres[i].shininess;
			//printf("int sph %d: %f %f %f\n", i, intr.point.x, intr.point.y, intr.point.z);
		}
	}

	
	return intr;
}

struct intersection ray_sphere_intersection(struct ray ray, struct sphere sphere) {
	struct vector v;
	v.x = sphere.pos.x - ray.pt.x;
	v.y = sphere.pos.y - ray.pt.y;
	v.z = sphere.pos.z - ray.pt.z;

	//printf("pos: %f %f %f, radius: %f\n", sphere.pos.x, sphere.pos.y, sphere.pos.z, sphere.radius);

	//if (vectorLength(ray.vec) < 0.95 || vectorLength(ray.vec) > 1.05) {
		//printf("WOAH: %f\n", vectorLength(ray.vec));
	//}

	float dotProduct = 0;
	dotProduct = dot_product(ray.vec, v);

	float vLength = vectorLength(v);
	float D_squared = vLength * vLength - dotProduct * dotProduct;
	
	//printf("vLength: %f, dotProduct: %f, D: %f\n", vLength, dotProduct, D);
	
	struct intersection outp;
	if (D_squared > sphere.radius * sphere.radius) { //ray does not intersect sphere
		outp.point.x = 0;
		outp.point.y = 0;
		outp.point.z = -1;
		outp.t = -1;
		return outp;
	}

	float t_d = sqrt(sphere.radius * sphere.radius - D_squared);
	float t1 = dotProduct - t_d;
	float t2 = dotProduct + t_d;

	if (t1 <= 0) {
	//	printf("dotProduct: %f, t_d: %f\n", dotProduct, t_d);
	}

	//printf("t1: %f, t2: %f\n", t1, t2);

	if (t1 <= self_ref_marg && t2 <= self_ref_marg) { //both intersections behind camera
		outp.point.x = 0;
		outp.point.y = 0;
		outp.point.z = -1;
		outp.t = -1;
		return outp;
	}

	//printf("ray length: %f\n", vectorLength(ray.vec));

	if (t1 > self_ref_marg) { //first intersection in front of camera
		outp.point.x = ray.pt.x + ray.vec.x * t1;
		outp.point.y = ray.pt.y + ray.vec.y * t1;
		outp.point.z = ray.pt.z + ray.vec.z * t1;

		outp.normal.x = (outp.point.x - sphere.pos.x) / sphere.radius;
		outp.normal.y = (outp.point.y - sphere.pos.y) / sphere.radius;
		outp.normal.z = (outp.point.z - sphere.pos.z) / sphere.radius;
		
		outp.t = t1;

		//printf("Normal: %f, %f, %f\n", outp.normal.x, outp.normal.y, outp.normal.z);
	} else {
		outp.point.x = ray.pt.x + ray.vec.x * t2;
		outp.point.y = ray.pt.y + ray.vec.y * t2;
		outp.point.z = ray.pt.z + ray.vec.z * t2;

		outp.normal.x = (outp.point.x - sphere.pos.x) / sphere.radius;
		outp.normal.y = (outp.point.y - sphere.pos.y) / sphere.radius;
		outp.normal.z = (outp.point.z - sphere.pos.z) / sphere.radius;

		outp.t = t2;
	}
	if (vectorLength(outp.normal) > 1.1) {
		struct vector testVec;
		testVec.x = outp.point.x - sphere.pos.x;
		testVec.y = outp.point.y - sphere.pos.y;
		testVec.z = outp.point.z - sphere.pos.z;
		//printf("YO!!!!!\n");
		//printf("ray start: %f %f %f, t: %f, t_d: %f\n", ray.pt.x, ray.pt.y, ray.pt.z, outp.t, t_d);
		//printf("rayvec: %f %f %f\n", ray.vec.x, ray.vec.y, ray.vec.z);
		//printf("int point: %f %f %f\n", outp.point.x, outp.point.y, outp.point.z);
		//printf("sphere center: %f %f %f, radius: %f\n", sphere.pos.x, sphere.pos.y, sphere.pos.z, sphere.radius);	
		//printf("norm length:%f, normal: %f %f %f\n\n", vectorLength(outp.normal), outp.normal.x, outp.normal.y, outp.normal.z);
	}
	return outp;
}

float sign(struct point pt1, struct point pt2, struct point pt3) {
	return (pt1.x - pt3.x) * (pt2.y - pt3.y) - (pt2.x - pt3.x) * (pt1.y - pt3.y);}


struct intersection ray_triangle_intersection(struct ray ray, struct triangle triangle) {
	float epsilon = std::numeric_limits<float>::epsilon();
	
	struct vector edge1 = triangle.v12;
	struct vector edge2 = triangle.v13;
	struct vector ray_cross_e2 = cross_product(ray.vec, edge2);
	float det = dot_product(edge1, ray_cross_e2);
	
	intersection outp;
	if (det > -1 * epsilon && det < epsilon) {
		outp.point.x = 0;
		outp.point.y = 0;

		return outp;
	}

	float inv_det = 1.0 / det;
	struct vector s;
	s.x = ray.pt.x - triangle.pt1.x;
	s.y = ray.pt.y - triangle.pt1.y;
	s.z = ray.pt.z - triangle.pt1.z;
	
	float u = inv_det * dot_product(s, ray_cross_e2);

	if (u < 0 || u > 1) {
		outp.point.x = 0;
		outp.point.y = 0;
		outp.point.z = -1;
		outp.t = -1;
		return outp;
	}

	struct vector s_cross_e1 = cross_product(s, edge1);
	float v = inv_det * dot_product(ray.vec, s_cross_e1);

	if (v < 0 || u + v > 1) {
		outp.t = -1;
		outp.point.z = -1;
		return outp;
	}

	float t = inv_det * dot_product(edge2, s_cross_e1);
	
	if (t > epsilon)
	{
		outp.t = t;
		outp.point.x = ray.pt.x + ray.vec.x * t;
		outp.point.y = ray.pt.y + ray.vec.y * t;
		outp.point.z = ray.pt.z + ray.vec.z * t;
		outp.normal = triangle.normal;
		return outp;
	}
	else {
		outp.point.z = -1;
		outp.t = -1;
		return outp;
	}
}

struct color add_colors(struct color color1, struct color color2) {
	struct color outp_color;
	
	outp_color.r = color1.r + color2.r;
	outp_color.g = color1.g + color2.g;
	outp_color.b = color1.b + color2.b;
	return outp_color;
}

struct color calc_all_lighting(struct intersection intr, struct ray ray, struct light lights[]) {
	struct color total_color;
	total_color.r = 0;
	total_color.g = 0;
	total_color.b = 0;
	
	for (int i = 0; i < numlights; i++) {
		total_color = add_colors(total_color, calc_lighting(intr, ray, lights[i]));
	}

	return total_color;
}

struct color calc_lighting(struct intersection intr, struct ray ray, struct light light) {
	struct point point = intr.point;
	struct vector normal = intr.normal;
	struct diffuse objDiff = intr.diff;
	struct specularity objSpec = intr.spec;
	float objShin = intr.shininess;
	
	

	struct vector incident_unnorm; //vector from point to light- incident light
	incident_unnorm.x = light.pos.x - point.x;
	incident_unnorm.y = light.pos.y - point.y;
	incident_unnorm.z = light.pos.z - point.z;
	struct vector incident = normalize(incident_unnorm);

	struct vector cam_incident;
	cam_incident.x = ray.pt.x - point.x;
	cam_incident.y = ray.pt.y - point.y;
	cam_incident.z = ray.pt.z - point.z;
	cam_incident = normalize(cam_incident);
	//printf("cam: %f %f %f hits point %f %f %f, ray = %f %f %f\n", ray.pt.x, ray.pt.y, ray.pt.z, point.x, point.y, point.z, cam_incident.x, cam_incident.y, cam_incident.z);

	struct ray shadowRay;
	shadowRay.pt = point;
	shadowRay.vec = incident;
	//printf("shooting shadow ray from %f %f %f at %f %f %f\n",point.x, point.y, point.z, incident.x, incident.y, incident.z);
	struct intersection occlusion = find_first_intersection(shadowRay, numtris, tris, numspheres, spheres);
	while(occlusion.t < self_ref_marg && occlusion.t > 0) {
		//printf("RECALC!");
		shadowRay.pt = occlusion.point;
		occlusion = find_first_intersection(shadowRay, numtris, tris, numspheres, spheres);
	}
	
	float length_to_light = vectorLength(incident_unnorm);
	//printf("t: %f\n", occlusion.t);
	if (occlusion.t > 0.001 && occlusion.t < length_to_light + 0.001) {
		struct color outp;
		outp.r = 0;
		outp.g = 0;
		outp.b = 0;
		//printf("DETERMINED AS SHADOW: %f\n", occlusion.t);
		return outp;
	} else if (occlusion.t > 0) {
		//printf("interesting: t: %f, length to light: %f\n", occlusion.t, length_to_light);
		//printf("start point: %f %f %f, end point: %f %f %f\n", point.x, point.y, point.z, occlusion.point.x, occlusion.point.y, occlusion.point.z);
	}

	struct color diff_color;
	float normal_inc_dotProduct = dot_product(normal, incident);
	if (normal_inc_dotProduct < 0) {
		normal_inc_dotProduct = 0;
	}

	float cam_normal_inc_dotProduct = dot_product(normal, cam_incident);
	if (cam_normal_inc_dotProduct < 0) {
		cam_normal_inc_dotProduct = 0;
	}

	diff_color.r = objDiff.r * normal_inc_dotProduct * light.diff.r * 256;
	diff_color.g = objDiff.g * normal_inc_dotProduct * light.diff.g * 256;
	diff_color.b = objDiff.b * normal_inc_dotProduct * light.diff.b * 256;

	struct color spec_color;
	struct vector incNormProj; //projection of incident vector onto normal vector
	incNormProj.x = normal_inc_dotProduct * normal.x;
	incNormProj.y = normal_inc_dotProduct * normal.y;
	incNormProj.z = normal_inc_dotProduct * normal.z;

	struct vector camIncNormProj = vector_multiply(normal, cam_normal_inc_dotProduct);

	struct vector perpVector; //vector to add to incident to get reflected vector- perpendicular to normal
	perpVector.x = incident.x - incNormProj.x;
	perpVector.y = incident.y - incNormProj.y;
	perpVector.z = incident.z - incNormProj.z;

	struct vector camPerpVector = vector_subtract(cam_incident, camIncNormProj);

	struct vector reflection; //flip incident vector, add two perpVectors to it
	reflection.x = incident.x - (2 * perpVector.x);
	reflection.y = incident.y - (2 * perpVector.y);
	reflection.z = incident.z - (2 * perpVector.z);

	struct vector camReflection = vector_subtract(cam_incident, vector_multiply(camPerpVector, 2.0));
	
	//printf("Inc dp: %f, ref dp: %f, perp dp: %f\n", dot_product(incident, normal), dot_product(reflection, normal), dot_product(perpVector, normal));

	
	float ray_reflection_dotProduct;
	ray_reflection_dotProduct = -1 * dot_product(ray.vec, reflection);
	if (fabs(dot_product(perpVector, normal)) > 0.01) {
		ray_reflection_dotProduct = 0.0;
	} 
	if (ray_reflection_dotProduct < 0) {
		ray_reflection_dotProduct = 0;
	}

	//printf("ray reflection dp: %f\n", ray_reflection_dotProduct);


	float ray_reflection_with_shininess = pow(ray_reflection_dotProduct, objShin);
	
	spec_color.r = light.spec.r * ray_reflection_with_shininess * objSpec.r * 256;
	spec_color.g = light.spec.g * ray_reflection_with_shininess * objSpec.g * 256;  
	spec_color.b = light.spec.b * ray_reflection_with_shininess * objSpec.b * 256;

	//printf("dp: %f shin: %f, pow: %f\n", ray_reflection_dotProduct, objShin, ray_reflection_with_shininess);

	struct ray reflected_ray;
	reflected_ray.pt = point;
	reflected_ray.vec = camReflection;
	reflected_ray.numBounces = ray.numBounces + 1;
	//printf("current num bounces: %d\n", reflected_ray.numBounces);	
	struct color reflection_color;
	struct color outp_color;
	struct color refl_color;
	refl_color.r = 0;
	refl_color.g = 0;
	refl_color.b = 0;
	
	//if (objSpec.r + objSpec.g + objSpec.b > 0.15) {
		struct intersection refl_intr = find_first_intersection(reflected_ray, numtris, tris, numspheres, spheres);

		while(refl_intr.t > -self_ref_marg && refl_intr.t < self_ref_marg) {
			reflected_ray.pt = refl_intr.point;
			refl_intr = find_first_intersection(reflected_ray, numtris, tris, numspheres, spheres);
		}		

		if (refl_intr.t > 0 && reflected_ray.numBounces <= maxBounces) {
			refl_color = calc_lighting(refl_intr, reflected_ray, light);
			refl_color.r = refl_color.r * objSpec.r;
			refl_color.g = refl_color.g * objSpec.g;
			refl_color.b = refl_color.b * objSpec.b;
			refl_color.r = refl_color.r / 2;
			refl_color.g = refl_color.g / 2;
			refl_color.b = refl_color.b / 2;
			//printf("refl_color: %d %d %d\n", refl_color.r, refl_color.g, refl_color.b);
			//printf("spec color: %d %d %d\n", spec_color.r, spec_color.g, spec_color.b);
			//spec_color.r = (0.5 * spec_color.r + 0.5 * refl_color.r);
			//spec_color.g = (0.5 * spec_color.g + 0.5 * refl_color.g);
			//spec_color.b = (0.5 * spec_color.b + 0.5 * refl_color.b);
			//spec_color = refl_color;
		} else if (reflected_ray.numBounces > maxBounces) {
			//printf("MAX BOUNCED!\n");
			outp_color.r = 0;
			outp_color.g = 0;
			outp_color.b = 0;
			return outp_color;
		}
		//printf("intersection z coord: %f bounces: %d\n", intr.point.z, ray.numBounces);
		//printf("diff color: %d %d %d\n", diff_color.r, diff_color.g, diff_color.b);
		//printf("refl color: %d %d %d\n", refl_color.r, refl_color.g, refl_color.b);
		//printf("spec color: %d %d %d\n", spec_color.r, spec_color.g, spec_color.b);
	//}


	outp_color.r = (int)(diff_color.r + spec_color.r + refl_color.r);
	outp_color.g = (int)(diff_color.g + spec_color.g + refl_color.g);
	outp_color.b = (int)(diff_color.b + spec_color.b + refl_color.b);

	//printf("outp color: r %d, g %d, b %d\n", outp_color.r, outp_color.g, outp_color.b);

	if (outp_color.r == 0 && outp_color.g == 0  && outp_color.b) {
		//printf("MADE IT HERE\n");
	}


	if (outp_color.r > 255) {
		outp_color.r = 255;
	}
	
	if (outp_color.g > 255) {
		outp_color.g = 255;
	}

	if (outp_color.b > 255) {
		outp_color.b = 255;
	}

	//printf("outp color: r %d, g %d, b %d\n", outp_color.r, outp_color.g, outp_color.b);
	return outp_color;
}
