/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include "exec_parser.h"

static so_exec_t *exec;
static struct sigaction oldHandler;
static int fd;
static int pageSize;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	// check if the signal is SIGSEGV
	if (signum != SIGSEGV || info->si_code == SEGV_ACCERR) {
		oldHandler.sa_sigaction(signum, info, context);
		return;
	}

	void *mapped;
	char *addr = (char *)info->si_addr;
	so_seg_t *found_segment = NULL;

	// find the segment which caused the page fault
	for (int i = 0; i < exec->segments_no; i++) {
		so_seg_t *segment = &(exec->segments[i]);
			char *startAddr = (char *)segment->vaddr;
			char *endAddr = (char *)(startAddr + segment->mem_size);

			if (addr <= endAddr && addr >= startAddr)
			{
				found_segment = segment;
				break;
			}
	}

	// find the index of the page
	int index = (int)(addr - found_segment->vaddr) / pageSize;
	char *page = (char *)found_segment->data;

	// check if the page of the segment is already mapped
	// 0 = unmapped; 1 = mapped
	if (page[index] == 0) {

		// map the address where the page fault occured and load the information
		mapped = mmap((char *)(found_segment->vaddr + pageSize * index), pageSize,
						PROT_WRITE, MAP_PRIVATE | MAP_FIXED, fd,
						found_segment->offset + pageSize * index);

		if (mapped == MAP_FAILED)
			exit(EXIT_FAILURE);

		// bss case: set the rest of the memory to 0
		if (found_segment->file_size < found_segment->mem_size
								&& (index + 1) * pageSize > found_segment->file_size) {
			if (memset((char *)(found_segment->vaddr + found_segment->file_size), 0,
							(index + 1) * pageSize - found_segment->file_size) < 0) {
				perror("memset failed");
				exit(EXIT_FAILURE);
			}
		}

		// set the permissions of "mapped"
		if (mprotect(mapped, pageSize, found_segment->perm) < 0) {
			perror("mprotect failed");
			exit(EXIT_FAILURE);
		}
	} else {
		oldHandler.sa_sigaction(signum, info, context);
		return;
	}

	// set the page as mapped
	page[index] = 1;
}

int so_init_loader(void)
{
	pageSize = sysconf(_SC_PAGE_SIZE);
	int rc;
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	// open the file descriptor
	fd = open(path, O_RDONLY);
	if (!fd)
		return -1;

	// allocate memory for data section and make data[i][j] unmapped
	for (int i = 0; i < exec->segments_no; i++) {
		so_seg_t *segment = &exec->segments[i];
		int number_of_pages = ceil(segment->mem_size / pageSize);
		char *page;

		segment->data = malloc(number_of_pages * sizeof(char));
		if (segment->data == NULL)
			return -1;

		page = (char *)segment->data;
		for (int j = 0; j < number_of_pages; j++)
			page[j] = 0;
	}

	so_start_exec(exec, argv);

	return -1;
}
