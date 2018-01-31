//
// Created by daniel on 12/8/17.
//

#ifndef OS_HW3_MESSAGE_SLOT_H
#define OS_HW3_MESSAGE_SLOT_H


// The major device number.
// We don't rely on dynamic registration
// any more. We want ioctls to know this
// number at compile time.
#define MAJOR_NUM 255
#define MAX_MESSAGE_LEN 128
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"


#include <linux/ioctl.h>

typedef struct channelList
{
    int channelNum;
    int msgSize;
    char message[MAX_MESSAGE_LEN];
    struct channelList* next;
} channelList ;

typedef struct slotList
{
    int currentChannel;
    unsigned int minor;
    struct slotList* next;
    struct channelList* chLst;
} slotList;




#endif //OS_HW3_MESSAGE_SLOT_H
