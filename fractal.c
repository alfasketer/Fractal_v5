#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "stdint.h"
#include "math.h"
#include "pthread.h"
#include "time.h"
#include "string.h"
#include "TinyPngOut.h"

const int ITER_LIMIT = 255;

char quiet = 0;

int WIDTH = 640;
int HEIGHT = 480;
int NTHREADS = 1;

uint8_t *PIXELS;
double *X_COORDS;
double *Y_COORDS;

long block_index = 0;
int granularity = 1;
int block_size = 1;


pthread_t * threads;
pthread_mutex_t lock;

double LIMITS[] = {-2, +2, -1, +1};
int indices[128];

char *file_name = NULL;


static void png_error(enum TinyPngOut_Status status) {
	const char *msg;
	switch (status) {
		case TINYPNGOUT_OK:
			msg = "OK";
			break;
		case TINYPNGOUT_INVALID_ARGUMENT:
			msg = "Invalid argument";
			break;
		case TINYPNGOUT_IMAGE_TOO_LARGE:
			msg = "Image too large";
			break;
		case TINYPNGOUT_IO_ERROR:
			msg = "I/O error";
			break;
		default:
			msg = "Unknown error";
			break;
	}
	fprintf(stderr, "Error: %s\n", msg);
}


int png_write()
{
	// Open output file

	FILE *fout;
	
	if(file_name == NULL) fout = fopen("zad19.png", "wb");
	else fout = fopen(file_name, "wb");

	if (fout == NULL) {
		perror("Error: fopen");
		return EXIT_FAILURE;
	}
	
	// Initialize Tiny PNG Output
	struct TinyPngOut pngout;
	enum TinyPngOut_Status status = TinyPngOut_init(&pngout, (uint32_t)WIDTH, (uint32_t)HEIGHT, fout);
	if (status != TINYPNGOUT_OK)
	{
		png_error(status);
		return EXIT_FAILURE;
	}

	// Write image data
	status = TinyPngOut_write(&pngout, PIXELS, (size_t)(WIDTH * HEIGHT));
	if (status != TINYPNGOUT_OK)
	{
		png_error(status);
		return EXIT_FAILURE;	
	}

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

	x_diff = (LIMITS[1]-LIMITS[0])/WIDTH;
	X_COORDS[0] = LIMITS[0] + x_diff/2;
	
	for (i = 1; i < WIDTH; i++)
		X_COORDS[i] = X_COORDS[i-1]+x_diff;

	y_diff = (LIMITS[3]-LIMITS[2])/HEIGHT;
	Y_COORDS[0] = LIMITS[2] + y_diff/2;
	
	for (i = 1; i < HEIGHT; i++)
		Y_COORDS[i] = Y_COORDS[i-1]+y_diff;

	return 0;
}

void *write_pixels(void *arg)
{
	struct timespec spec_a, spec_b;
	double start, end;
	int i, j, l, k, blue, pixel, p_index, count = 0;
	double cx, cy, x, y, zx = 0, zy = 0, xt = 0, yt = 0;
	int *index = (int*) arg;

	clock_gettime(CLOCK_REALTIME, &spec_a);
	if (!quiet) printf("Thread-%d started.\n", *index);
	
	while (1)
	{
		pthread_mutex_lock(&lock);
			if (block_index < WIDTH*HEIGHT)
			{
				p_index = block_index;
				block_index+=block_size;
				count++;
			}
			else
			{
				pthread_mutex_unlock(&lock);
				break;
			}

		pthread_mutex_unlock(&lock);

		for (l = 0; l < block_size; l++)
		{
			pixel = p_index+l;

			if (pixel >= WIDTH*HEIGHT) break;

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

				if (sqrt(x*x+y*y) > 400000000) break;
				zx = x;
				zy = y;
			}
				
			if(k==ITER_LIMIT) continue;

			if(k <= 15)
				blue = 18*k;
			else
				blue = 270 + k;

			blue > ITER_LIMIT? blue = ITER_LIMIT: 1;

			PIXELS[3*pixel] = ITER_LIMIT - (blue > 255? 511 - blue: blue);
			PIXELS[3*pixel+1] = ITER_LIMIT - k;
			PIXELS[3*pixel+2] = ITER_LIMIT;
		}
	}

	clock_gettime(CLOCK_REALTIME, &spec_b);
	
	start = spec_a.tv_sec*1000 + spec_a.tv_nsec/1000000;
	end = spec_b.tv_sec*1000 + spec_b.tv_nsec/1000000;
	
	if (!quiet) printf("Thread-%d stopped.\n", *index);
	if (!quiet) printf("Thread-%d calculated %d blocks.\n", *index, count);
	if (!quiet) printf("Thread-%d execution time was (milis): %lf.\n", *index, end-start);
	return 0;
}

int gen_pixels()
{
	int i;

	if (granularity != -1) block_size = (WIDTH*HEIGHT)/(granularity*NTHREADS);
	if ((WIDTH*HEIGHT)%(granularity*NTHREADS)>0) block_size++;

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
	if(file_name != NULL) free(file_name);
}

void get_resolution(char* input)
{
	int x, y;
	char format_warning[] = "ERROR: The value of option -s must be in the following format: <WIDTH>x<HEIGHT>.\n";
	char value_warning[] = "%sERROR: The value %s of option -s must be numerical and larger than zero.\n%s";
	char revert_message[] = "\tReverting to default.\n";
	const char s[2] = "x";
	char* curr;

	if (input == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }

	curr = strtok(input, s);
	x = atoi(curr);
	if (x == 0) { fprintf(stderr, value_warning, format_warning, "<WIDTH>", revert_message); return; }
	
	curr = strtok(NULL, s);
	if (curr == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }

	y = atoi(curr);
	if (y == 0) { fprintf(stderr, value_warning, format_warning, "<HEIGHT>", revert_message); return; }

	curr = strtok(NULL, s);
	if (curr != NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }

	WIDTH = x;
	HEIGHT = y;
}

void get_dimensions(char* input)
{
	double a, b, c, d;
	char format_warning[] = "ERROR: The value of option -r must be in the following format:\n\t\"<LEFT_LIMIT>:<RIGHT_LIMIT>:<DOWN_LIMIT>:<UP_LIMIT>\".\n";
	char value_warning[] = "%sERROR: The value %s of option -r must be smaller than the value %s.\n%s";
	char revert_message[] = "\tReverting to default.\n";
	const char s[2] = ":";
	char* curr;

	if (input == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }

	curr = strtok(input, s);
	a = atof(curr);
	
	curr = strtok(NULL, s);
	if (curr == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }
	b = atof(curr);

	curr = strtok(NULL, s);
	if (curr == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }
	c = atof(curr);

	curr = strtok(NULL, s);
	if (curr == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }
	d = atof(curr);

	curr = strtok(NULL, s);
	if (curr != NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); return; }

	if (a >= b) { fprintf(stderr, value_warning, format_warning, "<LEFT_LIMIT>", "<RIGHT_LIMIT>", revert_message); return; }
	if (c >= d) { fprintf(stderr, value_warning, format_warning, "<DOWN_LIMIT>", "<UP_LIMIT>", revert_message); return; }

	LIMITS[0] = a;
	LIMITS[1] = b;
	LIMITS[2] = c;
	LIMITS[3] = d;
}

void get_output_file(char* input)
{
	char format_warning[] = "ERROR: The output file must be in a name.png format.\n";
	char revert_message[] = "\tReverting to default.\n";

	int len = strlen(input) + 1;
	char* temp;
	temp = (char*)malloc(len);
	strcpy(temp, input);

	const char s[2] = ".";
	char* curr;

	if (input == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); free(temp); return; }

	curr = strtok(input, s);
	curr = strtok(NULL, s);

	if (curr == NULL) { fprintf(stderr, "%s%s", format_warning, revert_message); free(temp); return; }
	if (!!strcmp(curr, "png")) { fprintf(stderr, "%s%s", format_warning, revert_message); free(temp); return; }

	file_name = temp;
}

void get_num_threads(char* input)
{
	char value_warning[] = "ERROR: The value of option -t must be numerical and larger than zero.\n";
	char revert_message[] = "\tReverting to default.\n";
	int num = atoi(input);
	
	if (num == 0) { fprintf(stderr, "%s%s", value_warning, revert_message); return; }

	NTHREADS = num;
}

void get_granularity(char* input)
{
	char value_warning[] = "ERROR: The value of option -g must be numerical and larger than zero.\n";
	char revert_message[] = "\tReverting to default.\n";

	if(!strcmp(input, "MAX"))
	{
		granularity = -1;
		block_size = 1;
		return;
	}

	int num = atoi(input);
	
	if (num <= 0) { fprintf(stderr, "%s%s", value_warning, revert_message); return; }

	granularity = num;
}


int parse_args(int argc, char* argv[])
{
	int opt;
	
	while (1) {
		opt = getopt(argc, argv, "qs:r:o:t:g:");
		if(opt == -1) break;
		switch (opt) {
		case 'q':
			quiet = 1;
			break;
		case 's':
			get_resolution(optarg);
			break;
	  	case 'r':
			get_dimensions(optarg);
			break;
	  	case 'o':
			get_output_file(optarg);
			break;
	  	case 't':
			get_num_threads(optarg);
			break;
		case 'g':
			get_granularity(optarg);
			break;
		default:
			break;
		}
	}
}

int main(int argc, char* argv[])
{
	struct timespec spec_a, spec_b;
	double start, end;

	clock_gettime(CLOCK_REALTIME, &spec_a);

	if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n ERROR: Mutex init failed\n");
        return 1;
    }

	parse_args(argc, argv);
	malloc_all();
	gen_coords();
	gen_pixels();
	
	clock_gettime(CLOCK_REALTIME, &spec_b);

	png_write();
	free_all();

	start = spec_a.tv_sec*1000 + spec_a.tv_nsec/1000000;
	end = spec_b.tv_sec*1000 + spec_b.tv_nsec/1000000;
	
	if (!quiet) printf("Threads used in current run: %d\n", NTHREADS);
	if (!quiet) printf("Total execution time for current run with %d threads and granularity %d (millis): %lf.\n", NTHREADS, granularity, end-start);

	return 0;
}