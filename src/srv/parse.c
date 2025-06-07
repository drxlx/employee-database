#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    int i = 0;
    for (; i < dbhdr->count; i++) {
        printf("Employee %d\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employeeptr, char *addstring) {
	printf("DB currently has %d\n", dbhdr->count);

	char *name = strtok(addstring, ",");
	if (name == NULL) {
		return STATUS_ERROR;
	}

	char *addr = strtok(NULL, ",");
	if (addr == NULL) {
		return STATUS_ERROR;
	}

	char *hours = strtok(NULL, ",");
	if (hours == NULL || atoi(hours) == 0) {
		return STATUS_ERROR;
	}
	
	dbhdr->count++;
	*employeeptr = realloc(*employeeptr, dbhdr->count*(sizeof(struct employee_t)));
	struct employee_t *employees = *employeeptr;

	strncpy(employees[dbhdr->count-1].name, name, sizeof(employees[dbhdr->count-1].name));

	strncpy(employees[dbhdr->count-1].address, addr, sizeof(employees[dbhdr->count-1].address));

	employees[dbhdr->count-1].hours = atoi(hours);

	return STATUS_SUCCESS;
}

int remove_employee(struct dbheader_t *dbhdr, struct employee_t *employees, char *removenamestring) {
    int deleteIndex = -1;
    
    int i = 0;
    for (; i < dbhdr->count; i++) {
        if (strcmp(employees[i].name, removenamestring) == 0) {
            deleteIndex = i;
            break;
        }
    }

    if (deleteIndex != -1) {
        i = deleteIndex;
        for (; i < dbhdr->count-1; i++) {
            employees[i] = employees[i + 1];
        }
    } else {
        printf("There is no employee with name %s\n", removenamestring);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int update_hours(struct dbheader_t *dbhdr, struct employee_t *employees, char *updatehoursstring) {
    char *name = strtok(updatehoursstring, ",");
    char *hours = strtok(NULL, ",");

    int updateIndex = -1;

    int i = 0;
    for (; i < dbhdr->count; i++) {
        if (strcmp(employees[i].name, name) == 0) {
            updateIndex = i;
            break;
        }
    }

    if (updateIndex != -1) {
        employees[updateIndex].hours = atoi(hours);
    } else {
        printf("There is no employee with name %s\n", name);
        return STATUS_ERROR;
    }

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == -1) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count*sizeof(struct employee_t));

    int i = 0;
    for (; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Got a bad FD from the user \n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;
    long realfilesize = sizeof(struct dbheader_t) + sizeof(struct employee_t) * realcount;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->version = htons(dbhdr->version);
    dbhdr->count = htons(dbhdr->count);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));

    lseek(fd, 0, SEEK_SET);

    write(fd, dbhdr, sizeof(struct dbheader_t));

    if (dbhdr->count > 0) {
        int i = 0;
        for (; i < realcount; i++) {
            employees[i].hours = htonl(employees[i].hours);
            write(fd, &employees[i], sizeof(struct employee_t));
            employees[i].hours = ntohl(employees[i].hours);
        }
    }

    dbhdr->magic = ntohl(dbhdr->magic);
	dbhdr->filesize = ntohl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
	dbhdr->count = ntohs(dbhdr->count);
	dbhdr->version = ntohs(dbhdr->version);

    ftruncate(fd, realfilesize);

    return STATUS_SUCCESS;
}	

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user \n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    
    if (header == -1) {
        printf("Malloc failed to create database header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->magic = ntohl(header->magic);
    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return STATUS_ERROR;
    }

    if (header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
	struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    
    if (header == -1) {
        printf("Malloc failed to create database header\n");
        return STATUS_ERROR;
    }

    header->magic = HEADER_MAGIC;
    header->version = 0x1;
    header->count = 0;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}


