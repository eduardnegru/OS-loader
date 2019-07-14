/*
 * Loader Implementation
 * Negru Adrian Eduard
 * 332CC
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "exec_parser.h"
#include "utils.h"

static so_exec_t *exec;
static struct sigaction old_action;
static int file_descriptor;

static void segv_handler(int signum, siginfo_t *info, void *context)
{
	int i;
	int address_not_in_segment = 0;
	char *faultAddress = (char *)info->si_addr;
	int page_size = getpagesize();
	unsigned int temp_full_permission = PROT_READ | PROT_EXEC | PROT_WRITE;

	for (i = 0; i < exec->segments_no; i++) {

		char *mem_address = (char *)exec->segments[i].vaddr + exec->segments[i].mem_size;
		char *file_address = (char *)exec->segments[i].vaddr + exec->segments[i].file_size;
		int page_index = (faultAddress - (char *)exec->segments[i].vaddr) / page_size;
		unsigned int permissions = exec->segments[i].perm;

		if (mem_address < faultAddress) {
			address_not_in_segment++;
		} else {
			// address not mapped
			if (info->si_code == SEGV_MAPERR) {
				char *aligned_address = (char *)ALIGN_DOWN((uintptr_t)faultAddress, page_size);
				off_t offset = exec->segments[i].offset + page_index * page_size;
				char *mapped_address;

				if (faultAddress <  mem_address && faultAddress > file_address) {
					mapped_address = mmap(aligned_address, page_size, temp_full_permission, MAP_FIXED | MAP_PRIVATE, file_descriptor, offset);
					DIE(mapped_address == MAP_FAILED, "Address could not be mapped");
				} else {
					mapped_address = mmap(aligned_address, page_size, temp_full_permission, MAP_ANONYMOUS | MAP_PRIVATE, file_descriptor, offset);
					DIE(mapped_address == MAP_FAILED, "Address could not be mapped");
				}

				memset(mapped_address, 0, page_size);

				if (exec->segments[i].file_size < exec->segments[i].mem_size
					&& aligned_address + page_size > file_address) {
					if (aligned_address < file_address) {
						int length = file_address - aligned_address;
						char *buffer = (char *)calloc(length, sizeof(char));

						lseek(file_descriptor, 0, SEEK_SET);
						lseek(file_descriptor, exec->segments[i].offset + page_index * page_size, SEEK_SET);
						read(file_descriptor, buffer, length);
						memcpy(mapped_address, buffer, length);
						free(buffer);
					} else {
						//do nothing. memory is already set to 0 by memset
					}
				} else {
					char *buffer = (char *)calloc(page_size, sizeof(char));

					lseek(file_descriptor, 0, SEEK_SET);
					lseek(file_descriptor, exec->segments[i].offset + page_index * page_size, SEEK_SET);
					read(file_descriptor, buffer, page_size);
					memcpy(mapped_address, buffer, page_size);
					free(buffer);
				}
				//setting permissions on the mapped memory
				int rc = mprotect(mapped_address, page_size, permissions);

				DIE(rc == -1, "mprotect failed giving segment permissions");
				break;
			}
			// seg fault and mapped => invalid permissions => default handler
			old_action.sa_sigaction(signum, info, context);

		}
		// address is outside any segment => default handler
		if (address_not_in_segment == exec->segments_no)
			old_action.sa_sigaction(signum, info, context);
	}
}

int so_init_loader(void)
{
	struct sigaction action;
	int rc;

	action.sa_sigaction = segv_handler;
	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGSEGV);
	action.sa_flags = SA_SIGINFO;

	rc = sigaction(SIGSEGV, &action, &old_action);
	DIE(rc == -1, "sigaction");
	return -1;
}



int so_execute(char *path, char *argv[])
{
	// needed by mmap
	file_descriptor = open(path, O_RDWR);

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	close(file_descriptor);
	return -1;
}
