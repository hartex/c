#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char* argv[])
{
    printf("Usage: %s -n -f <db file path>\n", argv[0]);
    printf("\t -n - create new db file\n");
    printf("\t -t - (required) path to database file\n");
}

int main(int argc, char* argv[])
{
    int c;
    bool newfile = false;
    char* filepath = NULL;
    char* addString = NULL;
    bool listEmployees = false;

    int dbfd = -1;
    struct dbheader_t* header = NULL;
    struct employee_t* employees = NULL;

    while ((c = getopt(argc, argv, "nf:a:l")) != -1)
    {
        switch (c)
        {
        case 'n':
            newfile = true;
            break;
        case 'f':
            filepath = optarg;
            break;
        case 'a':
            addString = optarg;
            break;
        case 'l':
            listEmployees = true;
            break;
        case '?':
            printf("Unknown option -%c\n", c);
            break;
        default:
            return -1;
        }
    }

    if (filepath == NULL)
    {
        printf("Filepath is a required argument\n");
        print_usage(argv);
        return 0;
    }

    if (newfile)
    {
        dbfd = create_db_file(filepath);
        if (dbfd == STATUS_ERROR)
        {
            printf("Unable to create database file\n");
            return -1;
        }

        if (create_db_header(&header) == STATUS_ERROR)
        {
            printf("Failed to create db header\n");
            return -1;
        }
    }
    else
    {
        dbfd = open_db_file(filepath);
        if (dbfd == STATUS_ERROR)
        {
            printf("Unable to open database file\n");
            return -1;
        }
        if (validate_db_header(dbfd, &header) == STATUS_ERROR)
        {
            printf("Failed to validate db header\n");
            return -1;
        }
    }

    if (read_employees(dbfd, header, &employees) != STATUS_SUCCESS)
    {
        printf("Failed to read employees\n");
        return -1;
    }

    if (addString)
    {
        header->count++;
        employees = realloc(employees, header->count * sizeof(struct dbheader_t));
        if (employees == NULL)
        {
            printf("Realloc failed to allocate memory for new employee\n");
            free(employees);
            return -1;
        }

        if (add_employee(header, employees, addString) != STATUS_SUCCESS)
        {
            printf("Failed to add employee to the list\n");
            return -1;
        }
    }

    output_file(dbfd, header, employees);

    return 0;
}
