#include "cvi_db.h"
#include <rs232.h>
#include "C2_SR750.h"
#include <formatio.h>
#include <utility.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "pcb_print.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define SER_PRINT 1

#define	X_OFF 0
#define Y_OFF 0

int print_count = 0;

char config_path[100] = {""};

int read_write_int(int force_write,char* name,int* data,int default_val,char* comment);

int CVICALLBACK MainCallbackFunction (int panelOrMenuBarHandle,
                                  int controlOrMenuItemID,
                                  int event,
                                  void *callbackData,
                                  int eventData1,
                                  int eventData2);



char assembling_message[100] = {""};
char pending_message[100] = {""};

int db_hdbc = -1;
int db_open_connection(void);
int db_close_connection(void);

int get_range(char* unit_serial,int* error,char* error_str,int* range_id);
int get_brd_type(char* brd_serial,int* error,char* error_str,int* brd_type);

int cba_write(int range_id,char* main_brd_serial,int* error,char* error_str,char* unit_serial);
int assy_write(int range_id,char* main_brd_serial,char* handset_brd_serial,char* lamp_brd_serial,int* error,char* error_str,char* unit_serial);
int sts_write(char* unit_serial,char* sts_brd_serial,int* error,char* error_str);
int print_write(char* unit_serial,int* error,char* error_str);
int get_pcb_serial_start(int* ser_start);
int get_pcb_serial_next(int* ser_start);

int print_internal_serial(char* ser_num);

//int prnt_test(char* mac_num);
//int prnt_test(int range,int number);
int prnt_test(int range,int number1,char* text,int speed_offset,int x_offset,int y_offset);

int prnt_status(int* error_all,int* paper_out,int* pause,
					int* buffer_full,int* head_up,int* ribbon_out,int* num_in_buffer);

int record_print_range(int pcb_range,int serial_start,int serial_last,int month,int day);

static int panelHandle;

int com_port_open_flag = 0;

char g_nova_computer_name[100] = {""};

// Look for this to check incoming quality// 
int global_Quality_Flag = 0;

int main (int argc, char *argv[])
{
	int ser_start = 0;
	int ret=0;
	
	int x_offset = 0;
	int y_offset = 0;
	int speed_offset = 0;
	
	char file_name[500] = {""};
	
	int day,month,year;
	
	char* ptr = NULL;
	//int error = 0;
	//int count = 0;
	
	/**** Initialise the Scanner SR750 ******/
	 init_scanner();
	/****************************************/
	
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "pcb_print.uir", PANEL)) < 0)
		return -1;

	ptr = getenv ("COMPUTERNAME");
	if (ptr != NULL) strcpy(g_nova_computer_name,ptr);
	
	ret = GetFirstFile ("c:\\C2_setup", 1, 0, 0, 0, 0, 1, file_name);

	if (ret != 0) MakeDir ("c:\\C2_setup");
	
	ret = GetFirstFile ("c:\\C2_setup\\nova_pcb_print", 1, 0, 0, 0, 0, 1, file_name);
	
	if (ret != 0) MakeDir ("c:\\C2_setup\\nova_pcb_print");
	
	strcpy(config_path,"c:\\C2_setup\\nova_pcb_print");
	
	
	GetSystemDate (&month, &day, &year);
	SetCtrlVal(panelHandle,PANEL_NUM_DAY,day);
	SetCtrlVal(panelHandle,PANEL_NUM_MONTH,month);
	
	ret = read_write_int(0,"x_offset",&x_offset,0,"the value of the label x_offset for x dimention trimming");
	if (ret != 0) return(1);

	ret = read_write_int(0,"y_offset",&y_offset,0,"the value of the label y_offset for y dimention trimming");
	if (ret != 0) return(1);

	ret = read_write_int(0,"speed_offset",&speed_offset,5,"the value of the label speed_offset for y dimention trimming at speed");
	if (ret != 0) return(1);
	
	SetCtrlVal(panelHandle,PANEL_NUM_X,x_offset);
	SetCtrlVal(panelHandle,PANEL_NUM_Y,y_offset);
	SetCtrlVal(panelHandle,PANEL_NUM_S,speed_offset);
	
	ret = db_open_connection();
	if (ret != 0)
	{
		MessagePopup ("ERROR", "Can't Access DB");
		db_close_connection();
		return(0);
	}

	//InstallMainCallback (MainCallbackFunction,NULL,0);
	
	ret = get_pcb_serial_start(&ser_start);
	if (ret != 0)
	{
		MessagePopup ("ERROR", "Can't READ DB");
		db_close_connection();
		return(0);
	}
	
	SetCtrlVal (panelHandle, PANEL_NUMERIC_2, ser_start);
	
	
	DisplayPanel (panelHandle);

	//ret = prnt_status(&error,NULL,NULL,NULL,NULL,NULL,&count);

	//print_internal_serial("HL01A0001001S");
	//prnt_test("B00010500000101");
	//prnt_test(106,1);


	RunUserInterface ();
	
	db_close_connection();
	DiscardPanel (panelHandle);
	return 0;
}

int CVICALLBACK quit (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			QuitUserInterface(0);
			break;
		}
	return 0;
}

int CVICALLBACK quit_popup (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	switch (event)
		{
		case EVENT_GOT_FOCUS:

			break;
		case EVENT_LOST_FOCUS:

			break;
		case EVENT_CLOSE:
			//QueueUserEvent (1000, panel, PANEL_OP_CB_CANCEL);
			
			break;
		}
	return 0;
}




int CVICALLBACK start (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	
	int pcb_range = 0;
	//int serial_start = 0;
	int num_to_print = 0;
	
	int loc_error = 0;
	
	int loc_paper_out = 0;
	int loc_pause = 0;
	int loc_num_in_buffer = 0;
	int loc_buffer_full = 0;
	
	int loc_head_up = 0;
	int loc_ribbon_out = 0;
	
	
	int i=0;
	int j = 0;
	int ret = 0;
	int ret2 = 0;
	int temp = 0;
	
	int ret3 = 0;
	int serial_print1 = 0;
	//int serial_print2 = 0;
	//int serial_print3 = 0;
	//int serial_print4 = 0;
	
	int x_offset = 0;
	int y_offset = 0;
	
	int speed_offset = 0;
	
	int day = 0;
	int month = 0;
	
	char lot_str[50] = {""};
	
	int loc_serial_num_max = 0;
	
	int error = 0;
	
	int first_serial_to_print = 0;
	
	switch (event)
		{
		case EVENT_COMMIT:

			//printf("badger\n");
					//SELECT GEN_ID(PCB_SERIAL_REF_001,0) FROM RDB$DATABASE
					//ALTER SEQUENCE PCB_SERIAL_REF_001 RESTART WITH 1500
			
					
					ret3 = start_scanner();
					
					
			
					GetCtrlVal (panelHandle, PANEL_NUMERIC_2, &first_serial_to_print);
			
					GetCtrlVal(panelHandle,PANEL_NUM_DAY,&day);
					GetCtrlVal(panelHandle,PANEL_NUM_MONTH,&month);

					sprintf(lot_str,"%02d/%02d",day,month);

					GetCtrlVal(panelHandle,PANEL_NUM_X,&x_offset);
					GetCtrlVal(panelHandle,PANEL_NUM_Y,&y_offset);

					GetCtrlVal(panelHandle,PANEL_NUM_S,&speed_offset);

					ret = read_write_int(1,"x_offset",&x_offset,x_offset,"the value of the label x_offset for x dimention trimming");
					if (ret != 0) return(1);

					ret = read_write_int(1,"y_offset",&y_offset,y_offset,"the value of the label y_offset for y dimention trimming");
					if (ret != 0) return(1);

					ret = read_write_int(0,"serial_num_max",&loc_serial_num_max,-1,"the value of the serial number accumulation");
					if (ret != 0) return(1);
					if (loc_serial_num_max < 0)
					{
						MessagePopup ("ERROR", "NO LOCAL VAL FOR SERIAL ACCUMULATION");
						return(0);
					}
			
					GetCtrlVal (panelHandle, PANEL_NUMERIC, &pcb_range);
					//GetCtrlVal (panelHandle, PANEL_NUMERIC_2, &serial_start);
					GetCtrlVal (panelHandle, PANEL_NUMERIC_3, &num_to_print);
					

					ret = prnt_status(&loc_error,&loc_paper_out,&loc_pause,
								&loc_buffer_full,&loc_head_up,&loc_ribbon_out,&loc_num_in_buffer);
					if (ret != 0)
					{
						MessagePopup ("ERROR", "NO COMMS WITH PRINTER");
						break;
					}

					if (loc_error != 0)
					{
						MessagePopup ("ERROR", "PRINTER IS NOT READY");
						break;
					}

					for(i=0;i<(num_to_print);i++)
					{
						j = 0;
						// One Second Dealy Per Label Print
						Delay(0.1);
						for(;;)
						{
							j++;
							ret = prnt_status(&loc_error,&loc_paper_out,&loc_pause,
								&loc_buffer_full,&loc_head_up,&loc_ribbon_out,&loc_num_in_buffer);
							//printf("%d STATUS : ret =  %d, error = %d, count = %d\n",j,ret,loc_error,loc_num_in_buffer);
							if (ret != 0) break;
							if (loc_error != 0) 
							{
								ret2 = ConfirmPopup ("ERROR",
											  "An ERROR was detected\nDo you wish to continue(YES) or QUIT (NO)");
								
								if (ret2 != 1) break;
							}
							if (loc_error == 0)
							{
								if (loc_num_in_buffer < 20) 
								{
									break;
								}
								else
								{
									//Delay(0);
									//printf("%d STATUS : ret =  %d, error = %d, count = %d, ser = %d\n",j,ret,loc_error,loc_num_in_buffer,serial_print1);
								}
							}
							
						}
								
						if (ret != 0)
						{
							break;
						}
						
						if (loc_error != 0)
						{
							break;
						}
						
						//Scanner check 
						if(global_Quality_Flag != 0)
						{
							
							MessagePopup ("ERROR", "Bad barcode Quality. Check Barcode or Calibration!");
							global_Quality_Flag = 0;
							error = 1;
							break;
							
						}
					
						//Sleep(200);
						
						ret = get_pcb_serial_next(&serial_print1);
						if (ret != 0)
						{
							MessagePopup ("ERROR", "Can't Read DB");
							error = 1;
							break;
						}

						if (loc_serial_num_max >= serial_print1)
						{
							MessagePopup ("ERROR", "Database Issue With Accumulation. Please Call an Engineer.");
							error = 1;
							return(-1);
						}
						
						
						//serial_print1 = (serial_start + (i*4));
						//serial_print2 = (serial_start + (i*4)) + 1;
						//serial_print3 = (serial_start + (i*4)) + 2;
						//serial_print4 = (serial_start + (i*4)) + 3;
						
						//ret = prnt_status(&error,NULL,NULL,NULL,NULL,NULL,&count);
						//printf("serial number start %d %d %d %d\n",serial_print1,serial_print2,serial_print3,serial_print4);
						//loc_num_in_buffer
						
						if (i > 1) 
						{
							temp = speed_offset;
						}
						else
						{
							if (i == 0) temp = 0;
							if (i == 1) temp = speed_offset / 2;
						}
						
						//printf("%d STATUS : ret =  %d, error = %d, count = %d\n",j,ret,loc_error,loc_num_in_buffer);
						
						prnt_test(pcb_range,serial_print1,lot_str,temp,x_offset,y_offset);
						
						print_count++;
						
						/********* RUN SCANNER *************/
						
						
						
						SetCtrlVal (panelHandle, PANEL_NUMERIC_2, (serial_print1+1));
						SetCtrlVal (panelHandle, PANEL_NUMERIC_4, print_count);
						
					}

					// record range printed.
					// serial_print1 // last printed
					// first_serial_to_print
					// lot_str
					// day
					// month
					
					record_print_range(pcb_range,first_serial_to_print,serial_print1,month,day);
					
					if (error == 0)
					{
						ret = read_write_int(1,"serial_num_max",&temp,serial_print1,"the value of the serial number accumulation");
						if (ret != 0)
						{
							MessagePopup ("ERROR", "Can't write local accumulation");
							break;
						}
					}
	
					
					//serial_print1
					
					ret = get_pcb_serial_start(&serial_print1);
					if (ret != 0)
					{
						MessagePopup ("ERROR", "Can't Read DB");
						break;
					}

					SetCtrlVal (panelHandle, PANEL_NUMERIC_2, serial_print1);
					
					CloseCom(SER_PRINT);
					com_port_open_flag = 0;
			
			break;
		}
	return 0;
}

int CVICALLBACK reset_count (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
	
	switch (event)
		{
		case EVENT_COMMIT:

			print_count=0;
			SetCtrlVal (panelHandle, PANEL_NUMERIC_4, print_count);

			break;
		}
	return 0;
}


int CVICALLBACK test (int panel, int control, int event,
		void *callbackData, int eventData1, int eventData2)
{
int i = 0;	

	int loc_error = 0;
	
	int loc_paper_out = 0;
	int loc_pause = 0;
	int loc_num_in_buffer = 0;
	int loc_buffer_full = 0;
	
	int loc_head_up = 0;
	int loc_ribbon_out = 0;
	
	int ret =0;
	
	switch (event)
		{
		case EVENT_COMMIT:

			for(i=0;i<1;i++)
			{
				ret = prnt_status(&loc_error,&loc_paper_out,&loc_pause,
								&loc_buffer_full,&loc_head_up,&loc_ribbon_out,&loc_num_in_buffer);
				
				printf("%d STATUS : ret =  %d, error = %d, count = %d\n",i,ret,loc_error,loc_num_in_buffer);			
			
				prnt_test(0,i,"30/04",0,0,0);
			}
			
			printf("badger\n");
			// 23
			break;
		}
	return 0;
}



int db_open_connection()
{
	int resCode;

	db_hdbc = DBNewConnection();
	if (db_hdbc <= 0) return(-1);
	
	//DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_STRING,"DSN=cydendb1;MODIFYSQL=1;UID=CYDEN_MODIFY;PWD=cydenmodify2012" );
	//DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_STRING,"DSN=CYDENDB1");
	DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_STRING,"DSN=CAMDB;MODIFYSQL=1;UID=SYSDBA;PWD=odin1969" );
	DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_COMMAND_TIMEOUT, 10);
	DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_TIMEOUT, 10);
	DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_ISOLATION_LEVEL, DB_ISOLATION_LEVEL_READ_COMMITTED);
	
	DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_MODE,
							  DB_CONN_MODE_READ_WRITE);
	
	
	resCode = DBOpenConnection(db_hdbc);
	if (resCode != DB_SUCCESS)
	{
		return(resCode);
	}
	
	resCode = DBSetAttributeDefault (db_hdbc, ATTR_DB_USE_COMMAND,
									 DB_USE_COMMAND);
	if (resCode != DB_SUCCESS)
	{
		DBCloseConnection(db_hdbc);
		DBDiscardConnection(db_hdbc);
		return(resCode);
	}

	resCode = DBSetAttributeDefault(db_hdbc, ATTR_DB_COMMAND_TYPE,
                                    DB_COMMAND_TEXT);	
	if (resCode != DB_SUCCESS)
	{
		DBCloseConnection(db_hdbc);
		DBDiscardConnection(db_hdbc);
	
		return(resCode);
	}
	
	return(0);
}

int db_close_connection()
{
	int resCode;

	if (db_hdbc < 0) return(DB_SUCCESS);

	resCode = DBCloseConnection(db_hdbc);
	if (resCode != DB_SUCCESS)
	{
		DBDiscardConnection(db_hdbc);
		return(resCode);
	}
	resCode = DBDiscardConnection(db_hdbc);
	if (resCode != DB_SUCCESS)
	{
		return(resCode);
	}
	
	db_hdbc = -1;
	
	return(DB_SUCCESS);
}

int CVICALLBACK MainCallbackFunction (int panelOrMenuBarHandle,
                                  int controlOrMenuItemID,
                                  int event,
                                  void *callbackData,
                                  int eventData1,
                                  int eventData2)
{
	char stemp[5] = {""};
	int i=0;
	//int ret =0;


	switch (event)
		{
		case EVENT_KEYPRESS:
			
			if (controlOrMenuItemID == 0)
			{
				//if (panelOrMenuBarHandle == panelHandleMan) return(0);
			
				if ((eventData1 < 0x100)&&(eventData1 > 0))
				{
					stemp[0] = eventData1;
					stemp[1] = 0;
					strcat(assembling_message,stemp);
					
				}

				i = strlen(assembling_message);
			
				if (((eventData1 == 0x500)||(i > 99))&&(i > 0)) 
				{
					strcpy(pending_message,assembling_message);
					strcpy(assembling_message,"");
				
					
					if ((pending_message[0] == 'H')||(pending_message[0] == 'F'))
					{
						SetCtrlVal (panelHandle, PANEL_STRING_2, pending_message);
					}
					
					strcpy(pending_message,"");
				
					/*
					ret = validate_tally_checksum(pending_message);
					if (ret == 0)
					{
						if (pending_message[0] == 'T') process_tally(pending_message);
						if (pending_message[0] == 'C') process_clockcard(pending_message);
						strcpy(pending_message,"");
					}
					else
					{
						strcpy(assembling_message,"");
						strcpy(pending_message,"");
					}
					*/
				}
				return(1);
			}
			break;
		}

	return(0);
}

// ****************************************************************************
// ********** SERIAL STUFF ****************************************************


int print_internal_serial(char* ser_num)
{
 	char data[100];
 	
 	char ser_bar[20] = {""};
 	char ser_lab[20] = {""};

	//sprintf(ser_lab,"%07d",ser_num);
 	//sprintf(ser_bar,"%07d",ser_num);

	sprintf(ser_lab,"%s",ser_num);
 	sprintf(ser_bar,"%s",ser_num);
 
 
 	if (com_port_open_flag == 0)
 	{
 		OpenComConfig (SER_PRINT, "", 115200, 2, 8, 1, 512, 512);
 		com_port_open_flag = 1;
 	}

 	strcpy(data,"^XA");		// start of Label & Origin(across & down)
	ComWrt(SER_PRINT,data,strlen(data));

 	strcpy(data,"^MUd,300,300");		// units for format
	ComWrt(SER_PRINT,data,strlen(data));

	// SET DEFAULT BARCODE PARAMETERS
	//strcpy(data,"^BY2,3,100");
	strcpy(data,"^BY1,2,100");
	ComWrt(SER_PRINT,data,strlen(data));		// set defaults All barcodes
	// END SET DEFAULT BARCODE PARAMETERS

	sprintf(data,"^LH%d,%d",X_OFF,Y_OFF);
	ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^FO25,16");		// field origin for barcode (x,y)
	ComWrt(SER_PRINT,data,strlen(data));	


	//strcpy(data,"^B3N,N,70,N,N"); 		// setup CODE 39 Orientation-N,Height-35
	strcpy(data,"^BCN,60,N,N,Y,A"); 		// setup CODE 128 Orientation-N,Height-70
	ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^FD%s^FS",ser_bar); //
	ComWrt(SER_PRINT,data,strlen(data));

	// TEXT FOR NUMBER
	sprintf(data,"^FO110,85");		// origin for text
	ComWrt(SER_PRINT,data,strlen(data));	

	sprintf(data,"^A0,N,28,28");//			// font select and scale
	ComWrt(SER_PRINT,data,strlen(data));	

	sprintf(data,"^FD%s^FS",ser_lab);//
	ComWrt(SER_PRINT,data,strlen(data));	
	// END TEXT FOR NUMBER
		
	strcpy(data,"^XZ");				// end of Label
	ComWrt(SER_PRINT,data,strlen(data));
 							  
	do
	{
		//Delay(0.1);
		ProcessSystemEvents ();
	}
	while(GetOutQLen(SER_PRINT) > 0);
 
 	//CloseCom(SER_PRINT);
 	//com_port_open_flag = 0;
 	
 	return(0);
}


// ****************************************************************************

int get_range(char* unit_serial,int* error,char* error_str,int* range_id)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR,RANGE_ID FROM SERIAL_GET_RANGE ('%s')",unit_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    resCode = resCode | DBBindColInt (hstmt, 3, range_id, &stat);
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}

int get_brd_type(char* brd_serial,int* error,char* error_str,int* brd_type)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR,BRD_TYPE FROM SERIAL_GET_BRD_TYPE ('%s')",brd_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    resCode = resCode | DBBindColInt (hstmt, 3, brd_type, &stat);
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}

int cba_write(int range_id,char* main_brd_serial,int* error,char* error_str,char* unit_serial)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR,SERNUM FROM SERIAL_CBA (%d,'%s')",range_id,main_brd_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    resCode = resCode | DBBindColChar (hstmt, 3, 50, unit_serial, &stat, "");
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}


int assy_write(int range_id,char* main_brd_serial,char* handset_brd_serial,char* lamp_brd_serial,int* error,char* error_str,char* unit_serial)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR,SERNUM FROM SERIAL_ASSY (%d,'%s','%s','%s')",range_id,main_brd_serial,handset_brd_serial,lamp_brd_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    resCode = resCode | DBBindColChar (hstmt, 3, 50, unit_serial, &stat, "");
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}

int sts_write(char* unit_serial,char* sts_brd_serial,int* error,char* error_str)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR FROM SERIAL_STS ('%s','%s')",unit_serial,sts_brd_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}

int print_write(char* unit_serial,int* error,char* error_str)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];

	
	resCode=0;
    sprintf(sql_mess,"SELECT ERROR,ERROR_STR FROM SERIAL_PRINT ('%s')",unit_serial);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, error, &stat);
    resCode = resCode | DBBindColChar (hstmt, 2, 50, error_str, &stat, "");
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);
	
	return(0);
}

int get_pcb_serial_start(int* ser_start)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];
	int local_serial_start = 0;

					//SELECT GEN_ID(PCB_SERIAL_REF_001,0) FROM RDB$DATABASE
					//ALTER SEQUENCE PCB_SERIAL_REF_001 RESTART WITH 1500

	resCode=0;
    sprintf(sql_mess,"SELECT GEN_ID(PWB_KEY_INDEX_1,0) FROM RDB$DATABASE");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, &local_serial_start, &stat);
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);

	local_serial_start++;
	
	*ser_start = local_serial_start;

	return(0);
}

int get_pcb_serial_next(int* ser_start)
{
    int stat,hstmt,ret;
    int resCode = 0;
    char sql_mess[1000];
	int local_serial_start = 0;

					//SELECT GEN_ID(PCB_SERIAL_REF_001,0) FROM RDB$DATABASE
					//ALTER SEQUENCE PCB_SERIAL_REF_001 RESTART WITH 1500

	resCode=0;
    sprintf(sql_mess,"SELECT GEN_ID(PWB_KEY_INDEX_1,1) FROM RDB$DATABASE");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (hstmt <=0) return(-1);
    resCode = resCode | DBBindColInt (hstmt, 1, &local_serial_start, &stat);
    
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);
	if (resCode != 0) return(-5);

	local_serial_start++;
	
	*ser_start = local_serial_start;

	return(0);
}

// *****************************************************************************************

int prnt_test(int range,int number1,char* text,int speed_offset,int x_offset,int y_offset)
{
 	char data[100];
 	char mac_bar[20] = {""};
 
   	if (com_port_open_flag == 0)
 	{
 		OpenComConfig (SER_PRINT, "", 115200, 2, 8, 1, 512, 512);
 		//OpenComConfig (SER_PRINT, "", 9600, 2, 8, 1, 512, 512);
 		com_port_open_flag = 1;
 	}

	//**********
 
 
 
 
	strcpy(data,"^XA");		// start of Label & Origin(across & down)
	ComWrt(SER_PRINT,data,strlen(data));

	strcpy(data,"^MUd,300,300");		// units for format
	ComWrt(SER_PRINT,data,strlen(data));

	// SET DEFAULT BARCODE PARAMETERS
	strcpy(data,"^BY2,4,100");
	ComWrt(SER_PRINT,data,strlen(data));		// set defaults All barcodes
	// END SET DEFAULT BARCODE PARAMETERS

	sprintf(data,"^XB");		//was 7
	ComWrt(SER_PRINT,data,strlen(data));

	//sprintf(data,"^PR2,2,2");		//was 7
	//ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^LT%d",(y_offset + speed_offset));		//was 7
	//sprintf(data,"^LT%d",(y_offset));		//was 7
	ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^LS%d",x_offset);		//was 7
	ComWrt(SER_PRINT,data,strlen(data));


	//sprintf(data,"^LT%d",-7);		//was 7
	//ComWrt(SER_PRINT,data,strlen(data));

	//sprintf(data,"^LS%d",11);		//was 7
	//ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^LH%d,%d",X_OFF,Y_OFF);		//was 7
	ComWrt(SER_PRINT,data,strlen(data));

	//*********
	
	/*
	
	// OUTLINE BOX
	sprintf(data,"^FO75,0");		// field origin for box
	ComWrt(SER_PRINT,data,strlen(data));	
	
	
	sprintf(data,"^GB75,75,1^FS"); //
	ComWrt(SER_PRINT,data,strlen(data));
	
	*/
	
	//*********
	
	// BARCODE
	//sprintf(mac_bar,"B%06d%06d01",range,number1);
	//sprintf(mac_bar,"00%06d%06d01",range,number1);
	sprintf(mac_bar,"00%05d%09d",range,number1);

	sprintf(data,"^FO100,7");		// field origin for barcode (x,y) 
	ComWrt(SER_PRINT,data,strlen(data));	

	//strcpy(data,"^BXN,8,200,14,14"); 		// setup DATA MATRIX Orientation-N,Height-70
	strcpy(data,"^BXN,3,200,14,14"); 		// setup DATA MATRIX Orientation-N,Height-70
	//strcpy(data,"^BXN,2,200,14,14"); 		// setup DATA MATRIX Orientation-N,Height-70
	ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^FD%s^FS",mac_bar); //
	ComWrt(SER_PRINT,data,strlen(data));

	//*********
	
	// TEXT FOR LABEL
	//sprintf(data,"^FO93,56");		// origin for text
	sprintf(data,"^FO82,10");		// origin for text
	ComWrt(SER_PRINT,data,strlen(data));	

	//sprintf(data,"^A0N,16,16");//			// font select and scale
	sprintf(data,"^A0R,16,16");//			// font select and scale	
	ComWrt(SER_PRINT,data,strlen(data));	

	sprintf(data,"^FD%s^FS",text);//
	ComWrt(SER_PRINT,data,strlen(data));	

	//*********

	strcpy(data,"^XZ");				// end of Label
	ComWrt(SER_PRINT,data,strlen(data));
							  
							  
	do
	{
		ProcessSystemEvents ();
	}
	while(GetOutQLen(SER_PRINT) > 0);
 
 	
 	return(0);
}


int prnt_test_600(int range,int number1,char* text)
{
 	char data[100];
 	char mac_bar[20] = {""};
 	int ret = 0;
 
   	if (com_port_open_flag == 0)
 	{
 		ret = OpenComConfig (SER_PRINT, "", 9600, 2, 8, 1, 512, 512);
 		com_port_open_flag = 1;
 	}

	//**********
 
	strcpy(data,"^XA");		// start of Label & Origin(across & down)
	ComWrt(SER_PRINT,data,strlen(data));

	strcpy(data,"^MUd,300,300");		// units for format
	ComWrt(SER_PRINT,data,strlen(data));

	// SET DEFAULT BARCODE PARAMETERS
	strcpy(data,"^BY2,4,100");
	ComWrt(SER_PRINT,data,strlen(data));		// set defaults All barcodes
	// END SET DEFAULT BARCODE PARAMETERS

	sprintf(data,"^LH%d,%d",X_OFF,Y_OFF);		//was 7
	ComWrt(SER_PRINT,data,strlen(data));

	//*********
	
	// OUTLINE BOX
	sprintf(data,"^FO50,50");		// field origin for box
	ComWrt(SER_PRINT,data,strlen(data));	
	
	
	sprintf(data,"^GB150,150,2^FS"); //
	ComWrt(SER_PRINT,data,strlen(data));
	
	
	//*********
	
	// BARCODE
	sprintf(mac_bar,"B%06d%06d01",range,number1);

	sprintf(data,"^FO83,65");		// field origin for barcode (x,y) 
	ComWrt(SER_PRINT,data,strlen(data));	

	//strcpy(data,"^BXN,8,200,14,14"); 		// setup DATA MATRIX Orientation-N,Height-70
	strcpy(data,"^BXN,6,200,14,14"); 		// setup DATA MATRIX Orientation-N,Height-70
	ComWrt(SER_PRINT,data,strlen(data));

	sprintf(data,"^FD%s^FS",mac_bar); //
	ComWrt(SER_PRINT,data,strlen(data));

	//*********
	
	// TEXT FOR LABEL
	sprintf(data,"^FO87,160");		// origin for text
	ComWrt(SER_PRINT,data,strlen(data));	

	//sprintf(data,"^A0,N,36,36");//			// font select and scale
	sprintf(data,"^A0,N,36,36");//			// font select and scale
	ComWrt(SER_PRINT,data,strlen(data));	

	sprintf(data,"^FD%s^FS",text);//
	ComWrt(SER_PRINT,data,strlen(data));	

	//*********

	strcpy(data,"^XZ");				// end of Label
	ComWrt(SER_PRINT,data,strlen(data));
							  
							  
	do
	{
		ProcessSystemEvents ();
	}
	while(GetOutQLen(SER_PRINT) > 0);
 
 	
 	return(0);
}

int prnt_status(int* error_all,int* paper_out,int* pause,
					int* buffer_full,int* head_up,int* ribbon_out,int* num_in_buffer)
{
	char data[200] = {""};
	char data1[200] = {""};
	char data2[200] = {""};
	char data3[200] = {""};
	int temp = 0;
	int len = 0;
	char stemp[10] = {""};
	int count = 0;
	int line_count = 0;
	
	int loc_paper_out = 0;
	int loc_pause = 0;
	int loc_num_in_buffer = 0;
	int loc_buffer_full = 0;
	
	int loc_head_up = 0;
	int loc_ribbon_out = 0;

	int loc_all = 0;
	
	double t1,t2;
	
 	if (com_port_open_flag == 0)
 	{
 		temp = OpenComConfig (SER_PRINT, "", 115200, 2, 8, 1, 512, 512);
 		com_port_open_flag = 1;
 	}
	
	sprintf(data,"~HS");//			// font select and scale
	ComWrt(SER_PRINT,data,strlen(data));	

	do
	{
		//Delay(0.1);
		ProcessSystemEvents ();
	}
	while(GetOutQLen(SER_PRINT) > 0);
 
    data[0] = 0;
 	
 	t1 = Timer();
 	
 	count = 0;
	for(;;)
	{
		t2 = Timer();
		if ((t2 - t1) > 5) break;
 		len = GetInQLen(SER_PRINT);
 		if (len > 0)
 		{
			temp = ComRdByte (SER_PRINT);
 			if (temp > 0)
 			{
 				stemp[0] = temp;
 				stemp[1] = 0;
 				if (count < 100) 
 				{
 					if (isprint(temp))
 					{
 						strcat(data,stemp);
 						count++;
 					}
 				}
 				if (temp == '\n')
 				{
 					//printf("[%s]\n",data);
 					if (line_count == 0) strcpy(data1,data);
 					if (line_count == 1) strcpy(data2,data);
 					if (line_count == 2) strcpy(data3,data);
 					line_count++;
 					count = 0;
 					data[0] = 0;
 					if (line_count == 3) break;
 				}
 			}
 		}
 	}

 
 	//CloseCom(SER_PRINT);
 	//com_port_open_flag = 0;

	if (line_count != 3) return(-1);	

	len = sscanf(data1,"%d,%d,%d,%d,%d,%d",&temp,&loc_paper_out,&loc_pause,&temp,&loc_num_in_buffer,&loc_buffer_full);
	if (len != 6) return(-2);

	len = sscanf(data2,"%d,%d,%d,%d",&temp,&temp,&loc_head_up,&loc_ribbon_out);
	if (len != 4) return(-2);

	loc_all = loc_paper_out | loc_pause | loc_buffer_full | loc_head_up | loc_ribbon_out;

	/*
	printf("loc_paper_out   = %d\n",loc_paper_out);
	printf("loc_pause       = %d\n",loc_pause);
	printf("loc_buffer_full = %d\n",loc_buffer_full);
	printf("loc_head_up     = %d\n",loc_head_up);
	printf("loc_ribbon_out  = %d\n",loc_ribbon_out);
	printf("loc_all         = %d\n",loc_all);
	*/
	if (paper_out != NULL) *paper_out = loc_paper_out;
	if (pause != NULL) *pause = loc_pause;
	if (buffer_full != NULL) *buffer_full = loc_buffer_full;
	if (head_up != NULL) *head_up = loc_head_up;
	if (ribbon_out != NULL) *ribbon_out = loc_ribbon_out;
	if (error_all != NULL) *error_all = loc_all;

	if (num_in_buffer != NULL) *num_in_buffer = loc_num_in_buffer;
	return(0);
}

int read_write_int(int force_write,char* name,int* data,int default_val,char* comment)
{
	char file_path[300] = {""};
	char file_name[300] = {""};
	char stemp[500] = {""};
	int res;
	int handle;
	int c=0;
	
	
	sprintf(file_path,"%s\\%s.txt",config_path,name);
	res = GetFirstFile (file_path, 1, 0, 0, 0, 0, 0,file_name);
	if ((res != 0)||(force_write != 0))
	{
		handle = OpenFile (file_path, VAL_WRITE_ONLY, VAL_TRUNCATE, VAL_ASCII);
		sprintf(stemp,"# %s",comment);
		WriteLine (handle, stemp, strlen(stemp));
		sprintf(stemp,"%d",default_val);
		WriteLine (handle, stemp, strlen(stemp));
		CloseFile(handle);
		
		*data = default_val;
		return(0);
	}
	
	stemp[0] = 0;
	handle = OpenFile (file_path, VAL_READ_ONLY, VAL_OPEN_AS_IS, VAL_ASCII);
	
	res = ReadLine (handle, stemp, 60);
	while(res > 0)
	{
		if (stemp[0] != '#') break;
		c++;
		if (c == 10) break;
		res = ReadLine (handle, stemp, 60);
	}
	CloseFile(handle);
	
	if (c==10)
	{
		*data = default_val;
		return(-1);
	}
	
	sscanf(stemp,"%d",data);
	
	return(0);
}


// *****************************************************************************

int record_print_range(int pcb_range,int serial_start,int serial_last,int month,int day)
{
    //int ret = 0;
    int resCode = 0;
    //int hstmt;
    //int stat;
    char sql_mess[10000];
	
	char ser_start_str[100] = {""};
	char ser_last_str[100] = {""};
	char lot_str[100] = {""};
	
	sprintf(ser_start_str,"00%05d%09d",pcb_range,serial_start);
	sprintf(ser_last_str,"00%05d%09d",pcb_range,serial_last);

	sprintf(lot_str,"%02d/%02d",day,month);

	
	sprintf(sql_mess,"INSERT INTO N_PWB_KEY_BATCH ( REF,COMPUTER_NAME,REVISION,LATEST,ACTIVE,NAME1,NAME2,NAME3,VAL1,VAL2) \
                VALUES ( %d,'%s',%d,%d,%d,'%s','%s','%s',%d,%d)",
                0,g_nova_computer_name,1,1,1,ser_start_str,ser_last_str,lot_str,month,day);
    	resCode = DBImmediateSQL(db_hdbc, sql_mess);
    	if (resCode != 0) return(-2);	
	
	return(0);
}
