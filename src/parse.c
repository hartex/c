#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "common.h"
#include "parse.h"

#include <string.h>

int read_employees(const int fd, struct dbheader_t* dbhdr, struct employee_t** employeesOut)
{
    if (fd < 0)
    {
        printf("Got a wrong file descriptor");
        return STATUS_ERROR;
    }

    if (dbhdr == NULL)
    {
        printf("Null pointer passed as header\n");
        return STATUS_ERROR;
    }

    if (employeesOut == NULL)
    {
        printf("Null pointer passed as employees list\n");
        return STATUS_ERROR;
    }

    unsigned short count = dbhdr->count;

    struct employee_t* employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL)
    {
        printf("Malloc failed to create db employee\n");
        free(employees);
        return STATUS_ERROR;
    }

    unsigned long expectedBytes = count * sizeof(struct employee_t);
    if (read(fd, employees, expectedBytes) != expectedBytes)
    {
        printf("Reading of employees from db file was unsuccessful\n");
        free(employees);
        return STATUS_ERROR;
    }
    for (int i = 0; i < count; i++)
    {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int list_employees(struct dbheader_t *dbhdr, struct employee_t *employees)
{
    if (dbhdr == NULL)
    {
        printf("Null pointer passed as header\n");
        return STATUS_ERROR;
    }

    if (employees == NULL)
    {
        printf("Null pointer passed as employees list\n");
        return STATUS_ERROR;
    }

    for (int i = 0; i < dbhdr->count; i++)
    {
        printf("Employee %d\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }

    return STATUS_SUCCESS;
}

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring)
{
    if (dbhdr == NULL)
    {
        printf("Null pointer passed as header\n");
        return STATUS_ERROR;
    }

    if (addstring == NULL)
    {
        printf("Null pointer passed as addstring\n");
        return STATUS_ERROR;
    }

    if (employees == NULL)
    {
        printf("Null pointer passed as employees list\n");
        return STATUS_ERROR;
    }

    if (*employees == NULL)
    {
        printf("Null pointer passed as employees list\n");
        return STATUS_ERROR;
    }

    dbhdr->count++;

    struct employee_t * new_employees = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
    if (new_employees == NULL)
    {
        printf("Realloc failed to allocate memory for new employee\n");
        free(new_employees);
        return STATUS_ERROR;
    }

    char* name = strtok(addstring, ",");
    char* addr = strtok(NULL, ",");
    char* hours = strtok(NULL, ",");

    strncpy(new_employees[dbhdr->count-1].name, name, sizeof(new_employees[dbhdr->count-1].name));
    strncpy(new_employees[dbhdr->count-1].address, addr, sizeof(new_employees[dbhdr->count-1].address));

    new_employees[dbhdr->count - 1].hours = atoi(hours);
    *employees = new_employees;

    return STATUS_SUCCESS;
}

int output_file(const int fd, struct dbheader_t* dbhdr, struct employee_t* employees)
{
    if (dbhdr == NULL)
    {
        printf("Null pointer passed as header\n");
        return STATUS_ERROR;
    }

    if (employees == NULL)
    {
        printf("Null pointer passed as employees list\n");
        return STATUS_ERROR;
    }

    if (fd < 0)
    {
        printf("Got a wrong file descriptor");
        return STATUS_ERROR;
    }
    unsigned short employeesCount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * employeesCount));
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);
    lseek(fd, 0, SEEK_SET);

    if (write(fd, dbhdr, sizeof(struct dbheader_t)) < 0)
    {
        printf("Failed to write header to a file");
        perror("write");
        return STATUS_ERROR;
    }

    for (int i = 0; i < employeesCount; i++)
    {
        employees[i].hours = htonl(employees[i].hours);
        if (write(fd, &employees[i], sizeof(struct employee_t)) < 0)
        {
            printf("Failed to write employee %d to a file", i);
            perror("write");
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

int validate_db_header(const int fd, struct dbheader_t** headerOut)
{
    if (headerOut == NULL)
    {
        printf("Null pointer is passed as headerOut\n");
        return STATUS_ERROR;
    }

    if (fd < 0)
    {
        printf("Got a wrong file descriptor");
        return STATUS_ERROR;
    }

    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL)
    {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t))
    {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC)
    {
        printf("Improper header version\n");
        free(header);
        return STATUS_ERROR;
    }

    if (header->version != 1)
    {
        printf("Incompatible header version\n");
        free(header);
        return STATUS_ERROR;
    }

    struct stat db_stat = {0};
    fstat(fd, &db_stat);
    if (header->filesize != db_stat.st_size)
    {
        printf("Corrupted db\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t** headerOut)
{
    if (headerOut == NULL)
    {
        printf("Null pointer is passed as headerOut\n");
        return STATUS_ERROR;
    }

    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL)
    {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;
    return STATUS_SUCCESS;
}
