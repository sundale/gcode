#include "rs274ngc.hh"
#include "rs274ngc_interp.hh"
#include "rs274ngc_return.hh"
#include "inifile.hh"		// INIFILE
#include "canon.hh"		// _parameter_file_name
#include "config.h"		// LINELEN
#include "tool_parse.h"
#include <stdio.h>    /* gets, etc. */
#include <stdlib.h>   /* exit       */
#include <string.h>   /* strcpy     */
#include <getopt.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <string.h>
#include <linux/input.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define bit(x) (1<<(x))
#define SPI_SPEED 1000000 //1M Hz
#define SPI_BITS 8

float pulse_equivalency_x = 0.001250;//pulse~mm
float pulse_equivalency_y = 0.001250;//pulse~mm
float pulse_equivalency_z = 0.001250;//pulse~mm

int spi_fd = 0;//fpga_fd

Interp interp_new;
//Inifile inifile;

#define active_settings  interp_new.active_settings
#define active_g_codes   interp_new.active_g_codes
#define active_m_codes   interp_new.active_m_codes
#define error_text	 interp_new.error_text
#define interp_execute	 interp_new.execute
#define file_name	 interp_new.file_name
#define interp_init	 interp_new.init
#define stack_name	 interp_new.stack_name
#define line_text	 interp_new.line_text
#define line_length	 interp_new.line_length
#define sequence_number  interp_new.sequence_number
#define interp_close     interp_new.close
#define interp_exit      interp_new.exit
#define interp_open      interp_new.open
#define interp_read	 interp_new.read
#define interp_load_tool_table interp_new.load_tool_table

/*

This file contains the source code for an emulation of using the six-axis
rs274 interpreter from the EMC system.

*/

/*********************************************************************/

/* report_error

Returned Value: none

Side effects: an error message is printed on stderr

Called by:
  interpret_from_file
  interpret_from_keyboard
  main

This

1. calls error_text to get the text of the error message whose
code is error_code and prints the message,

2. calls line_text to get the text of the line on which the
error occurred and prints the text, and

3. if print_stack is on, repeatedly calls stack_name to get
the names of the functions on the function call stack and prints the
names. The first function named is the one that sent the error
message.


*/

void report_error( /* ARGUMENTS                            */
 int error_code,   /* the code number of the error message */
 int print_stack)  /* print stack if ON, otherwise not     */
{
  char interp_error_text_buf[LINELEN];
  int k;

  error_text(error_code, interp_error_text_buf, 5); /* for coverage of code */
  error_text(error_code, interp_error_text_buf, LINELEN);
  fprintf(stderr, "%s\n",
          ((interp_error_text_buf[0] == 0) ? "Unknown error, bad error code" : interp_error_text_buf));
  line_text(interp_error_text_buf, LINELEN);
  fprintf(stderr, "%s\n", interp_error_text_buf);
  if (print_stack == ON)
    {
      for (k = 0; ; k++)
        {
          stack_name(k, interp_error_text_buf, LINELEN);
          if (interp_error_text_buf[0] != 0)
            fprintf(stderr, "%s\n", interp_error_text_buf);
          else
            break;
        }
    }
}

/***********************************************************************/

/* interpret_from_keyboard

Returned Value: int (0)

Side effects:
  Lines of NC code entered by the user are interpreted.

Called by:
  interpret_from_file
  main

This prompts the user to enter a line of rs274 code. When the user
hits <enter> at the end of the line, the line is executed.
Then the user is prompted to enter another line.

Any canonical commands resulting from executing the line are printed
on the monitor (stdout).  If there is an error in reading or executing
the line, an error message is printed on the monitor (stderr).

To exit, the user must enter "quit" (followed by a carriage return).

*/

int interpret_one_line(  /* ARGUMENTS                 */
 int block_delete,            /* switch which is ON or OFF */
 int print_stack,
 char *line)             /* option which is ON or OFF */
{
	int status;
	
	status = interp_read(line);
	if ((status == INTERP_EXECUTE_FINISH) && (block_delete == ON));
	else if (status == INTERP_ENDFILE);
	else if ((status != INTERP_EXECUTE_FINISH) &&
			 (status != INTERP_OK))
	  report_error(status, print_stack);
	else
	  {
		status = interp_execute();
		if ((status == INTERP_EXIT) ||
			(status == INTERP_EXECUTE_FINISH));
		else if (status != INTERP_OK)
		  report_error(status, print_stack);
	  }
}

/************************************************************************/

/* read_tool_file

Returned Value: int
  Returns 0 for success, nonzero for failure.  Failures can be caused by:
  1. The file named by the user cannot be opened.
  2. Any error detected by loadToolTable()

Side Effects:
  Values in the tool table of the machine setup are changed,
  as specified in the file.

Called By: main
*/

int read_tool_file(  /* ARGUMENTS         */
 const char * tool_file_name)   /* name of tool file */
{
  char buffer[1000];

  if (tool_file_name[0] == 0) /* ask for name if given name is empty string */
    {
      fprintf(stderr, "name of tool file => ");
      fgets(buffer, 1000, stdin);
      buffer[strlen(buffer) - 1] = 0;
      tool_file_name = buffer;
    }

  return loadToolTable(tool_file_name, _tools, 0, 0, 0);
}

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static uint16_t spi_read_single(int fd, uint16_t addr)
{
	int ret;
	uint8_t tx_temp[4];
	uint8_t rx_temp[4];
	struct spi_ioc_transfer tr[2];

	tr[0].tx_buf = (unsigned long)tx_temp,
	tr[0].rx_buf = (unsigned long)rx_temp,
	tr[0].len = 2,
	tr[0].delay_usecs = 0,//delay,
	tr[0].speed_hz = SPI_SPEED,
	tr[0].bits_per_word = SPI_BITS,
	tr[0].cs_change = 0,
	tr[1].tx_buf = (unsigned long)&tx_temp[2],
	tr[1].rx_buf = (unsigned long)rx_temp,
	tr[1].len = 2,
	tr[1].delay_usecs = 0,//delay,
	tr[1].speed_hz = SPI_SPEED,
	tr[1].bits_per_word = SPI_BITS,
	tr[1].cs_change = 0,
	
	memset(tx_temp,0xff,4);
	memset(rx_temp,0,4);
	tx_temp[0] = (addr >> 8);
	tx_temp[1] = (addr & 0x00ff);
	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("read spi:[%04x]=0x%02x%02x \r\n",addr,rx_temp[0],rx_temp[1]);
	return 0;
	
}
static uint16_t spi_write_single(int fd,uint16_t addr,uint16_t dat)
{

	int ret;
	uint8_t tx_temp[4];
	uint8_t rx_temp[4];
	struct spi_ioc_transfer tr[2];

	tr[0].tx_buf = (unsigned long)tx_temp,
	tr[0].rx_buf = (unsigned long)rx_temp,
	tr[0].len = 2,
	tr[0].delay_usecs = 0,//delay,
	tr[0].speed_hz = SPI_SPEED,
	tr[0].bits_per_word = SPI_BITS,
	tr[0].cs_change = 0,
	tr[1].tx_buf = (unsigned long)&tx_temp[2],
	tr[1].rx_buf = (unsigned long)rx_temp,
	tr[1].len = 2,
	tr[1].delay_usecs = 0,//delay,
	tr[1].speed_hz = SPI_SPEED,
	tr[1].bits_per_word = SPI_BITS,
	tr[1].cs_change = 0,

	tx_temp[0] = (addr >> 8) | 0x80;
	tx_temp[1] = (addr & 0x00ff);
	tx_temp[2] = dat >> 8;
	tx_temp[3] = dat & 0x00ff;
	memset(rx_temp,0,4);
	ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("write spi:[%04x]=0x%04x \r\n",addr,dat);
	return 0;	
}

static int spi_read_multi(int fd,uint16_t addr ,uint16_t *buf,uint16_t count)
{
	
	int ret;
	uint8_t tx_temp[4];
	uint8_t rx_temp[4];
	int i;
	uint16_t my_addr;
	struct spi_ioc_transfer *tr;
	uint16_t *t_temp,*r_temp;
	if((count>20)||(count == 0))
		return 0;
	
	tr = (struct spi_ioc_transfer *)malloc(sizeof(struct spi_ioc_transfer)*(count*2));
	if(tr == NULL)
		return -1;
	t_temp = (uint16_t *)malloc(sizeof(uint16_t)*(count*2));
	if(t_temp == NULL)
	{
		free(tr);
		return -1;
	}
	r_temp = (uint16_t *)malloc(sizeof(uint16_t)*(count*2));
	if(r_temp == NULL)
	{
		free(tr);
		free(t_temp);
		return -1;
	}
	my_addr = addr;
	memset(tx_temp,0xff,4);
	memset(rx_temp,0,4);


	for(i=0;i<count;i++)
	{
		t_temp[i*2] = (((my_addr&0x00ff)<<8)|(my_addr>>8));
		r_temp[i*2] = 0x00;
		tr[i*2].tx_buf = (unsigned long)&t_temp[i*2];
		tr[i*2].rx_buf = (unsigned long)&r_temp[i*2]; 
		tr[i*2].len = 2;
		tr[i*2].delay_usecs = 0;
		tr[i*2].speed_hz = SPI_SPEED;
		tr[i*2].bits_per_word = SPI_BITS;
		tr[i*2].cs_change = 0;
		t_temp[i*2+1] = 0x00;
		tr[i*2+1].tx_buf = (unsigned long)&t_temp[i*2+1];
		tr[i*2+1].rx_buf = (unsigned long)(&buf[i]); 
		tr[i*2+1].len = 2;
		tr[i*2+1].delay_usecs = 0;
		tr[i*2+1].speed_hz = SPI_SPEED;
		tr[i*2+1].bits_per_word = SPI_BITS;
		if(i==(count-1))
			tr[i*2+1].cs_change = 0;
		else
			tr[i*2+1].cs_change = 1;
		my_addr = my_addr +2;
	}
	ret = ioctl(fd, SPI_IOC_MESSAGE(count*2), tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("read spi:[%04x]\r\n",addr);
	for(i=0;i<count;i++)
		printf("%04x ",buf[i]);
	printf("\r\n");
	free(tr);
	free(r_temp);
	free(t_temp);
	return 0;
}

static int spi_write_multi(int fd,uint16_t addr ,uint16_t *buf,uint16_t count)
{
	
	int ret;
	uint8_t tx_temp[4];
	uint8_t rx_temp[4];
	uint16_t *t_temp,*r_temp;
	int i;
	struct spi_ioc_transfer *tr;
	uint16_t my_addr;
	if((count>20)||(count == 0))
		return 0;
	
	tr = (struct spi_ioc_transfer *)malloc(sizeof(struct spi_ioc_transfer)*(count*2));
	if(tr == NULL)
		return -1;
	t_temp = (uint16_t *)malloc(sizeof(uint16_t)*(count*2));
	if(t_temp == NULL)
	{
		free(tr);
		return -1;
	}
	r_temp = (uint16_t *)malloc(sizeof(uint16_t)*(count*2));
	if(r_temp == NULL)
	{
		free(tr);
		free(t_temp);
		return -1;
	}
	memset(tx_temp,0xff,4);
	memset(rx_temp,0,4);
	tx_temp[0] = (addr >> 8) | 0x80;
	tx_temp[1] = (addr & 0x00ff);
	my_addr = addr;


	for(i=0;i<count;i++)
	{
		t_temp[i*2] = (((my_addr&0x00ff)<<8)|(my_addr>>8));
		t_temp[i*2] |= 0x0080;
		r_temp[i*2] = 0x00;
		printf("write my_addr= %x \r\n",t_temp[i*2]);
		tr[i*2].tx_buf = (unsigned long)&(t_temp[i*2]);
		tr[i*2].rx_buf = (unsigned long)&r_temp[i*2]; 
		tr[i*2].len = 2;
		tr[i*2].delay_usecs = 0;
		tr[i*2].speed_hz = SPI_SPEED;
		tr[i*2].bits_per_word = SPI_BITS;
		tr[i*2].cs_change = 0;
		t_temp[i*2+1] = (((buf[i]&0x00ff)<<8)|(buf[i]>>8));
		r_temp[i*2+1] = 0x00;
		tr[i*2+1].tx_buf = (unsigned long)&t_temp[i*2+1];
		tr[i*2+1].rx_buf = (unsigned long)(r_temp[i*2+1]); 
		tr[i*2+1].len = 2;
		tr[i*2+1].delay_usecs = 0;
		tr[i*2+1].speed_hz = SPI_SPEED;
		tr[i*2+1].bits_per_word = SPI_BITS;
		if(i==(count-1))
			tr[i*2+1].cs_change = 0;
		else
			tr[i*2+1].cs_change = 1;
		my_addr = my_addr +2;
	}
	ret = ioctl(fd, SPI_IOC_MESSAGE(count*2), tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("write spi:[%04x]\r\n",addr);
	for(i=0;i<count;i++)
		printf("%04x ",buf[i]);
	printf("\r\n");
	free(tr);
	free(r_temp);
	free(t_temp);
	return 0;
}

void line_demo(int spi_fd, int x, int y, int z)
{
	uint16_t command_buf[10] = {0};
	int x_direct = 0;
	int y_direct = 0;
	int z_direct = 0;

	if(x>=0)
	{
		x_direct = 0;
	}
	else
	{
		x_direct = 1;
	}
	if(y>=0)
	{
		y_direct = 0;
	}
	else
	{
		y_direct = 1;
	}
	if(z>=0)
	{
		z_direct = 0;
	}
	else
	{
		z_direct = 1;
	}
	x = abs(x);
	y = abs(y);
	z = abs(z);
	command_buf[0] = 100*50;//50~1us
	command_buf[1] = (bit(15)*x_direct)|(x>>16);
	command_buf[2] = x&0x0ffff;
	command_buf[3] = (bit(15)*y_direct)|(y>>16);
	command_buf[4] = y&0x0ffff;
	command_buf[5] = (bit(15)*z_direct)|(z>>16);
	command_buf[6] = z&0x0ffff;
	command_buf[7] = 1;
	spi_write_single(spi_fd, 0x1a, 0xabcd);//reset
	spi_write_single(spi_fd, 0x10e, command_buf[0]);
	spi_write_multi(spi_fd, 0x100, &command_buf[1], 7);
	//usleep(100000);
	//spi_read_single(spi_fd, 0x22);
	//spi_read_single(spi_fd, 0x24);
}

int get_pulse_x(float mm)
{
	float pulse_number;
	
	pulse_number = mm/pulse_equivalency_x;

	return (int)pulse_number;
}

int get_pulse_y(float mm)
{
	float pulse_number;
	
	pulse_number = mm/pulse_equivalency_y;

	return (int)pulse_number;
}

int get_pulse_z(float mm)
{
	float pulse_number;
	
	pulse_number = mm/pulse_equivalency_z;

	return (int)pulse_number;
}

int fpga_spi_init(void)
{
	int err = 0;
	static uint8_t mode = SPI_CPHA;

	spi_fd = open("/dev/spidev0.0",O_RDWR);
	if(spi_fd < 0)
		pabort("/dev/spidev0.0");
	err = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (err == -1)
		pabort("set spi mode");
	return spi_fd;
}

int main (int argc, char ** argv)
{
	int status;
	int block_delete = OFF;
	int print_stack = OFF;
	char line[LINELEN];

	if (read_tool_file(TOOL_CONFIGURE_PATH) != 0)
		exit(1);

	if ((status = interp_init()) != INTERP_OK)
	{
		report_error(status, print_stack);
		exit(1);
	}
	spi_fd = fpga_spi_init();
	for(; ;)
	{
		char *result;
		printf("READ => ");
		memset(line, 0, sizeof(line));
		result = fgets(line, LINELEN, stdin);
		if (!result || strcmp (line, "quit\n") == 0)
			return 0;
		interpret_one_line(block_delete, print_stack, line);
	}
	close(spi_fd);
	return 0;
}

