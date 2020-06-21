#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "math.h"
#include "pthread.h"
#include "time.h"
#include "TinyPngOut.h"

const int ITER_LIMIT = 255;

uint8_t *PIXELS;
double *X_COORDS;
double *Y_COORDS;

double LIMITS[] = {-2, +2, -1, +1};

int WIDTH = 320;
int HEIGHT = 240;
int NTHREADS = 1;

int indices[128];
pthread_t * threads;


//
static int printError(enum TinyPngOut_Status status) {
	const char *msg;
	switch (status) {
		case TINYPNGOUT_OK              :  msg = "OK";                break;
		case TINYPNGOUT_INVALID_ARGUMENT:  msg = "Invalid argument";  break;
		case TINYPNGOUT_IMAGE_TOO_LARGE :  msg = "Image too large";   break;
		case TINYPNGOUT_IO_ERROR        :  msg = "I/O error";         break;
		default                         :  msg = "Unknown error";     break;
	}
	fprintf(stderr, "Error: %s\n", msg);
	return EXIT_FAILURE;
}


int png_write()
{
	// Open output file
	FILE *fout = fopen("zad19.png", "wb");
	if (fout == NULL) {
		perror("Error: fopen");
		return EXIT_FAILURE;
	}
	
	// Initialize Tiny PNG Output
	struct TinyPngOut pngout;
	enum TinyPngOut_Status status = TinyPngOut_init(&pngout, (uint32_t)WIDTH, (uint32_t)HEIGHT, fout);
	if (status != TINYPNGOUT_OK)
		return printError(status);

	// Write image data
	status = TinyPngOut_write(&pngout, PIXELS, (size_t)(WIDTH * HEIGHT));
	if (status != TINYPNGOUT_OK)
		return printError(status);

	// Close output file
	if (fclose(fout) != 0) {
		perror("Error: fclose");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int gen_coords()
{
	int i;
	double x_diff, y_diff;

	x_diff = (LIMITS[1]-LIMITS[0])/(WIDTH+1);
	X_COORDS[0] = LIMITS[0] + x_diff/2;
	
	for (i = 1; i < WIDTH; i++)
		X_COORDS[i] = X_COORDS[i-1]+x_diff;

	y_diff = (LIMITS[3]-LIMITS[2])/(HEIGHT+1);
	Y_COORDS[0] = LIMITS[2] + y_diff/2;
	
	for (i = 1; i < HEIGHT; i++)
		Y_COORDS[i] = Y_COORDS[i-1]+y_diff;

	return 0;
}

void *write_pixels(void *arg)
{
	struct timespec spec_a, spec_b;
	double start, end;
	int i, j, k, blue, pixel;
	double cx, cy, x, y, zx = 0, zy = 0, xt = 0, yt = 0;
	int *index = (int*) arg;

	clock_gettime(CLOCK_REALTIME, &spec_a);
	printf("Thread-%d started.\n", *index);
	
	for (pixel = *index; pixel < HEIGHT*WIDTH; pixel+=NTHREADS)
	{
		i = pixel%WIDTH;
		j = pixel/WIDTH;

		cx = X_COORDS[i];
		cy = Y_COORDS[j];
		zx = 0;
		zy = 0;

		for(k = 0; k < ITER_LIMIT; k++)
		{
			xt = exp(zx*zx-zy*zy+cx);
			yt = 2*zx*zy + cy;
			x = xt * cos(yt);
			y = xt * sin(yt);

			if (x*x+y*y > 4.0) break;
			zx = x;
			zy = y;
		}
			
		if(k==ITER_LIMIT) continue;

		if(k <= 15)
			blue = 17*k;
		else
			blue = 90 + k;

		blue > ITER_LIMIT? blue = ITER_LIMIT: 1;

		PIXELS[3*pixel] = ITER_LIMIT - blue;
		PIXELS[3*pixel+1] = ITER_LIMIT - k;
		PIXELS[3*pixel+2] = ITER_LIMIT;
	}

	clock_gettime(CLOCK_REALTIME, &spec_b);
	
	start = spec_a.tv_sec*1000 + spec_a.tv_nsec/1000000;
	end = spec_b.tv_sec*1000 + spec_b.tv_nsec/1000000;
	
	printf("Thread-%d stopped.\n", *index);
	printf("Thread-%d execution time was (milis): %lf.\n", *index, end-start);
	return 0;
}

int gen_pixels()
{
	int i;

	for(i = 0; i < NTHREADS; i++)
		indices[i] = i;

	for (i = 1; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, &write_pixels, &indices[i]);
	
	write_pixels(&indices[0]);

	for (i = 1; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
}

void malloc_all()
{
	PIXELS = (uint8_t*)malloc(3*WIDTH*HEIGHT*sizeof(uint8_t));
	X_COORDS = (double*)malloc(WIDTH*sizeof(double));
	Y_COORDS = (double*)malloc(HEIGHT*sizeof(double));
	threads = (pthread_t*)malloc(NTHREADS*sizeof(pthread_t));
}

void free_all()
{
	free(threads);
	free(Y_COORDS);
	free(X_COORDS);
	free(PIXELS);
}

int main()
{
	struct timespec spec_a, spec_b;
	double start, end;
	WIDTH = 3000;
	HEIGHT = 3000;
	LIMITS[0] = 0;
	LIMITS[1] = 0.5;
	LIMITS[2] = 0.5;
	LIMITS[3] = 1;
	NTHREADS = 8;

	clock_gettime(CLOCK_REALTIME, &spec_a);

	malloc_all();
	gen_coords();
	gen_pixels();
	png_write();
	free_all();

	clock_gettime(CLOCK_REALTIME, &spec_b);

	start = spec_a.tv_sec*1000 + spec_a.tv_nsec/1000000;
	end = spec_b.tv_sec*1000 + spec_b.tv_nsec/1000000;
	
	printf("Threads used in current run: %d\n", NTHREADS);
	printf("Total execution time for current run (millis): %lf.\n", end-start);
	return 0;
}