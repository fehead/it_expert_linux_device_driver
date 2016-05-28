#define MODULE  
#include <linux/module.h>  
#include <linux/kernel.h>        
  
#include <linux/fs.h>            
#include <linux/errno.h>         
#include <linux/types.h>         
#include <linux/fcntl.h>         
  
#include <asm/uaccess.h>  
#include <asm/io.h>  
  
#include <linux/time.h> 
#include <linux/timer.h> 
  
#define   KERNELTIMER_WRITE_ADDR           0x0378 
 
#define   TIME_STEP                       (2*HZ/10) 
 
typedef struct 
{ 
        struct timer_list  timer;             
	unsigned long      led; 
} __attribute__ ((packed)) KERNEL_TIMER_MANAGER; 
 
static KERNEL_TIMER_MANAGER *ptrmng = NULL; 
 
void kerneltimer_timeover(unsigned long arg ); 
 
void kerneltimer_registertimer( KERNEL_TIMER_MANAGER *pdata, unsigned long timeover ) 
{ 
     init_timer( &(pdata->timer) ); 
     pdata->timer.expires  = jiffies + timeover; 
     pdata->timer.data     = (unsigned long) pdata      ; 
     pdata->timer.function = kerneltimer_timeover       ; 
     add_timer( &(pdata->timer) ); 
} 
 
void kerneltimer_timeover(unsigned long arg ) 
{ 
   KERNEL_TIMER_MANAGER *pdata = NULL;      
    
   if( arg ) 
   { 
      pdata = ( KERNEL_TIMER_MANAGER * ) arg; 
 
      outb( (unsigned char) ( pdata->led & 0xFF ) , KERNELTIMER_WRITE_ADDR );    
 
      pdata->led = ~(pdata->led); 
 
      kerneltimer_registertimer( pdata , TIME_STEP ); 
   } 
} 
  
int init_module(void)  
{  
    ptrmng = kmalloc( sizeof( KERNEL_TIMER_MANAGER ), GFP_KERNEL ); 
    if( ptrmng == NULL ) return -ENOMEM; 
      
    memset( ptrmng, 0, sizeof( KERNEL_TIMER_MANAGER ) ); 
      
    ptrmng->led = 0; 
    kerneltimer_registertimer( ptrmng, TIME_STEP ); 
      
    return 0; 
}  
  
void cleanup_module(void)  
{  
    if( ptrmng != NULL )  
    { 
        del_timer( &(ptrmng->timer) ); 
        kfree( ptrmng ); 
    }     
    outb( 0x00 , KERNELTIMER_WRITE_ADDR );    
}  
