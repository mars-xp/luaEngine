#include <jni.h>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <linux/input.h>
#include <errno.h>
#include <unistd.h>

#include <android/log.h>
#include <locale>
#include <__hash_table>

#define LOG_TAG  "native-lib"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#define LOGS(...)  __android_log_print(ANDROID_LOG_SILENT, LOG_TAG, __VA_ARGS__)


static struct pollfd *ufds;
static char **device_names;
static int nfds;

//touch fds
static struct pollfd *utfds;
static int ntfds;

static int pipefd0 = -1;
static int pipefd1 = -1;

const char *touch_device = "/dev/input/event1";

static int open_device(const char *device){
    int version;
    int fd;
    int clkid = CLOCK_MONOTONIC;
    struct pollfd *new_ufds;
    char **new_device_names;
    char name[80];
    char location[80];
    char idstr[80];
    struct input_id id;

    fd = open(device, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }

    if(ioctl(fd, EVIOCGVERSION, &version)){
        fprintf(stderr, "could not get driver version for %s, %s \n", device, strerror(errno));
        return -1;
    }

    if(ioctl(fd, EVIOCGID, &id)) {
        fprintf(stderr, "could not get driver id for %s, %s\n", device, strerror(errno));
        return -1;
    }
    name[sizeof(name) - 1] = '\0';
    location[sizeof(location) - 1] = '\0';
    idstr[sizeof(idstr) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        name[0] = '\0';
    }
    if(ioctl(fd, EVIOCGPHYS(sizeof(location) - 1), &location) < 1) {
        location[0] = '\0';
    }
    if(ioctl(fd, EVIOCGUNIQ(sizeof(idstr) - 1), &idstr) < 1) {
        idstr[0] = '\0';
    }

    if (ioctl(fd, EVIOCSCLOCKID, &clkid) != 0) {
        fprintf(stderr, "Can't enable monotonic clock reporting: %s\n", strerror(errno));
    }

    new_ufds = (pollfd *)realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL) {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = (char **)realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL) {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    device_names = new_device_names;

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(device);
    nfds++;
    return 0;

}

static int scan_dir(const char *dirname) {
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        open_device(devname);
    }
    closedir(dir);
    return 0;
}


static int init_touch_fds(){
    int version;
    int fd;
    int clkid = CLOCK_MONOTONIC;
    char name[80];
    char location[80];
    char idstr[80];
    struct input_id id;

    fd = open(touch_device, O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "could not open %s, %s\n", touch_device, strerror(errno));
        return -1;
    }

    if(ioctl(fd, EVIOCGVERSION, &version)){
        fprintf(stderr, "could not get driver version for %s, %s \n", touch_device, strerror(errno));
        return -1;
    }

    if(ioctl(fd, EVIOCGID, &id)) {
        fprintf(stderr, "could not get driver id for %s, %s\n", touch_device, strerror(errno));
        return -1;
    }
    name[sizeof(name) - 1] = '\0';
    location[sizeof(location) - 1] = '\0';
    idstr[sizeof(idstr) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
        name[0] = '\0';
    }
    if(ioctl(fd, EVIOCGPHYS(sizeof(location) - 1), &location) < 1) {
        location[0] = '\0';
    }
    if(ioctl(fd, EVIOCGUNIQ(sizeof(idstr) - 1), &idstr) < 1) {
        idstr[0] = '\0';
    }

    if (ioctl(fd, EVIOCSCLOCKID, &clkid) != 0) {
        fprintf(stderr, "Can't enable monotonic clock reporting: %s\n", strerror(errno));
    }

    int pipefd[2];
    if(pipe(pipefd) != 0) {
        fprintf(stderr, "could not create pipe %sn", strerror(errno));
        return -1;
    }
    pipefd0 = pipefd[0];
    pipefd1 = pipefd[1];

    ntfds = 2;
    utfds = (pollfd *)calloc(2, sizeof(ufds[0]));
    utfds[0].fd = pipefd1;
    utfds[0].events = POLLIN;
    utfds[1].fd = fd;
    utfds[1].events = POLLIN;

    return 0;
}


static int release(){
    return 0;
}

static int stop(){
    if(pipefd0 > 0){
        write(pipefd0, "0", 1);
    }
}


static int loop_touch_event(){
    int i;
    struct input_event event;
    int res;
    LOGD("start to loop touch event");
    while(true){
        poll(utfds, ntfds, -1);
        if(utfds[0].revents & POLLIN) {
            LOGD("exit poll");
            release();
            return 0;
        }

        for(i = 1; i < ntfds; i++ ){
            if(utfds[i].revents && (utfds[i].revents & POLLIN )) {
                res = (int)read(utfds[i].fd, &event, sizeof(event));
                if(res < (int)sizeof(event)) {
                    fprintf(stderr, "could not get event\n");
                    return 1;
                }
                LOGD("event type: %04x, code: %04x, value: %08x", event.type, event.code, event.value);
            }
        }
    }

    return 0;
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_tapmacro_eventinjector_EventInjector_nativeTest(
        JNIEnv *env,
        jobject /* this */) {
//    std::string hello = "Hello from C++";
//    return env->NewStringUTF(hello.c_str());

    LOGD("test start");
//    const char *device_path = "/dev/input";
//    scan_dir(device_path);

    init_touch_fds();

    loop_touch_event();


    LOGD("test end");
}