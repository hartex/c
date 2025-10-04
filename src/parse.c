#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "common.h"
#include "parse.h"

int read_employees(int fd, struct dbheader_t* dbhdr, struct employee_t** employeesOut)
{
    return STATUS_SUCCESS;
}

int output_file(const int fd, struct dbheader_t* dbhdr, struct employee_t *employees)
{
    if (fd < 0)
    {
        printf("Got a wrong file descriptor");
        return STATUS_ERROR;
    }
    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(dbhdr->filesize);
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);
    lseek(fd, 0, SEEK_SET);

    if (write(fd, dbhdr, sizeof(struct dbheader_t)) < 0)
    {
        printf("Failed to write to a file");
        perror("write");
        return STATUS_ERROR;
    }
    return STATUS_SUCCESS;
}

int validate_db_header(const int fd, struct dbheader_t** headerOut)
{
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

int create_db_header(int fd, struct dbheader_t** headerOut)
{
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
