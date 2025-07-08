#include <linux/time.h>
#include <linux/module.h>
#include <linux/timekeeping.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>



#define HW_REGS_SPAN 			( 0x04000000 )
#define HW_REGS_MASK 			( HW_REGS_SPAN - 1 )
#define FPGA_HEX_BASE       	0x33000
#define  ALT_LWFPGASLVS_OFST 	0xff200000


static int HexSet(int index, int value);
static int afficheur_probe(struct platform_device *pdev);
static int afficheur_remove(struct platform_device *pdev);

static ssize_t afficheur_read(struct file *file, char *buffer, size_t len, loff_t *offset);


struct ensea_afficheur_dev {
    struct miscdevice miscdev;
    void __iomem *regs;
};


struct ensea_afficheur_dev *dev;


static int HexSet(int index, int value){
    uint8_t *m_hex_base;
    uint8_t szMask[] = {
        63, 6, 91, 79, 102, 109, 125, 7,
        127, 111, 119, 124, 57, 94, 121, 113
    };

    if (value < 0)
        value = 0;
    else if (value > 15)
        value = 15;

    //qDebug() << "index=" << index << "value=" << value << "\r\n";
	
	m_hex_base= (uint8_t *)dev->regs + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + FPGA_HEX_BASE ) & ( unsigned long)( HW_REGS_MASK ) );
    *((uint32_t *)m_hex_base + index) = szMask[value];

    return 0;
}


// Specify which device tree devices this driver supports
static struct of_device_id ensea_afficheur_dt_ids[] = {
    {
        .compatible = "dev,time"
    },
    { /* end of table */ }
};

// Inform the kernel about the devices this driver supports
MODULE_DEVICE_TABLE(of, ensea_afficheur_dt_ids);

// Data structure that links the probe and remove functions with our driver
static struct platform_driver afficheur_platform = {
    .probe = afficheur_probe,
    .remove = afficheur_remove,
    .driver = {
        .name = "Ensea Afficheur Driver",
        .owner = THIS_MODULE,
        .of_match_table = ensea_afficheur_dt_ids
    }
};

static const struct file_operations ensea_afficheur_fops = {
    .owner = THIS_MODULE,
    .read = afficheur_read,
};


static ssize_t afficheur_read(struct file *file, char *buffer, size_t len, loff_t *offset)
{
    int success = 0;
    char result[100];
	int tens = 0 ;
	int ones = 0 ;

    struct timespec64 ts;
    struct tm tm;
 
    ktime_get_real_ts64(&ts);
    time_to_tm(ts.tv_sec, 0, &tm);
	
//	[ 123.456789] Current Kernel Time: 2023-12-16 14:30:15
    pr_info("Current Kernel Time: %ld-%02d-%02d %02d:%02d:%02d\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

    sprintf(result, "Current Kernel Time: %ld-%02d-%02d %02d:%02d:%02d\n",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
//days	
	tens = tm.tm_mday / 10;
    ones = tm.tm_mday % 10;
	HexSet(0,tens);
	HexSet(1,ones);
//hour
    tens = tm.tm_hour / 10;
    ones = tm.tm_hour % 10;
	HexSet(2,tens);
	HexSet(3,ones);
//min
    tens = tm.tm_sec / 10;
    ones = tm.tm_sec % 10;
	HexSet(4,tens);
	HexSet(5,ones);

    // Give the user the current time
	success = copy_to_user(buffer, &result, sizeof(result));
    // If we failed to copy the value to userspace, display an error message
    if(success != 0) {
        pr_info("Failed to return current time value to userspace\n");
        return -EFAULT; // Bad address error value. It's likely that "buffer" doesn't point to a good address
    }
    return 0; 
}

static int afficheur_remove(struct platform_device *pdev)
{
    // Grab the instance-specific information out of the platform device
     dev = (struct ensea_afficheur_dev*)platform_get_drvdata(pdev);

    pr_info("leds_remove enter\n");

    // Unregister the character file (remove it from /dev)
    misc_deregister(&dev->miscdev);

    pr_info("leds_remove exit\n");

    return 0;
}


static int afficheur_probe(struct platform_device *pdev)
{
		int ret_val = -EBUSY;
		struct resource *r = 0;
	
		pr_info("afficheur_probe enter\n");
	  
		// Get the memory resources for this LED device
		r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if(r == NULL) {
			pr_err("IORESOURCE_MEM (register space) does not exist\n");
			goto bad_exit_return;
		}
	
		// Create structure to hold device-specific information (like the registers)
		dev = devm_kzalloc(&pdev->dev, sizeof(struct ensea_afficheur_dev), GFP_KERNEL);
	
		// Both request and ioremap a memory region
		// This makes sure nobody else can grab this memory region
		// as well as moving it into our address space so we can actually use it
		dev->regs = devm_ioremap_resource(&pdev->dev, r);
		if(IS_ERR(dev->regs))
			goto bad_ioremap;
	
		// Initialize the misc device (this is used to create a character file in userspace)
		dev->miscdev.minor = MISC_DYNAMIC_MINOR;	// Dynamically choose a minor number
		dev->miscdev.name = "ensea_afficheur";
		dev->miscdev.fops = &ensea_afficheur_fops;
	
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



static int __init my_module_init(void) {

		int ret_val = 0;
		ret_val = platform_driver_register(&afficheur_platform);
		if(ret_val != 0) {
			pr_err("platform_driver_register returned %d\n", ret_val);
			return ret_val;
		}
	
		pr_info("Ensea afficheur module successfully initialized!\n");
	
		return 0;
	}

static void __exit my_module_exit(void) {
    pr_info("Exiting Kernel Module\n");
   
	// Unregister our driver from the "Platform Driver" bus
    // This will cause "leds_remove" to be called for each connected device
    platform_driver_unregister(&afficheur_platform);

    pr_info("Ensea afficheur module successfully unregistered\n");
}



module_init(my_module_init);
module_exit(my_module_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("kaixuan jaing");
MODULE_DESCRIPTION("affiche de l'heure courante");
MODULE_VERSION("1.0");




























