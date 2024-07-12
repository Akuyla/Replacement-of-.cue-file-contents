#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_LINE 1024
#define TARGET_EXT ".cue"

void replace_in_file(const char *file_path);
void process_directory(const char *dir_path);
int has_extension(const char *file_name, const char *ext);

int main() {
    const char *dir_path = "The\\path\\"; // Modify the directory path to contain the ".cue" file, pay attention to the double backslash, and make the subfiles in the folder modifiable
    process_directory(dir_path);
    return 0;
}

void process_directory(const char *dir_path) {
    struct dirent *entry;
    DIR *dp = opendir(dir_path);
    
    if (dp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dp))) {
        // Splice the file path
        char file_path[1024];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (stat(file_path, &statbuf) == -1) {
            perror("stat");
            continue;
        }

        if (S_ISREG(statbuf.st_mode)) {
            // Usin the file name suffix to determine the file type
            if (has_extension(entry->d_name, TARGET_EXT)) {
                replace_in_file(file_path);
            }
        }
    }

    closedir(dp);
}

int has_extension(const char *file_name, const char *ext) {
    const char *dot = strrchr(file_name, '.');
    return dot && strcmp(dot, ext) == 0;
}

void replace_in_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char temp_path[1024];
    snprintf(temp_path, sizeof(temp_path), "%s.temp", file_path);

    FILE *temp_file = fopen(temp_path, "w");
    if (!temp_file) {
        perror("fopen temp file");
        fclose(file);
        return;
    }

    char cue_file_name[256];
    char *cue_file_base_name = NULL;
    // Get the file name of the .cue file (except the suffix)
    strncpy(cue_file_name, file_path, sizeof(cue_file_name));
    cue_file_base_name = strrchr(cue_file_name, '/');
    if (cue_file_base_name == NULL) {
        cue_file_base_name = cue_file_name;
    } else {
        cue_file_base_name++;
    }
    char *dot = strrchr(cue_file_base_name, '.');
    if (dot) {
        *dot = '\0';
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        // Find the target row and replace it
        if (strstr(line, ".flac\" WAVE")) {
            char *start = strstr(line, "\"") + 1;
            char *end = strstr(start, ".flac\" WAVE");
            if (start && end) {
                *end = '\0'; 
                snprintf(line, sizeof(line), "FILE \"%s.flac\" WAVE\n", cue_file_base_name);
            }
        }
        fputs(line, temp_file);
    }

    fclose(file);
    fclose(temp_file);

    // Replace the original file
    if (remove(file_path) != 0) {
        perror("remove");
        // If the deletion fails, try to delete the temporary file(.temp)
        remove(temp_path);
        return;
    }

    if (rename(temp_path, file_path) != 0) {
        perror("rename");
        // If the rename fails, try deleting the temporary file
        remove(temp_path);
        return;
    }
}
