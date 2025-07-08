#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>



#define PROC_ENTRY "speed"
#define SUB_DIR "ensea"


static struct proc_dir_entry *proc_entry_speed;
static struct proc_dir_entry *proc_entry_dir;
static struct proc_dir_entry *sub_entry;
static struct timer_list my_timer;
static struct timer_list my_timer2;


// Prototypes
static int leds_probe(struct platform_device *pdev);
static int leds_remove(struct platform_device *pdev);
static ssize_t leds_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static ssize_t leds_write(struct file *file, const char *buffer, size_t len, loff_t *offset);
static ssize_t speed_read(struct file *file, char *buffer, size_t len, loff_t *offset);
static ssize_t dir_read(struct file *file, char *buffer, size_t len, loff_t *offset);





//static int INTERVALLE;
static int speed;
static char copy2usr[100];
static char directe[100];

module_param(speed,int,0);
MODULE_PARM_DESC(speed,"Un paramètre de ce module");




// An instance of this structure will be created for every ensea_led IP in the system
struct ensea_leds_dev {
    struct miscdevice miscdev;
    void __iomem *regs;
    u8 leds_value;
};



struct ensea_leds_dev *dev;

// Specify which device tree devices this driver supports
static struct of_device_id ensea_leds_dt_ids[] = {
    {
        .compatible = "dev,ensea"
    },
    { /* end of table */ }
};

// Inform the kernel about the devices this driver supports
MODULE_DEVICE_TABLE(of, ensea_leds_dt_ids);

// Data structure that links the probe and remove functions with our driver
static struct platform_driver leds_platform = {
    .probe = leds_probe,
    .remove = leds_remove,
    .driver = {
        .name = "Ensea LEDs Driver",
        .owner = THIS_MODULE,
        .of_match_table = ensea_leds_dt_ids
    }
};

// The file operations that can be performed on the ensea_leds character file
static const struct file_operations ensea_leds_fops = {
    .owner = THIS_MODULE,
    .read = leds_read,
    .write = leds_write,
};

static const struct file_operations ensea_speed_fops = {
    .owner = THIS_MODULE,
    .read = speed_read,
};
	
static const struct file_operations ensea_dir_fops = {
    .owner = THIS_MODULE,
    .read = dir_read,
};

static ssize_t dir_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
	int success = 0;
	char result[100] = {0};
    sprintf(result, "dir: %s\r\n", directe);
	success = copy_to_user(buffer, &result, sizeof(result));
    // If we failed to copy the value to userspace, display an error message
    if(success != 0) {
        pr_info("Failed to return current led value to userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    }
	return 0;

}

static ssize_t speed_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
	int success = 0;
	char result[100] = {0};
	//speed = INTERVALLE ;
    sprintf(result, "Speed: %d\r\n", speed);
	success = copy_to_user(buffer, &result, sizeof(result));
    // If we failed to copy the value to userspace, display an error message
    if(success != 0) {
        pr_info("Failed to return current led value to userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    }
	return 0;

}



// Called when the driver is installed
static int leds_init(void)
{
    int ret_val = 0;
    pr_info("Initializing the Ensea LEDs module\n");
    printk(KERN_INFO "Setting blink speed to %d\n", speed);
	   sub_entry = proc_mkdir(SUB_DIR, NULL);
    if (!sub_entry) {
        printk(KERN_ERR "Failed to create subdirectory\n");
        return -ENOMEM;
    }

    proc_entry_speed = proc_create(PROC_ENTRY, 0666, sub_entry, &ensea_speed_fops);
    if (proc_entry_speed == NULL) {
        pr_err("Failed to create /proc/ensea/speed entry\n");
    }

    proc_entry_dir = proc_create("dir", 0666, sub_entry, &ensea_dir_fops);
	if (proc_entry_dir == NULL) {
		   pr_err("Failed to create /proc/ensea/dir entry\n");
	   }
	
    // Register our driver with the "Platform Driver" bus
    ret_val = platform_driver_register(&leds_platform);
    if(ret_val != 0) {
        pr_err("platform_driver_register returned %d\n", ret_val);
        return ret_val;
    }

    pr_info("Ensea LEDs module successfully initialized!\n");

    return 0;
}

// Called whenever the kernel finds a new device that our driver can handle
// (In our case, this should only get called for the one instantiation of the Ensea LEDs module)
static int leds_probe(struct platform_device *pdev)
{
    int ret_val = -EBUSY;
    struct resource *r = 0;

    pr_info("leds_probe enter\n");
  
	// Get the memory resources for this LED device
    r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if(r == NULL) {
        pr_err("IORESOURCE_MEM (register space) does not exist\n");
        goto bad_exit_return;
    }

    // Create structure to hold device-specific information (like the registers)
    dev = devm_kzalloc(&pdev->dev, sizeof(struct ensea_leds_dev), GFP_KERNEL);

    // Both request and ioremap a memory region
    // This makes sure nobody else can grab this memory region
    // as well as moving it into our address space so we can actually use it
    dev->regs = devm_ioremap_resource(&pdev->dev, r);
    if(IS_ERR(dev->regs))
        goto bad_ioremap;

    // Turn the LEDs on (access the 0th register in the ensea LEDs module)
    dev->leds_value = 0xFF;
    iowrite32(dev->leds_value, dev->regs);

    // Initialize the misc device (this is used to create a character file in userspace)
    dev->miscdev.minor = MISC_DYNAMIC_MINOR;    // Dynamically choose a minor number
    dev->miscdev.name = "ensea_leds";
    dev->miscdev.fops = &ensea_leds_fops;

    ret_val = misc_register(&dev->miscdev);
    if(ret_val != 0) {
        pr_info("Couldn't register misc device :(");
        goto bad_exit_return;
    }

    // Give a pointer to the instance-specific data to the generic platform_device structure
    // so we can access this data later on (for instance, in the read and write functions)
    platform_set_drvdata(pdev, (void*)dev);

    pr_info("leds_probe exit\n");

    return 0;

bad_ioremap:
   ret_val = PTR_ERR(dev->regs);
bad_exit_return:
    pr_info("leds_probe bad exit :(\n");
    return ret_val;
}

// This function gets called whenever a read operation occurs on one of the character files
static ssize_t leds_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    int success = 0;
    char result[200];

    /*
    * Get the ensea_leds_dev structure out of the miscdevice structure.
    *
    * Remember, the Misc subsystem has a default "open" function that will set
    * "file"s private data to the appropriate miscdevice structure. We then use the
    * container_of macro to get the structure that miscdevice is stored inside of (which
    * is our ensea_leds_dev structure that has the current led value).
    *
    * For more info on how container_of works, check out:
    * http://linuxwell.com/2012/11/10/magical-container_of-macro/
    */
   // struct ensea_leds_dev *dev = container_of(file->private_data, struct ensea_leds_dev, miscdev);


    sprintf(result, " Description: %s",copy2usr);

    // Give the user the current speed
	success = copy_to_user(buffer, &result, sizeof(result));
    // If we failed to copy the value to userspace, display an error message
    if(success != 0) {
        pr_info("Failed to return current led value to userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    }


    return 0; // "0" indicates End of File, aka, it tells the user process to stop reading
}


static void my_timer_callback(struct timer_list *timer)
{

		static int y = 0;
		static int i = 0;
		
		printk("%s called (%ld)\n", __func__, jiffies);
	
		dev->leds_value = (0x01 << i);
		iowrite32(dev->leds_value, dev->regs);
		i++;  // Move to the next LED value
	
		if (i == 8) {
			i = 0;	// Reset i to 0 when all LEDs are processed
			y++;   // Move to the next iteration of the outer loop
		}
	
		if (y < 5) {
			// If not all iterations are completed, set the timer for the next iteration
			mod_timer(&my_timer, jiffies + msecs_to_jiffies(speed));
		} else {
			// All iterations are completed, reset y and stop the timer
			y = 0;
		}



}

static void my_timer_callback2(struct timer_list *timer)
{

		static int y2 = 0;
		static int i2 = 0;
	
		printk("%s called (%ld)\n", __func__, jiffies);
	
		dev->leds_value = (0x80 >> i2);
		iowrite32(dev->leds_value, dev->regs);
		i2++;  // Move to the next LED value
	
		if (i2 == 8) {
			i2 = 0;	// Reset i to 0 when all LEDs are processed
			y2++;   // Move to the next iteration of the outer loop
		}
	
		if (y2 < 5) {
			// If not all iterations are completed, set the timer for the next iteration
			mod_timer(&my_timer2, jiffies + msecs_to_jiffies(speed));
		} else {
			// All iterations are completed, reset y and stop the timer
			y2 = 0;
		}



}



// This function gets called whenever a write operation occurs on one of the character files
static ssize_t leds_write(struct file *file, const char *buffer, size_t len, loff_t *offset)
{
	int mode= 0 ;
	int y,i;
	char input[10]; 
	char pattern[]= "pattern=";
	char sens1[] = "LED dans le sens de gauche a droite";
	char sens2[] = "LED dans le sens de droite a gauche";
	char mode1[] = " LED en mode chenillard";
	char mode3[] = " LED en mode on";
	char mode4[] = " LED en mode off";
    /*
    * Get the ensea_leds_dev structure out of the miscdevice structure.
    *
    * Remember, the Misc subsystem has a default "open" function that will set
    * "file"s private data to the appropriate miscdevice structure. We then use the
    * container_of macro to get the structure that miscdevice is stored inside of (which
    * is our ensea_leds_dev structure that has the current led value).
    *
    * For more info on how container_of works, check out:
    * http://linuxwell.com/2012/11/10/magical-container_of-macro/
    */
 //   struct ensea_leds_dev *dev = container_of(file->private_data, struct ensea_leds_dev, miscdev);

 //  if (len > sizeof(dev->leds_value) - 1) {
   //     printk(KERN_ERR "Input is too long\n");
     //   return -EINVAL;
   // }


    if (copy_from_user(input, buffer, len)) {
        return -EFAULT;
    }

    input[len] = '\0'; 
    if (strncmp(input, pattern, strlen(pattern)) == 0) {
        if (sscanf(input + strlen(pattern), "%d", &mode) == 1) 
		{
            printk(KERN_INFO "Input is valid. pattern value: %d\n", mode);
        switch (mode) {
            case 1:
                printk(KERN_INFO "Processing for mode 1\n");
				dev->leds_value = 0xff ;
			    iowrite32(dev->leds_value, dev->regs);
				memset(copy2usr, 0, sizeof(copy2usr));
				strcpy(copy2usr, mode3);				
                break;
            case 2: 
                printk(KERN_INFO "Processing for mode 2\n");
				dev->leds_value = 0x00 ;
				iowrite32(dev->leds_value, dev->regs);				
				memset(copy2usr, 0, sizeof(copy2usr));
				strcpy(copy2usr, mode4);
                break;
            case 3: 
                printk(KERN_INFO "Processing for mode 3\n");
				dev->leds_value = (0x01 << 2);
				iowrite32(dev->leds_value, dev->regs);

				setup_timer(&my_timer, my_timer_callback, 0);
				mod_timer(&my_timer, jiffies + msecs_to_jiffies(speed));
				memset(copy2usr, 0, sizeof(copy2usr));
				strcpy(copy2usr, mode1);
				memset(directe, 0, sizeof(directe));
				strcpy(directe, sens2);				
                break;
            case 4: 
                printk(KERN_INFO "Processing for mode 4 ,changer le sens \n");			
				setup_timer(&my_timer2, my_timer_callback2, 0);
			    mod_timer(&my_timer2, jiffies + msecs_to_jiffies(speed));
				memset(copy2usr, 0, sizeof(copy2usr));
				strcpy(copy2usr, mode1);
				memset(directe, 0, sizeof(directe));
				strcpy(directe, sens1);		
                break;			
            default:
                printk(KERN_INFO "Invalid mode value\n");
              
                break;
        }
        	}

       }
     // Tell the user process that we wrote every byte they sent
    // (even if we only wrote the first value, this will ensure they don't try to re-write their data)
    return len;
#if 0
    // Use kstrtouint to convert the string to an unsigned integer
    if (kstrtouint(buffer, 0, &dev->leds_value) != 0) {
        printk(KERN_ERR "Failed to convert string to integer\n");
        return -EINVAL;
    }
	else {
			// We read the data correctly, so update the LEDs
			iowrite32(dev->leds_value, dev->regs);
		}
#endif
#if 0
	// Get the new led value (this is just the first byte of the given data)
	   success = copy_from_user(&dev->leds_value, buffer, sizeof(dev->leds_value));
    // If we failed to copy the value from userspace, display an error message
    if(success != 0) {
        pr_info("Failed to read led value from userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    } else {
        // We read the data correctly, so update the LEDs
        iowrite32(dev->leds_value, dev->regs);
    }
#endif

 
}

// Gets called whenever a device this driver handles is removed.
// This will also get called for each device being handled when
// our driver gets removed from the system (using the rmmod command).
static int leds_remove(struct platform_device *pdev)
{
    // Grab the instance-specific information out of the platform device
    struct ensea_leds_dev *dev = (struct ensea_leds_dev*)platform_get_drvdata(pdev);

    pr_info("leds_remove enter\n");

    // Turn the LEDs off
    iowrite32(0x00, dev->regs);

    // Unregister the character file (remove it from /dev)
    misc_deregister(&dev->miscdev);

    pr_info("leds_remove exit\n");

    return 0;
}

// Called when the driver is removed
static void leds_exit(void)
{
	int ret;
	pr_info("Ensea LEDs module exit\n");	
	ret = del_timer(&my_timer);
	if (ret)
		pr_err("%s: The timer is still is use ...\n", __func__);	
	
    remove_proc_entry(PROC_ENTRY, sub_entry);
    remove_proc_entry(SUB_DIR, NULL);

    pr_info("Ensea LEDs module successfully unregistered\n");
    
	// Unregister our driver from the "Platform Driver" bus
    // This will cause "leds_remove" to be called for each connected device
    platform_driver_unregister(&leds_platform);

    pr_info("Ensea LEDs module successfully unregistered\n");
}

// Tell the kernel which functions are the initialization and exit functions
module_init(leds_init);
module_exit(leds_exit);

// Define information about this kernel module
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Devon Andrade <devon.andrade@oit.edu>");
MODULE_DESCRIPTION("Exposes a character device to user space that lets users turn LEDs on and off");
MODULE_VERSION("1.0");



