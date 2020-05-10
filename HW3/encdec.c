#include <linux/ctype.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>  	
#include <linux/slab.h>
#include <linux/fs.h>       		
#include <linux/errno.h>  
#include <linux/types.h> 
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/string.h>

#include "encdec.h"

#define MODULE_NAME "encdec"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YOUR NAME");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void zero_buffer(char* buf, int size);
void caesar_decrypt(char* buf, unsigned char key, int count);
void caesar_encrypt(char* buf, unsigned char key, int count);
void xor_crypt(char* buf, unsigned char key, int count);


int memory_size = 0;
char* caesar_memory_buffer;
char* xor_memory_buffer;




MODULE_PARM(memory_size, "i");

int major = 0;

struct file_operations fops_caesar = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

void zero_buffer(char* buf, int size) {
	if (buf) {
		int len = strlen(buf);
		int i;
		int min = (len < size) ? len : size;
		for (i = 0; i < min; i++) {
			buf[i] = 0;
		}
		
	}
}

void caesar_decrypt(char* buf, unsigned char key,int count) {
	int i;
	for (i = 0; i < count; i++) {
		buf[i] = ((buf[i] - key) + 128) % 128;
	}
}

void caesar_encrypt(char* buf, unsigned char key,int count) {
	int i;
	for (i = 0; i < count; i++)
	{
		buf[i] = (buf[i] + key) % 128;
	}
}

void xor_crypt(char* buf, unsigned char key,int count) {
	int i;
	for (i = 0; i < count; i++) {
		buf[i] = buf[i] ^ key;
	}
}



// Implemetation suggestion:
// -------------------------
// Use this structure as your file-object's private data structure
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data;

int init_module(void) {
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);
	if(major < 0) {
		printk("could not register device");
		return major;
	}
	caesar_memory_buffer = kmalloc(memory_size, GFP_KERNEL);
	if (!caesar_memory_buffer) {
		printk("could not allocate memory");
		return -ENOMEM;
	}
	zero_buffer(caesar_memory_buffer,memory_size);

	xor_memory_buffer = kmalloc(memory_size, GFP_KERNEL);
	if (!xor_memory_buffer) {
		printk("could not allocate memory");
		return -ENOMEM;
	}
	zero_buffer(xor_memory_buffer, memory_size);
	// Implemetation suggestion:
	// -------------------------
	// 1. Allocate memory for the two device buffers using kmalloc (each of them should be of size 'memory_size')

	return 0;
}

void cleanup_module(void) {

	unregister_chrdev(major, "memory");
	if (caesar_memory_buffer) {
		kfree(caesar_memory_buffer);
	}
	if (xor_memory_buffer) {
		kfree(xor_memory_buffer);
	}

	// Implemetation suggestion:
	// -------------------------	
	// 1. Unregister the device-driver
	// 2. Free the allocated device buffers using kfree
}

int encdec_open(struct inode *inode, struct file *filp) {
	int minor = MINOR(inode->i_rdev);


	filp->f_op = (minor == 0) ? &fops_caesar : &fops_xor;
	filp->private_data = kmalloc(sizeof(encdec_private_data), GFP_KERNEL);
	if (!filp->private_data) {
		return -1;
	}
	((encdec_private_data*)(filp->private_data))->key = 0;
	((encdec_private_data*)(filp->private_data))->read_state = 0;


	// Implemetation suggestion:
	// -------------------------
	// 1. Set 'filp->f_op' to the correct file-operations structure (use the minor value to determine which)
	// 2. Allocate memory for 'filp->private_data' as needed (using kmalloc)

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp) {

	if (filp->private_data) {
		kfree(filp->private_data);
	}

	// Implemetation suggestion:
	// -------------------------
	// 1. Free the allocated memory for 'filp->private_data' (using kfree)

	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	if (filp->private_data) {
		switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY:
			((encdec_private_data*)(filp->private_data))->key = arg;
			break;
		case ENCDEC_CMD_SET_READ_STATE:
			((encdec_private_data*)(filp->private_data))->read_state = arg;
			break;
		case ENCDEC_CMD_ZERO:
			if (MINOR(inode->i_rdev) == 0) {
				zero_buffer(caesar_memory_buffer,memory_size);
			}
			else {
				zero_buffer(xor_memory_buffer,memory_size);
			}
			break;
		default:
			printk("ioctl expect to get ENCDEC_CMD_CHANGE_KEY, ENDEC_CMD_SET_READ_STATE or ENDEC-CMD_ZERO as the cmd parameter");
			break;
		}
	}
	
	// Implemetation suggestion:
	// -------------------------
	// 1. Update the relevant fields in 'filp->private_data' according to the values of 'cmd' and 'arg'

	return 0;
}

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos >= memory_size) {
		return -EINVAL;
	}
	if (filp->private_data) {
		while (((*f_pos) + count) > memory_size && count>0) {
			count--;
		}
		if (((*f_pos) + count) > memory_size) {
			return -EINVAL;
		}
		/*
		copy_to_user(buf, caesar_memory_buffer + (*f_pos) -1, count);
		if (((encdec_private_data*)(filp->private_data))->read_state == ENCDEC_READ_STATE_DECRYPT) {
			caesar_decrypt(buf, ((encdec_private_data*)(filp->private_data))->key,count);
		}
		*/
		if (((encdec_private_data*)(filp->private_data))->read_state == ENCDEC_READ_STATE_DECRYPT) {
			caesar_decrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);
			copy_to_user(buf, caesar_memory_buffer + (*f_pos), count);
			caesar_encrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);
		}
		else {
			copy_to_user(buf, caesar_memory_buffer + (*f_pos), count);	
		}
		(*f_pos) += count;
		return count;
	}
	else {
		return -EINVAL;
	}
}


ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos >= memory_size) {
		return -EINVAL;
	}
	if (filp->private_data) {
		while (((*f_pos) + count) > memory_size && count>0) {
			count--;
		}
		if (((*f_pos) + count) > memory_size) {
			return -EINVAL;
		}
		if (((encdec_private_data*)(filp->private_data))->read_state == ENCDEC_READ_STATE_DECRYPT) {
			xor_crypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);
			copy_to_user(buf, xor_memory_buffer + (*f_pos), count);
			xor_crypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);

		}
		else {
			copy_to_user(buf, xor_memory_buffer + (*f_pos), count);
		}
		(*f_pos) += count;
		return count;
	}
	else {
		return -EINVAL;
	}
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos >= memory_size) {
		return -ENOSPC;
	}
	if (filp->private_data) {
		while (((*f_pos) + count )> memory_size && count>0) {
			count--;
		}
		if (((*f_pos) + count) > memory_size) {
			return -ENOSPC;
		}
		copy_from_user(caesar_memory_buffer + (*f_pos), buf, count);
		caesar_encrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);
		(*f_pos) += count;
		return count;
	}
	else {
		return -ENOSPC;
	}
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	if (*f_pos >= memory_size) {
		return -ENOSPC;
	}
	if (filp->private_data) {
		while (((*f_pos) + count) > memory_size && count > 0) {
			count--;
		}
		if (((*f_pos) + count) > memory_size) {
			return -ENOSPC;
		}
		copy_from_user(xor_memory_buffer + (*f_pos), buf, count);
		xor_crypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);
		(*f_pos) += count;
		return count;
	}
	else {
		return -ENOSPC;
	}
}



// Add implementations for:
// ------------------------
