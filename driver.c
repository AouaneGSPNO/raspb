/* 
 * driver1.c: Cree un peripherique caractere tres simple qui retourne un message fixe
 */ 
 
#include <linux/atomic.h>
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <linux/uaccess.h> /* pour get_user et put_user */ 
 
/*  Prototypes - doivent normalement etre dans un fichier .h */ 
static int device_open(struct inode *, struct file *); 
static int device_release(struct inode *, struct file *); 
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t device_write(struct file *, const char __user *, size_t, 
                            loff_t *); 
 
#define SUCCESS 0 
#define DEVICE_NAME "chardev" /* le nom du peripherique comme il apparait dans /proc/devices   */ 
 
/* Les variables globales sont declarees statiques, elles sont globales dans le fichier. */ 
 
static int major; /* le nombre majeur affecte a notre driver */ 
 
enum { 
    CDEV_NOT_USED = 0, 
    CDEV_EXCLUSIVE_OPEN = 1, 
}; 
 
/* le fichier est-il ouvert? utilise pour prevenir les acces multiples au fichier */ 
static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
 
 
static struct class *cls; 
 
static struct file_operations chardev_fops = { 
	.read = device_read, 
	.write = device_write, 
	.open = device_open, 
	.release = device_release, 
}; 
 
static int __init chardev_init(void) 
{ 
	major = register_chrdev(0, DEVICE_NAME, &chardev_fops); 
 
	if (major < 0) { 
		pr_alert("Enregistrement du periph. echoue avec majeur %d\n", major); 
		return major; 
    } 
 
    pr_info("Nombre majeur affecte: %d.\n", major); 
 
    cls = class_create(THIS_MODULE, DEVICE_NAME); 
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME); 
 
    pr_info("Periph. cree dans /dev/%s\n", DEVICE_NAME); 
 
    return SUCCESS; 
} 
 
static void __exit chardev_exit(void) 
{ 
    device_destroy(cls, MKDEV(major, 0)); 
    class_destroy(cls); 
 
    /* desenregistrer le peripherique */ 
    unregister_chrdev(major, DEVICE_NAME); 
} 
 
/* Methodes */ 
 
/* Appelee quand un processus essaie d'ouvrir le fichier periph., comme 
 * "sudo cat /dev/chardev" 
 */ 
static int device_open(struct inode *inode, struct file *file) 
{ 
 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 
 
	pr_info("On ouvre le peripherique ...\n");
    try_module_get(THIS_MODULE); 
 
    return SUCCESS; 
} 
 
/* Appelee quand un processus ferme le fichier periph. */ 
static int device_release(struct inode *inode, struct file *file) 
{ 
    /* Prets pour le prochain appel */ 
    atomic_set(&already_open, CDEV_NOT_USED); 
	pr_info("On ferme le peripherique ...\n");
 
    /* Decremente le nombre d'usage, sinon, apres ouverture du fichier, il sera impossible 
     * de retirer le module.
     */ 
    module_put(THIS_MODULE); 
 
    return SUCCESS; 
} 
 
/* Appelee quand un processus, qui a deja ouvert un fichier dev, essaie de lire 
 * depuis. 
 */ 
static ssize_t device_read(struct file *filp, /* voir include/linux/fs.h   */ 
                           char __user *buffer, /* buffer a remplir avec des donnees */ 
                           size_t length, /* taille du buffer     */ 
                           loff_t *offset) 
{ 
	
	if (*offset)
	{
		return 0;
	}
	 
	if(length>=2)
	{
        /* Le buffer est dans le segment data de l'utilisateur, et pas das le  
      	 * segment du kernel, alors, l'affectation "*" ne va pas marcher.  On doit utiliser 
         * put_user qui copie les donnes du kernel data segment vers 
         * le user data segment. 
         */ 
		put_user('X', buffer++);
		put_user('\n', buffer++);
		*offset = 2;
	 	return 2;
	} 
	else
	 	return 0;
 
} 
 
/* Appelee quand un processus ecrit dans le fichier dev: echo "hi" > /dev/hello */ 
static ssize_t device_write(struct file *filp, const char __user *buff, 
                            size_t len, loff_t *off) 
{ 
    pr_alert("Oops, cette operation n'est pas supportee.\n"); 
    return -EINVAL; 
} 
 
module_init(chardev_init); 
module_exit(chardev_exit); 
 
MODULE_LICENSE("GPL");
