/* #include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_ENTRY "chenille"
#define SUB_DIR "ensea"

static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *sub_entry;


static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *offset) {

    return 0;
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset) {
  

    return count;
}

static const struct file_operations proc_fops = {
    .read = proc_read,
    .write = proc_write,
};

static int __init my_module_init(void) {
    // Création d’un fichier 
  
    sub_entry = proc_mkdir(SUB_DIR, NULL );
    if (!sub_entry) {
        printk(KERN_ERR "Failed to create subdirectory\n");
        remove_proc_entry(SUB_DIR, NULL);
        return -ENOMEM;
    }
	proc_entry = proc_create(PROC_ENTRY, 0666, sub_entry, &proc_fops);
	  
    return 0;
}

static void __exit my_module_exit(void) {
 
 remove_proc_entry(PROC_ENTRY, sub_entry);
 remove_proc_entry(SUB_DIR, NULL);
 
 printk(KERN_ALERT "Bye bye...\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");




 */


#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_ENTRY "chenille"
#define SUB_DIR "ensea"

static struct proc_dir_entry *proc_entry;
static struct proc_dir_entry *sub_entry;

// 新增一个全局变量以保存闪烁速度，默认为0
static int blink_speed = 0;

static ssize_t proc_read(struct file *file, char __user *buffer, size_t count, loff_t *offset) {
    // 在这里实现从文件中读取图案的逻辑，将相应的数据复制到用户空间
    // 可以使用copy_to_user函数
   
    return 0;
}

static ssize_t proc_write(struct file *file, const char __user *buffer, size_t count, loff_t *offset) {
    char input[10]; 
	char speed[] = "speed=";
	char pattern1[]= "pattern=1";
	
    if (count > sizeof(input) - 1) {
        printk(KERN_ERR "Input is too long\n");
        return -EINVAL;
    }

    if (copy_from_user(input, buffer, count)) {
        return -EFAULT;
    }

    input[count] = '\0'; 

    if (strncmp(input, speed, strlen(speed)) == 0) {
        if (sscanf(input + strlen(speed), "%d", &blink_speed) == 1) {
            printk(KERN_INFO "Input is valid. Speed value: %d\n", blink_speed);
        } else {
            printk(KERN_INFO "Invalid speed value\n");
        }
		
    } else {
        printk(KERN_INFO "Input does not start with 'speed='\n");
    }
 

    return count;
}

static int proc_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Open success\n");
    return 0;
}


static const struct file_operations proc_fops = {
    .read = proc_read,
    .write = proc_write,
	.open = proc_open,
};

static int __init my_module_init(void) {
    // 在这里添加获取速度并设置的逻辑
   // printk(KERN_INFO "Setting blink speed to %d\n", blink_speed);

    sub_entry = proc_mkdir(SUB_DIR, NULL);
    if (!sub_entry) {
        printk(KERN_ERR "Failed to create subdirectory\n");
        return -ENOMEM;
    }

    proc_entry = proc_create(PROC_ENTRY, 0666, sub_entry, &proc_fops);
    printk(KERN_INFO "module_init success\n");
    return 0;
}

static void __exit my_module_exit(void) {
    remove_proc_entry(PROC_ENTRY, sub_entry);
    remove_proc_entry(SUB_DIR, NULL);

    printk(KERN_ALERT "Bye bye...\n");
}

module_init(my_module_init);
module_exit(my_module_exit);

MODULE_LICENSE("GPL");
