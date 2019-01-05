#include <string.h>
#include <stdint.h>
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>

#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>

#include <android/log.h>
#include "EventInjector.h"

#define LOG_TAG  "native-lib"
#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGF(...)  __android_log_print(ANDROID_LOG_FATAL, LOG_TAG, __VA_ARGS__)
#define LOGS(...)  __android_log_print(ANDROID_LOG_SILENT, LOG_TAG, __VA_ARGS__)


#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))


static struct typedev {
	struct pollfd ufds;
	char *device_path;
	char *device_name;

    bool touch;
} *pDevs = NULL;



struct pollfd *ufds;
static int nDevsCount;

const char *device_path = "/dev/input";


struct input_event event;
int c;
int i;
char *newline = "\n";
int version;

int dont_block = -1;
int sync_rate = 0;
int64_t last_sync_time = 0;
const char *device = NULL; 

static int open_device(int index)
{
	if (index >= nDevsCount || pDevs == NULL) return -1;
	LOGD("open_device prep to open");
	char *device = pDevs[index].device_path;

	LOGD("open_device call %s", device);
    int version;
    int fd;
    
    char name[80];
    char location[80];
    char idstr[80];
    struct input_id id;
	
    fd = open(device, O_RDWR);
    if(fd < 0) {
		pDevs[index].ufds.fd = -1;
		
		pDevs[index].device_name = NULL;
		LOGD("could not open %s, %s", device, strerror(errno));
        return -1;
    }
    
	pDevs[index].ufds.fd = fd;
	ufds[index].fd = fd;
	
    name[sizeof(name) - 1] = '\0';
    if(ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
		LOGD("could not get device name for %s, %s", device, strerror(errno));
        name[0] = '\0';
    }
	LOGD("Device %d: %s: %s", nDevsCount, device, name);
	
	pDevs[index].device_name = strdup(name);

    uint8_t keyBitmask[(KEY_MAX + 1) / 8];
    uint8_t absBitmask[(ABS_MAX + 1) / 8];
    // Figure out the kinds of events the device reports.
    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask);
    ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(absBitmask)), absBitmask);

    if (test_bit(ABS_MT_POSITION_X, absBitmask)
        && test_bit(ABS_MT_POSITION_Y, absBitmask)) {
        LOGD("this is a touch device");
        pDevs[index].touch = true;
    }

    return 0;
}

int remove_device(int index)
{
	if (index >= nDevsCount || pDevs == NULL ) return -1;
	
	int count = nDevsCount - index - 1;
	LOGD("remove device %d", index);
	free(pDevs[index].device_path);
	free(pDevs[index].device_name);
	
	memmove(&pDevs[index], &pDevs[index+1], sizeof(pDevs[0]) * count);
	nDevsCount--;
	return 0;
} 



static int scan_dir(const char *dirname)
{
	nDevsCount = 0;
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
		struct typedev *new_pDevs = (typedev *)realloc(pDevs, sizeof(pDevs[0]) * (nDevsCount + 1));
		if(new_pDevs == NULL) {
			LOGD("out of memory");
			return -1;
		}
		pDevs = new_pDevs;
		
		struct pollfd *new_ufds = (pollfd *)realloc(ufds, sizeof(ufds[0]) * (nDevsCount + 1));
		if(new_ufds == NULL) {
			LOGD("out of memory");
			return -1;
		}
		ufds = new_ufds; 
		ufds[nDevsCount].events = POLLIN;
		
		pDevs[nDevsCount].ufds.events = POLLIN;
		pDevs[nDevsCount].device_path = strdup(devname);
		
        nDevsCount++;
    }
    closedir(dir);
    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_intSendEvent(JNIEnv* env,jobject thiz, jint index, uint16_t type, uint16_t code, int32_t value) {
	if (index >= nDevsCount || pDevs[index].ufds.fd == -1) return -1;
	int fd = pDevs[index].ufds.fd;
	LOGD("SendEvent call (%d,%d,%d,%d)", fd, type, code, value);
	struct uinput_event event;
	int len;

	if (fd <= fileno(stderr)) return -1;

	memset(&event, 0, sizeof(event));
	event.type = type;
	event.code = code;
	event.value = value;

	len = write(fd, &event, sizeof(event));
	LOGD("SendEvent done:%d",len);

	return 0;
}


extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_ScanFiles( JNIEnv* env, jobject thiz ) {
	int res = scan_dir(device_path);
	if(res < 0) {
		LOGD("scan dir failed for %s:", device_path);
		return -1;
	}
	
	return nDevsCount;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_tapmacro_eventinjector_EventInjector_getDevPath( JNIEnv* env, jobject thiz, jint index) {
	return env->NewStringUTF(pDevs[index].device_path);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_tapmacro_eventinjector_EventInjector_getDevName( JNIEnv* env, jobject thiz, jint index) {
	if (pDevs[index].device_name == NULL) return NULL;
	else return env->NewStringUTF(pDevs[index].device_name);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_OpenDev( JNIEnv* env,jobject thiz, jint index ) {
	return open_device(index);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_RemoveDev( JNIEnv* env,jobject thiz, jint index ) {
	return remove_device(index);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_PollDev( JNIEnv* env,jobject thiz, jint index ) {
	if (index >= nDevsCount || pDevs[index].ufds.fd == -1) return -1;
	poll(ufds, nDevsCount, -1);
	if(ufds[index].revents) {
		if(ufds[index].revents & POLLIN) {
			int res = read(ufds[index].fd, &event, sizeof(event));
			if(res < (int)sizeof(event)) {
				return 1;
			} else {
                return 0;
            }
		}
	}
	return -1;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_getType( JNIEnv* env,jobject thiz ) {
	return event.type;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_getCode( JNIEnv* env,jobject thiz ) {
	return event.code;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_tapmacro_eventinjector_EventInjector_getValue( JNIEnv* env,jobject thiz ) {
	return event.value;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_tapmacro_eventinjector_EventInjector_isTouchDev(JNIEnv* env,jobject thiz, jint index ) {
    if(index >= nDevsCount || index < 0) return JNI_FALSE;
    if(pDevs[index].touch) {
        return JNI_TRUE;
    }

    return JNI_FALSE;
}