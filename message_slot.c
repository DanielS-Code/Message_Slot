//
// Created by daniel on 12/8/17.
//


// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h>
#include "message_slot.h"

#define NO_CHANNEL 0
MODULE_LICENSE("GPL");

static slotList* slots = NULL;


//================== LINKED LIST FUNCTIONS ===========================

void printLinkedList(char* str)
{
	if (slots!=NULL)
	{	
		
		slotList* slot = slots;
		channelList* chPointer;
		printk(KERN_EMERG "call from func: %s", str);
		while (slot!=NULL)
		{
			printk(KERN_EMERG "we are at slot: %d the current channel is: %d\nThe channels are:\n", slot->minor,slot->currentChannel);
			chPointer = slot->chLst;
			while (chPointer!=NULL)
			{
				printk(KERN_EMERG "channel: %d content: %s\n",chPointer->channelNum, chPointer->message);
				chPointer = chPointer->next;
			}
			slot = slot->next;
		}
	}
	
}

/**
 * checks if slot exists in list
 * @param inode
 * @return slot pointer if yes, or returns NULL otherwise
 */
static slotList* getSlot (struct inode* inode)
{
	slotList* slotPointer = slots;	
    while (slotPointer!=NULL && (slotPointer->minor!=iminor(inode)))
    {
        slotPointer = slotPointer->next;
    }
	return slotPointer;
}

/**
 * adds new slot to slot list.
 * @param inode
 * @return 0 if slot was added, -1 for error;
 */
static int addSlotToList (struct inode* inode)
{
	
    slotList* newSlot = getSlot(inode);
	if (newSlot==NULL)
    {
        newSlot = (slotList*)kmalloc(sizeof(slotList),GFP_KERNEL);
        if (newSlot == NULL)
        {
            printk(KERN_INFO "ERROR: fail to create slot with kmalloc");
            return -ENOMEM;
        }
        newSlot->currentChannel = NO_CHANNEL;
        newSlot->minor = iminor(inode);
        newSlot->chLst = NULL;
        newSlot->next = slots;
        slots = newSlot;
    
    }
    return 0;

}


/**
 *
 * @param slot
 * @param chNum
 * @return 1 if chNum is already in slot, 0 otherwise
 *
 */

static channelList* getChannel(slotList* slot , int chNum)
{

    channelList* channelPointer = slot->chLst;
    while (channelPointer!=NULL && (channelPointer->channelNum!=chNum))
    {
        channelPointer = channelPointer->next;
    }
    return channelPointer;
}

/**
 * function is called from ioctl
 * @param inode
 * @param channelNum
 * @return if slot exists: checks is channel exist - if yes, returns 0 otherwise creates new channel in slot
 * also changes current channel of slot
 */
static int addChannelToSlot (struct inode* inode, int channelNum)
{
    slotList* slot = getSlot(inode);
	channelList* channel;
    if (slot!=NULL)
    {
        channel = getChannel(slot,channelNum);
        if (channel==NULL) //checks if channel does not exist - creates new channel
        {
            channel = (channelList*)kmalloc(sizeof(channelList),GFP_KERNEL);
            if (channel == NULL)
            {
                printk(KERN_INFO "ERROR: fail channel create with kmalloc");
                return -ENOMEM;
            }
            channel->channelNum = channelNum;
            channel->msgSize = 0;
            channel->next = slot->chLst;
            slot->chLst = channel;
        }
        slot->currentChannel = channelNum; //change current channel

        return 0;

    }
    else
    {
        printk(KERN_INFO "message slot does not exist");
        return -EINVAL;
    }
}

/**
 * @param slot - that exists in slot list
 * function removes all channels from specific slot and frees memory
 */
static void removeSlotChannels (slotList* slot)
{
    channelList* channelPointer = slot->chLst;
    while (channelPointer!=NULL)
    {
        slot->chLst = channelPointer->next;
        kfree(channelPointer);
        channelPointer = slot->chLst;

    }
}


/**
 * removes a single slot from slotList
 * @param slot
 * @return 0 on success, -1 otherwise
 static int removeSlot (struct inode* inode)
{
    slotList* slot = getSlot(inode);
    if (slot!=NULL) //check if slot exist in slot list
    {
        if (slots ==slot)
        {
            slots = slot->next;
        }
        else
        {
            slotList* slotPointer = slots;
            while (slotPointer->next!=slot) //find the node that is before the one we want to delete
            {
                slotPointer = slotPointer->next;
            }
            slotPointer->next = slot->next;
        }
        removeSlotChannels(slot);
        kfree(slot);
        return 0;
    }
    else
    {
        printk("message slot does not exist - can't be removed");
        errno = EINVAL;
        return -1;
    }

}

 */

static void removeAllSlots(void)
{
    slotList* slotPointer = slots;
    while (slotPointer!=NULL)
    {
        slots = slots->next;
        removeSlotChannels(slotPointer);
        kfree(slotPointer);
        slotPointer =slots;
    }

}


//================== DEVICE FUNCTIONS ===========================

static int device_open( struct inode* inode, struct file*  file )
{

    int retVal = addSlotToList(inode);
    return  retVal;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode, struct file*  file)
{
    return 0;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file, char __user* buffer,size_t length, loff_t* offset)
{
    slotList* slot = getSlot(file->f_path.dentry->d_inode);
    int i;
    if (slot==NULL) //check if slot exists
    {
        printk(KERN_INFO "ERROR: file not found");
        return -EINVAL;
    }
    else
    {
        if (slot->currentChannel == NO_CHANNEL) //check if there is a current channel to read from
        {
            printk(KERN_INFO "ERROR: no channel was chosen");
            return -EINVAL;
        }
        else
        {
			
            channelList* channel = getChannel(slot,slot->currentChannel);		
            if (length<channel->msgSize)
            {
                printk(KERN_INFO "ERROR: can't read message - length is to small");
                return -ENOSPC;
            }
            if (channel->msgSize == 0)
            {
                printk(KERN_INFO "ERROR: no message on channel %d",channel->channelNum);
                return -EWOULDBLOCK;
            }
            for (i=0; i<channel->msgSize; i++)
            {
                if (put_user(channel->message[i],buffer+i)!=0) //checks putuser was correct
                {
                    printk(KERN_INFO "ERROR: unable to putuser");
                    return -EINVAL;
                }

            }
            return channel->msgSize; //returns number of characters written to buffer
        }
    }
}



//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset)
{
	
    int i;
    slotList* slot;
    if (length > MAX_MESSAGE_LEN)
    {
        printk(KERN_INFO "ERROR: can't write message - length is to big");
        return -EINVAL;
    }
    slot = getSlot(file->f_path.dentry->d_inode);
    if (slot==NULL) //check if slot exists
    {
        printk(KERN_INFO "ERROR: file not found");
        return -EINVAL;
    }
    else
    {
        if (slot->currentChannel == NO_CHANNEL) //check if there is a current channel to read from
        {
            printk(KERN_INFO "ERROR: no channel was chosen");
            return -EINVAL;
        }
        else
        {
			
            channelList* channel = getChannel(slot,slot->currentChannel);
            for (i=0; i<length; i++)
            {
                if (get_user(channel->message[i],buffer+i)!=0) //checks putuser was correct
                {
                    printk(KERN_INFO "ERROR: unable to putuser");
                    return -EINVAL;
                }

            }
            channel->msgSize = length;
            return length; //returns number of characters written to buffer
        }
    }

}


//----------------------------------------------------------------
static long device_ioctl( struct   file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param )
{
    int retVal;
    if( IOCTL_SET_ENC == ioctl_command_id )
    {
        retVal = addChannelToSlot(file->f_path.dentry->d_inode,(int)ioctl_param);
        return retVal;
    }
    else
    {
        return -EINVAL;
    }
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .release        = device_release,
        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init initial_driver(void)
{
    int rc = -1;
    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );
    // Negative values signify an error
    if( rc < 0 )
    {
        printk(KERN_ALERT "registraion failed for device with major  %d\n", MAJOR_NUM);
        return rc;
    }
    printk(KERN_INFO "message_slot: registered major number %d\n",MAJOR_NUM);
    return 0;
}

//---------------------------------------------------------------
static void __exit cleanup_driver(void)
{
    removeAllSlots();
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(initial_driver);
module_exit(cleanup_driver);

//========================= END OF FILE =========================
