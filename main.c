/*
 * piface_tool
 * 
 * This program reads & sets pins of the piface board, without requiring additional dependencies
 * It includes a slightly modified version of the C library in https://github.com/thomasmacpherson/piface
 * 
 * use piface_tool -h for help
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <string.h>
#include "pfio.h"

/* Place the status in a temp file that must be deleted at boot
 *		/tmp usually is tmpfs, so it's perfect
 */
#ifndef PIFACE_M
#define PIFACE_M "/tmp/piface.status"
#endif

/* Save piface status in this structure */
typedef struct p{
	short piface_init;
	/* We might save some other properties, sooner or later */
} p_mem;

p_mem* s_piface;

/* Block concurrent access to piface status locking the file*/
struct flock fl;

int fd,size;

/* Show debug messages? */
int verbose=0;
/* The piface needs to be configured? */
int init=1;

/* Load status in "shared memory" (mmapped file) */
void get_piface() {
	int pages, new=0;
	
	fd=open(PIFACE_M, O_RDWR);
	if (fd < 0 && errno==ENOENT){
		/* First run: create file and set flag */
		fd=open(PIFACE_M, O_RDWR | O_CREAT, S_IRWXU);
		new=1;
	}

	if (fd < 0){
		perror("fopen");
		exit(1);
	}
	
	fl.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	fl.l_start  = 0;        /* Offset from l_whence         */
	fl.l_len    = 0;        /* length, 0 = to EOF           */
	fl.l_pid    = getpid(); /* our PID                      */
	
	/* Lock "shared memory" */
	if (fcntl(fd, F_SETLKW, &fl)==-1){  /* F_GETLK, F_SETLK, F_SETLKW */
		perror("fcntl");
		exit(1);
	}
	
	/* Mmapped memory must be aligned to page size */
	pages=1+((sizeof(p_mem)-1)/sysconf(_SC_PAGE_SIZE));
	size=pages*sysconf(_SC_PAGE_SIZE);
	
	/* Enlarge the file to the defined size */
	if (ftruncate(fd,size)<0){
		perror("ftruncate");
		exit(1);
	}
	/* Allocate the "shared memory" */
	s_piface=(p_mem*) mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (s_piface==MAP_FAILED){
		perror("mmap");
		exit(1);
	}
	
	/* If this is the first run, the piface must be initialised (the -n option can override this) */
	if (new && init)
		s_piface->piface_init=0;
}



int main(int argc, char** argv) {
	int c,index;

	/* Mode: in | out */
	char mode[8];
	/* Pin number */
	short pin=0;
	/* Pin value (only used with mode=out) */
	short value=0;
    
    /* Load settings from commandline */
    while ((c = getopt (argc, argv, "nvh")) != -1){
        switch (c){
			case 'v':
				verbose++;
				break;
			case 'n':
				init=0;
				break;
            case 'h':
                printf("Usage: piface_tool [options] <pin number> [in|out [<value>]]\n"
                    "Read/Set the piface pins.\n"
					"Pin '8' is a shortcut for all pins.\n\n"
					"-n\t\tForce no init (use if other programs are using the piface)\n"
                    "-v\t\tVerbose\n"
                    "-h\t\tShow this message\n\n"
                );
                exit (1);
        }
    }
	index = optind;
	if (index == argc){
		printf("Missing argument(s)\n");
		exit(1);
	}
	pin=atoi(argv[index]);
	if (pin<0 || pin>8){
		printf("Invalid pin: %d\n",pin);
		exit(1);
	}
	index++;
	
	snprintf(mode,7,"%s",argv[index]);
	index++;
	
	if (index < argc)
		value=atoi(argv[index]);
	if (value<0 || value>1){
		printf("Invalid value: %d\n",pin);
		exit(1);
	}

	if (verbose)
		fprintf(stderr,"Loading previous state\n");
	get_piface();
	
	if (verbose)
		fprintf(stderr,"Opening SPI port\n");
	pfio_init_spi();
	
	if (! s_piface->piface_init){
		/* The piface must be configured */
		if (verbose)
			fprintf(stderr,"Initializing piface...\n");
		
		/* Configure the piface */
		pfio_init_piface();
		/* Turn off all outputs */
		pfio_reset();
		
		/* The piface is ready! */
		s_piface->piface_init=1;
		
		if (verbose)
			fprintf(stderr,"Piface ready\n");
	}

	if (strcmp(mode,"in")==0){
	/* Read pin */
		if (pin==8){
			while (pin>0){
				pin--;
				printf("%d\n", pfio_digital_read(pin));
			}
		}
		printf("%d\n",pfio_digital_read(pin));
	}
	else if (strcmp(mode,"out")==0){
	/* Write value to pin */
		if (pin==8){
			while (pin>0){
				pin--;
				pfio_digital_write(pin,value);
			}
		}
		pfio_digital_write(pin,value);
		
	}
	else{
		printf("Invalid mode: %s\n",mode);
		exit(1);
	}
	
	/* Close SPI port */
	pfio_deinit();
	
	/* Relase the lock and the shared memory */
	munmap(s_piface,size);
	if (fcntl(fd, F_UNLCK, &fl)==-1){  /* F_GETLK, F_SETLK, F_SETLKW */
		perror("fcntl");
		exit(1);
	}
	
	return (EXIT_SUCCESS);
}
