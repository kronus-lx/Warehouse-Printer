#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include <tcpsupp.h>
#include <string.h>
#include<stdio.h>
#include<stdlib.h>

#define scanner_Port 9004
#define scanner_IP "192.168.100.100"

//int panelHandle;

#define BARCODE_MEMORY	5

extern int global_Quality_Flag;  

//static char last_barcodes[BARCODE_MEMORY][100] = {"","","","","",};
//static int last_barcode_marker =0;


static int scanner_handle = 0;

static int bad_label_count = 0;

static int scanned_label_count = 0;

int error_count = 0; 

int scanner_callback (unsigned handle, int xType, int errCode, void *callbackData);

/*int check_read_count(int* scannedcount, int* badCount)
{
	*scannedcount = scanned_label_count;
	*badCount = bad_label_count;
	
	return 0;
	
}

*/
int init_scanner()
{
	
	ConnectToTCPServer (&scanner_handle, scanner_Port, scanner_IP, scanner_callback, 0, 0); 
	
	return(0);
}


int start_scanner()
{	
	//int i=0;
	//char message[100] = {"TEST1\r"};
	char message[100] = {" "};
	int ret = 0;
	ret = ClientTCPWrite (scanner_handle, message, strlen(message), 0);
	//ret = ClientTCPWrite (scanner_handle, message, strlen(message), 0);  
	//bad_label_count = 0;
	//scanned_label_count = 0;
	
	/*for (i=0;i<BARCODE_MEMORY;i++)
	{
		strcpy(last_barcodes[i],"");
	}
	
	
	last_barcode_marker = 0;
	*/
	return ret;
}


/*int stop_scanner()
{
	char message[100] = {"QUIT\r"};
	int ret = 0;
	
	ret = ClientTCPWrite (scanner_handle, message, strlen(message), 0);
	
	return 0;
	
}   */

int scanner_callback (unsigned handle, int xType, int errCode, void *callbackData)
{
	char data[100] = {""};
	int ret = 0;
	int i = 0;
	int len = 0;
	int scanner_handle = 0;
	
	/// Output Formating Variables ///
	
	char val[100];
	int x = 0;
	int result = 0;
	int count;
	char* piece = data;
	char localbarcode[100] = {""};
	int tmp1 = 0;
	int tmp2 = 0;
	int local_quality = 0;
	
	
	if (xType == TCP_DISCONNECT)
	{
		printf("Disconnected\n");
	}
		
	if (xType == TCP_DATAREADY)
	{
		
			ret = ClientTCPRead (handle, data, 100, 0);
		
			if(ret > 0)
			{	
				int len =0;
				
				len = strlen(data);
				//printf("%s\n",data);
				
				//printf("%d\n\n", error_count);
				
				if(len < 15)
				{
					error_count++;
					
					if(error_count > 5)
					{
						global_Quality_Flag = 1;
						error_count = 0;
					}
					  
				}
				
				else
				{
				
					error_count = 0;
					
				
				}
			}
			
		
		
			
	}
		
		return 0;
		
}
		/*
		if (ret > 0) 
		{
			len = strlen(data);
			for (i=0;i<len;i++)
			{
				if (isprint(data[i]) == 0)
				{
					data[i] = '*';
				}
			}
			  
			  
			// Print out raw Feedback Data // 
			len = strlen(data);
			for(i = 0; i<len; i++)
			{
				// Replace char ':' and % with blank
				if(data[i] == ':') data[i] = ' ';
				if(data[i] == '%') data[i] = ' ';
			}
			//Raw Print//
			//printf("%s\n",data);
			if (strstr(data,"ERROR") == NULL) // If there is an ERROR in the Code Do not Print to Output
			{
				//Split input string into separate variables 
				len = sscanf(data,"%s %d %d %d",localbarcode, &tmp1, &tmp2, &local_quality);
				if (len == 4)
				{
					
					for (i=0;i<BARCODE_MEMORY;i++)
					{
						if (strcmp(localbarcode,last_barcodes[i]) == 0) break; // Compare localbarcode to last_barcode	
					}
					
					if (i == BARCODE_MEMORY)
					{
						// new barcode
					
						scanned_label_count++;
						
						
						if (local_quality < 50)
							bad_label_count++;
						
						//printf("%s %d\n", localbarcode, local_quality);
						
						//printf("%d %s %d\n",len,localbarcode, local_quality);
								
						//printf("%d %d\n",scanned_label_count,bad_label_count);
						
						strcpy(last_barcodes[last_barcode_marker],localbarcode);
						
						last_barcode_marker++;
						if (last_barcode_marker >= BARCODE_MEMORY) 
							last_barcode_marker = 0;
					}
					
				}
				
			}
		}
		
	}  
	return 0;	
} */








