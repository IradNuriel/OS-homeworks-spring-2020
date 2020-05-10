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
MODULE_AUTHOR("IRAD NURIEL");

int 	encdec_open(struct inode *inode, struct file *filp);
int 	encdec_release(struct inode *inode, struct file *filp);
int 	encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

ssize_t encdec_read_caesar( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

ssize_t encdec_read_xor( struct file *filp, char *buf, size_t count, loff_t *f_pos );
ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

//helper functions
void zero_buffer(char* buf, int size);
void caesar_decrypt(char* buf, unsigned char key, int count);
void caesar_encrypt(char* buf, unsigned char key, int count);
void xor_crypt(char* buf, unsigned char key, int count);


int memory_size = 0;
char* caesar_memory_buffer;
char* xor_memory_buffer;



//uxtract module parameter memory_size
MODULE_PARM(memory_size, "i");

int major = 0;

//file operation for caesar cipher
struct file_operations fops_caesar = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_caesar,
	.write 	 =	encdec_write_caesar,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};

//file operation for xor cipher
struct file_operations fops_xor = {
	.open 	 =	encdec_open,
	.release =	encdec_release,
	.read 	 =	encdec_read_xor,
	.write 	 =	encdec_write_xor,
	.llseek  =	NULL,
	.ioctl 	 =	encdec_ioctl,
	.owner 	 =	THIS_MODULE
};


//function zero_buffer, take buffer and its size and fill it with zeros.
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

void caesar_decrypt(char* buf, unsigned char key,int count) {//function caesar_decrypt, take buffer, encryption key, and number of characters to decrypt and decrypt it using caesar cipher.
	int i;
	for (i = 0; i < count; i++) {
		buf[i] = ((buf[i] - key) + 128) % 128;
	}
}

void caesar_encrypt(char* buf, unsigned char key,int count) {//function caesar_encrypt, take buffer, encryption key, and number of characters to encrypt and encrypt it using caesar cipher.
	int i;
	for (i = 0; i < count; i++)
	{
		buf[i] = (buf[i] + key) % 128;
	}
}

void xor_crypt(char* buf, unsigned char key,int count) {//function xor_crypt, take buffer, encryption key, and number of characters to crypt and crypt it using xor cipher.(since xor cipher works in the exact same way for encryption and decryption, it is the same function for both)
	int i;
	for (i = 0; i < count; i++) {
		buf[i] = buf[i] ^ key;
	}
}

//struct encdec_private_data, holds the read state and the encryption key 
typedef struct {
	unsigned char key;
	int read_state;
} encdec_private_data;

int init_module(void) {//function init_module, register the device and allocate space for the data buffers
	major = register_chrdev(major, MODULE_NAME, &fops_caesar);//register device
	if(major < 0) {//if error
		printk("could not register device");
		return major;
	}
	caesar_memory_buffer = kmalloc(memory_size, GFP_KERNEL);//allocate space for the caesar data buffer
	if (!caesar_memory_buffer) {//if could not allocate memory, error
		printk("could not allocate memory");
		return -ENOMEM;
	}
	zero_buffer(caesar_memory_buffer,memory_size);//fill the buffer with 0.

	xor_memory_buffer = kmalloc(memory_size, GFP_KERNEL);//allocate space for the caesar data buffer
	if (!xor_memory_buffer) {//if could not allocate memory, error
		printk("could not allocate memory");
		return -ENOMEM;
	}
	zero_buffer(xor_memory_buffer, memory_size);//fill the buffer with 0.
	return 0;
}

void cleanup_module(void) {//function cleanup_module, unregister the device and free all alocated memory

	unregister_chrdev(major, "memory");
	if (caesar_memory_buffer) {
		kfree(caesar_memory_buffer);
	}
	if (xor_memory_buffer) {
		kfree(xor_memory_buffer);
	}
}

int encdec_open(struct inode *inode, struct file *filp) {//function encdec_open,  set the filp->fops to the correct file-operations structure(using the minor value to determine which) and allocate space for flip->private_data 
	int minor = MINOR(inode->i_rdev);


	filp->f_op = (minor == 0) ? &fops_caesar : &fops_xor;// set filp->fops
	filp->private_data = kmalloc(sizeof(encdec_private_data), GFP_KERNEL);//allocate space for flip->private_data
	if (!filp->private_data) {//if couldn't allocate, error
		return -1;
	}
	//set default values to private_data->key and private_data->read_state as 0.
	((encdec_private_data*)(filp->private_data))->key = 0;
	((encdec_private_data*)(filp->private_data))->read_state = 0;

	return 0;
}

int encdec_release(struct inode *inode, struct file *filp) {//functio encdec_release, free allocated private data 

	if (filp->private_data) {
		kfree(filp->private_data);
	}
	return 0;
}

int encdec_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {//function ioctl
	if (filp->private_data) {//if the file is opened
		switch (cmd) {
		case ENCDEC_CMD_CHANGE_KEY://if we need to change encryption key
			((encdec_private_data*)(filp->private_data))->key = arg;//set new key
			break;
		case ENCDEC_CMD_SET_READ_STATE://if we need to change read state
			((encdec_private_data*)(filp->private_data))->read_state = arg;//set the read state
			break;
		case ENCDEC_CMD_ZERO://if we need to zero the data buffer
			if (MINOR(inode->i_rdev) == 0) {//if we in caesar cipher
				zero_buffer(caesar_memory_buffer,memory_size);
			}
			else {//if we in xor cipher
				zero_buffer(xor_memory_buffer,memory_size);
			}
			break;
		default:
			printk("ioctl expect to get ENCDEC_CMD_CHANGE_KEY, ENDEC_CMD_SET_READ_STATE or ENDEC-CMD_ZERO as the cmd parameter");
			break;
		}
	}
	return 0;
}

ssize_t encdec_read_caesar(struct file *filp, char *buf, size_t count, loff_t *f_pos) {//functon encdec_read_caesar
	if (*f_pos >= memory_size) {//if we have no space to read from, error
		return -EINVAL;
	}
	if (filp->private_data) {//if device is opened
		while (((*f_pos) + count) > memory_size && count>0) {//set count to amount we can read.
			count--;
		}
		if (((*f_pos) + count) > memory_size) {//if we still can't read, error
			return -EINVAL;
		}
		if (((encdec_private_data*)(filp->private_data))->read_state == ENCDEC_READ_STATE_DECRYPT) {//if we wants to read decrypted data
			caesar_decrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//decrypt the part we want to read from
			copy_to_user(buf, caesar_memory_buffer + (*f_pos), count);//read it
			caesar_encrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//reencrypt the part we read from. 
		}
		else {//if we want to read raw data
			copy_to_user(buf, caesar_memory_buffer + (*f_pos), count);// just read it	
		}
		(*f_pos) += count;//increase (*f_pos) by the amount we read
		return count;//return the number of characters we read 
	}
	else {//if device isn't opened, error
		return -EINVAL;
	}
}


ssize_t encdec_read_xor(struct file *filp, char *buf, size_t count, loff_t *f_pos) {//functon encdec_read_xor
	if (*f_pos >= memory_size) {//if we have no space to read from, error
		return -EINVAL;
	}
	if (filp->private_data) {//if device is opened
		while (((*f_pos) + count) > memory_size && count > 0) {//set count to amount we can read.
			count--;
		}
		if (((*f_pos) + count) > memory_size) {//if we still can't read, error
			return -EINVAL;
		}
		if (((encdec_private_data*)(filp->private_data))->read_state == ENCDEC_READ_STATE_DECRYPT) {//if we wants to read decrypted data
			xor_decrypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//decrypt the part we want to read from
			copy_to_user(buf, xor_memory_buffer + (*f_pos), count);//read it
			xor_encrypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//reencrypt the part we read from. 
		}
		else {//if we want to read raw data
			copy_to_user(buf, xor_memory_buffer + (*f_pos), count);// just read it	
		}
		(*f_pos) += count;//increase (*f_pos) by the amount we read
		return count;//return the number of characters we read 
	}
	else {//if device isn't opened, error
		return -EINVAL;
	}
}

ssize_t encdec_write_caesar(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {//function encdec_write_caesar
	if (*f_pos >= memory_size) {//if no memory to write to, error
		return -ENOSPC;
	}
	if (filp->private_data) {//if the device is opened
		while (((*f_pos) + count )> memory_size && count>0) {//set count to amount we can read.
			count--;
		}
		if (((*f_pos) + count) > memory_size) {//if we still can't read, error
			return -ENOSPC;
		}
		copy_from_user(caesar_memory_buffer + (*f_pos), buf, count);//write to the device
		caesar_encrypt(caesar_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//encrypt what we just wrote
		(*f_pos) += count;//increase (*f_pos) by the amount we wrote 
		return count;//return number of characters that we have written
	}
	else {//if the device isn't opened,error
		return -ENOSPC;
	}
}

ssize_t encdec_write_xor(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {//function encdec_write_caesar
	if (*f_pos >= memory_size) {//if no memory to write to, error
		return -ENOSPC;
	}
	if (filp->private_data) {//if the device is opened
		while (((*f_pos) + count) > memory_size && count > 0) {//set count to amount we can read.
			count--;
		}
		if (((*f_pos) + count) > memory_size) {//if we still can't read, error
			return -ENOSPC;
		}
		copy_from_user(cor_memory_buffer + (*f_pos), buf, count);//write to the device
		xor_encrypt(xor_memory_buffer + (*f_pos), ((encdec_private_data*)(filp->private_data))->key, count);//encrypt what we just wrote
		(*f_pos) += count;//increase (*f_pos) by the amount we wrote 
		return count;//return number of characters that we have written
	}
	else {//if the device isn't opened,error
		return -ENOSPC;
	}
}



// Add implementations for:
// ------------------------
