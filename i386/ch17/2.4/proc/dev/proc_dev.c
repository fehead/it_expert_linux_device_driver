#define MODULE
#include <linux/module.h>
#include <linux/kernel.h>      

#include <linux/fs.h>          
#include <linux/errno.h>       
#include <linux/types.h>       
#include <linux/fcntl.h>       

#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/stat.h>  
#include <linux/proc_fs.h>

struct proc_dir_entry *sumproc_root_fp          = NULL;
struct proc_dir_entry *sumproc_val1_fp          = NULL;
struct proc_dir_entry *sumproc_val2_fp          = NULL;
struct proc_dir_entry *sumproc_result_fp        = NULL;

char sumproc_str1[PAGE_SIZE-80] = { 0, };
char sumproc_str2[PAGE_SIZE-80] = { 0, };

int read_sumproc_val( char *page, char **start, off_t off,int count,int *eof, void *data_unused )
{
	char *buf;
        char *realdata;

        realdata = (char *) data_unused;
	buf = page;
	buf += sprintf( buf, "Value = [%s]\n",  realdata );
                                      
        *eof = 1;
        
        return buf - page;
}

int write_sumproc_val( struct file *file, const char *buffer, unsigned long count, void *data)
{
    int   len;
    char *realdata;
    
    realdata = (char *) data;
        
    if (copy_from_user(realdata, buffer, count)) return -EFAULT;
  
    realdata[count] = '\0';
    len = strlen(realdata);
    if (realdata[len-1] == '\n')  realdata[--len] = 0;
    
    return count;
}

int read_sumproc_result( char *page, char **start, off_t off,int count,int *eof, void *data_unused )
{
    char *buf;
    int  a,b,sum;

    buf = page;
	
    a   = simple_strtoul( sumproc_str1, NULL, 10 );
    b   = simple_strtoul( sumproc_str2, NULL, 10 );
    sum = a + b;
    buf += sprintf( buf, "Result [%d + %d = %d]\n", a,b, sum );
                                      
    *eof = 1;
    return buf - page;
}

int init_module(void)
{
    sumproc_root_fp   = proc_mkdir( "sumproc", 0 ); 
    
    sumproc_val1_fp       = create_proc_entry( "val1", S_IFREG | S_IRWXU, sumproc_root_fp );
    if( sumproc_val1_fp ) 
    {
        sumproc_val1_fp->data       = sumproc_str1;
        sumproc_val1_fp->read_proc  = read_sumproc_val;
        sumproc_val1_fp->write_proc = write_sumproc_val;
    }
    
    sumproc_val2_fp   = create_proc_entry( "val2", S_IFREG | S_IRWXU , sumproc_root_fp );
    if( sumproc_val2_fp ) 
    {
        sumproc_val2_fp->data       = sumproc_str2;
        sumproc_val2_fp->read_proc  = read_sumproc_val;
        sumproc_val2_fp->write_proc = write_sumproc_val;
    }

    sumproc_result_fp  = create_proc_entry( "result", S_IFREG | S_IRUSR, sumproc_root_fp );
    if( sumproc_result_fp ) 
    {
        sumproc_result_fp->read_proc  = read_sumproc_result;
    }
        
    return 0; 
}

void cleanup_module(void)
{
    remove_proc_entry( "result"  , sumproc_root_fp );
    remove_proc_entry( "val2"    , sumproc_root_fp );
    remove_proc_entry( "val1"    , sumproc_root_fp );
    remove_proc_entry( "sumproc" , 0 );
}
