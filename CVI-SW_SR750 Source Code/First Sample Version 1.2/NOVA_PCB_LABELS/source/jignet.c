#include "cvi_db.h"
#include <formatio.h>
#include <utility.h>
#include <ansi_c.h>
#include "jignet.h"

#ifndef YES
	#define YES 1
#endif

char net_log_file_path[] = {"\\\\cam-serv2\\shares\\log"};
int log_use_network = 0;

int db_hdbc;

int db_get_handle(void);
int db_open_connection(void);
int db_close_connection(void);

int get_mac_continue_permission(int p_num,int s_num, char* mac_num);
int write_mac_link(int p_num,int s_num, char* mac_num);
int get_mac_given_serial(int p_num,int s_num, char* mac_num);
int get_product_info_from_pcode(char* pcode,int* p_num,char* family_name,char* model_name,char* origin_name,char* destination,char* oracle_code);
int get_product_info_from_pnum(int p_num,char* family_name,char* model_name,char* origin_name,char* destination,char* oracle_code);
int get_control_info_from_pnum(int p_num,int* has_mac_num,int* is_sub_model,int* has_sub_model);
int get_job_info(int p_num,int s_num,char* job_name,int* scrap,int* year,int* month,int* day,int* stage_prof);

int get_count_of_binds(int p_num,int sp_num,int* count);
int get_bind_continue_permission(char* comp_type,int a_num,int sa_num,int p_num,int sp_num);
int get_bind_component(char* comp_type,int p_num,int sp_num,int* a_num,int* sa_num);
int get_bind_parent(int a_num,int sa_num,int* p_num,int* sp_num);
int write_bind_link(int a_num,int sa_num,int p_num,int sp_num,int operator,int responsible);

int get_clock_data(int clock_id,int* clock_ref,char* full_name,char* filepath);
int get_clock_permission(int clock_ref,char* ts_num,int* rights);

int check_if_completed(int p_num,int s_num);
int check_if_logged(int p_num,int s_num,char* key);
int read_logged_val(int p_num,int s_num,char* key,char* val);
int read_serial_from_logged_val(int p_num,int* s_num,char* key,char* val);

int gen_instance_id(char* instance_id);
int save_data_item(
		char* loc_instance_id,
		int loc_prod_num,
		int loc_serial,
		char* loc_computer_name,
		char* loc_ts_front,
		char* loc_ts_page,
		int loc_jig_id,
		int loc_log_data_mode,
		int loc_id_ref_operator,
		int loc_id_ref_responsible,
		char* loc_model_name,
		char* loc_operation,
		char* loc_current_seq,
		char* loc_date_str,
		char* loc_time_str,
		char* loc_log_data_key,
		char* loc_log_data_unit,
		char* loc_log_data_value,
		int loc_fail_flag,
		int loc_system_flag
				);
int make_log_dir_struct(int jig_id,char* model_name,char* operation,int loc_prod_num,int loc_serial);
int create_log_if_none(int p_number);

//**************************************************************************************************************
// old recnet.c stuff

int show_db_error_string(int error,char* e_mess);
int create_xp_if_none(int year,int month);
int create_xc_if_none(int year,int month);
int create_pro_if_none(int p_number);
int get_db_stage_validity(int s_num,int p_num,char* jig_type_in1,char* jig_type_in2,char* jig_type_in3,char* jig_type_out1,char* jig_type_out2,char* jig_type_out3);
int get_db_continue_permission(char* jig_type,int s_num,int p_num,int once_or_many);
int write_db_result(char* jig_type,int s_num,int p_num,int once_or_many,int operation,int operator,int responsible);
int write_db_extra_fail(int manual,char* mode_name,char* sub_name,char* meas_name,
                            int mode_num,int sub_num,int meas_num,
                            double hi_spec,double lo_spec,double reading);

int scrap_serial_number(int p_num,int s_num,int op_id);

int check_module_version(char* app_name,int app_ver);
//int write_db_reprint(int s_num,int p_num,int operator);


int com_pnum;
int com_snum;
int com_stage;
int com_stage_index;
char com_stage_text[100];
char com_table[100];

char stage_names[][20] = {
							"CREATE",
							"PC_END",
							"SASSY",
							"ASSY1",
							"ASSY2",
							"ASSY3",
							"ASSY4",
							"ASSY5",
							"ASSY_END",
							"AINSUL",
							"TEST1",
							"TEST2",
							"TEST3",
							"TEST4",
							"TEST5",
							"TEST_END",
							"APP",
							"INSUL",
							"QC",
							"QC_END",
							"SHIP",
							"PACK",
							"SCRAP",
							""
							};

int stage_nums[100] = {
							1000,	// CREATE
							2500,	// PC_END
							2600,	// SASSY
							3000,	// ASSY1
							3100,	// ASSY2
							3200,	// ASSY3
							3300,	// ASSY4
							3400,	// ASSY5
							3500,	// ASSY_END
							3700,	// AINSUL
							4000,	// TEST1
							4100,	// TEST2
							4200,	// TEST3
							4300,	// TEST4
							4400,	// TEST5
							4500,	// TEST_END
							5000,	// APP
							6000,	// INSUL
							7000,	// QC
							7200,	// QC_END
							8000,	// SHIP
							9000,	// PACK
							-1000	// SCRAP
							};

//**************************************************************************************************************

int db_get_handle()
{
	return(db_hdbc);
}

int db_open_connection()
{
    int resCode;

    db_hdbc = DBNewConnection();
    if (db_hdbc <= 0) return(-1);

    DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_STRING,"DSN=CAMDB;MODIFYSQL=1;UID=SYSDBA;PWD=odin1969" );
    DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_COMMAND_TIMEOUT, 10);
    DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_TIMEOUT, 10);
    DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_ISOLATION_LEVEL, DB_ISOLATION_LEVEL_READ_COMMITTED);

    resCode = DBOpenConnection(db_hdbc);
    if (resCode != DB_SUCCESS)
    {
        DBSetConnectionAttribute (db_hdbc, ATTR_DB_CONN_CONNECTION_STRING,"DSN=CAMDB;MODIFYSQL=1;UID=SYSDBA;PWD=masterkey" );
        resCode = DBOpenConnection(db_hdbc);
        if (resCode != DB_SUCCESS)
        {
            return(resCode);
        }
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

	log_use_network = YES;

    return(0);
}

int db_close_connection()
{
    int resCode;

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

    return(DB_SUCCESS);
}

int create_mac_if_none()
{
    int hstmt;
    int resCode;
    int stat;
    int ret;
    char match[50];
    char sql_mess[5000] = {""};
    //char sql_tmp[1000];

    char table[200] = {""};

    sprintf(match,"MAC_SER_LINK");

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
        sprintf(sql_mess,"CREATE TABLE MAC_SER_LINK ( REF INT DEFAULT 0 , P_NUMBER INT DEFAULT 0 , \
                    S_NUMBER INT DEFAULT 0, MAC_NUMBER VARCHAR(15) DEFAULT '', MAC_DATE DATE DEFAULT 'NOW', MAC_TIME TIME default 'NOW')");
        resCode = DBImmediateSQL(db_hdbc, sql_mess);

    }
    return(0);
}

int get_mac_continue_permission(int p_num,int s_num, char* mac_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    int loc_p_num,loc_s_num;
    char loc_mac_num[30] = {""};

    //create_mac_if_none();
    if ((p_num == 0)||(s_num == 0))
    {
        return(DB_ER_DATA);
    }

    // range check
    sprintf(sql_mess,"SELECT * FROM MAC_INDEX WHERE (MAC_START <= '%s' AND MAC_NEXT > '%s') AND (MAC_START <= '%s' AND MAC_END > '%s')" ,mac_num,mac_num,mac_num,mac_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_MAC_RANGE);

    // check if this unit (ser_num,p_num) already has a mac_num bound to it
    sprintf(sql_mess,"SELECT MAC_NUMBER FROM MAC_SER_LINK WHERE P_NUMBER = %d AND S_NUMBER = %d AND VALID_FLAG = 1",p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar(hstmt,1,20,loc_mac_num,&stat,"");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret == 0)
    {
        if (strcmp(mac_num,loc_mac_num) != 0) return(DB_ER_UNIT_ASSIGNED); // this unit is already bound to a differnt mac address
    }

    // check if the mac address is already used
    sprintf(sql_mess,"SELECT S_NUMBER, P_NUMBER FROM MAC_SER_LINK WHERE MAC_NUMBER = '%s' AND VALID_FLAG = 1",mac_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &loc_s_num, &stat);
    resCode = DBBindColInt (hstmt, 2, &loc_p_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_NONE);   // not used already

    // if already used then check if this is the board that it is used on (retest)

    if ((p_num == loc_p_num)&&(s_num == loc_s_num))
    {
        //printf("good 2\n");
        return(DB_ER_NONE); // retest
    }

    // not new or a retest so fail
    return(DB_ER_MAC_USED);
}

int write_mac_link(int p_num,int s_num, char* mac_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    int new_ref=0;

    ret = get_mac_continue_permission(p_num,s_num,mac_num);

    if (ret != DB_ER_NONE) return(ret);

    // check if the mac address is already used
    sprintf(sql_mess,"SELECT * FROM MAC_SER_LINK WHERE MAC_NUMBER = '%s' AND P_NUMBER = %d AND S_NUMBER = %d AND VALID_FLAG = 1",mac_num,p_num,s_num);


    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret == 0) return(DB_ER_NONE);   // retest so don't write record again


    sprintf(sql_mess,"SELECT MAX(REF) FROM MAC_SER_LINK");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &new_ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    new_ref++;

    //create new record
    sprintf(sql_mess,"INSERT INTO MAC_SER_LINK ( REF, P_NUMBER, S_NUMBER, MAC_NUMBER ) \
                VALUES ( %d, %d, %d, '%s')",
                new_ref,p_num,s_num,mac_num);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // check if the mac address has writen correctly
    sprintf(sql_mess,"SELECT * FROM MAC_SER_LINK WHERE MAC_NUMBER = '%s' AND P_NUMBER = %d AND S_NUMBER = %d AND VALID_FLAG = 1",mac_num,p_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0) return(DB_ER_DB);   // not writen for some reason




    return(DB_ER_NONE);
}

int get_mac_given_serial(int p_num,int s_num, char* mac_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    char loc_mac_num[30] = {""};

    sprintf(sql_mess,"SELECT MAC_NUMBER FROM MAC_SER_LINK WHERE P_NUMBER = %d AND S_NUMBER = %d AND VALID_FLAG = 1",p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar(hstmt,1,20,loc_mac_num,&stat,"");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(-1);

    strcpy(mac_num,loc_mac_num);

    return(0);
}

int get_product_info_from_pcode(char* pcode,int* p_num,char* family_name,char* model_name,char* origin_name,char* destination,char* oracle_code)
{
    char sql_mess[1000] = {""};
    int ret=0;
	int resCode=0;
    int hstmt;
    int stat;

    if(strcmp(pcode,"000") == 0)
    {
        if (p_num != NULL) *p_num = 0;
        if (family_name != NULL) strcpy(family_name,"");
        if (model_name != NULL) strcpy(model_name,"");
        if (origin_name != NULL) strcpy(origin_name,"");
        if (destination != NULL) strcpy(destination,"");
        if (oracle_code != NULL) strcpy(oracle_code,"");
        return(DB_ER_NONE);
    }

    sprintf(sql_mess,"SELECT P_NUMBER, FAMILY_NAME, MODEL_NAME, ORIGIN_NAME, DEST_NAME,ORACLE_CODE FROM MODEL_INDEX WHERE P_CODE = '%s'",pcode);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (p_num != NULL) resCode = resCode | DBBindColInt  (hstmt, 1, p_num, &stat);     // global variable
    if (family_name != NULL) resCode = resCode | DBBindColChar (hstmt, 2, 8, family_name, &stat, "");
    if (model_name != NULL) resCode = resCode | DBBindColChar (hstmt, 3, 16, model_name, &stat, "");
    if (origin_name != NULL) resCode = resCode | DBBindColChar (hstmt, 4, 8, origin_name, &stat, "");
    if (destination != NULL) resCode = resCode | DBBindColChar (hstmt, 5, 4, destination, &stat, "");
	if (oracle_code != NULL) resCode = resCode | DBBindColChar (hstmt, 6, 30, oracle_code, &stat, "");

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0) return(DB_ER_BADNAME);

    return(DB_ER_NONE);
}

int get_product_info_from_pnum(int p_num,char* family_name,char* model_name,char* origin_name,char* destination,char* oracle_code)
{
    char sql_mess[1000] = {""};
    int ret=0;
	int resCode=0;
    int hstmt;
    int stat;

	if (p_num <=0)
    {
        if (family_name != NULL) strcpy(family_name,"");
        if (model_name != NULL) strcpy(model_name,"");
        if (origin_name != NULL) strcpy(origin_name,"");
        if (destination != NULL) strcpy(destination,"");
        if (oracle_code != NULL) strcpy(oracle_code,"");
        return(DB_ER_NONE);
    }

    sprintf(sql_mess,"SELECT FAMILY_NAME, MODEL_NAME, ORIGIN_NAME, DEST_NAME, ORACLE_CODE FROM MODEL_INDEX WHERE P_NUMBER = %d",p_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (family_name != NULL) resCode = resCode | DBBindColChar (hstmt, 1, 8, family_name, &stat, "");
    if (model_name != NULL) resCode = resCode | DBBindColChar (hstmt, 2, 16, model_name, &stat, "");
    if (origin_name != NULL) resCode = resCode | DBBindColChar (hstmt, 3, 8, origin_name, &stat, "");
    if (destination != NULL) resCode = resCode | DBBindColChar (hstmt, 4, 4, destination, &stat, "");
    if (oracle_code != NULL) resCode = resCode | DBBindColChar (hstmt, 5, 30, oracle_code, &stat, "");

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0) return(DB_ER_BADNAME);

    return(DB_ER_NONE);
}


int get_control_info_from_pnum(int p_num,int* has_mac_num,int* is_sub_model,int* has_sub_model)
{
    char sql_mess[1000] = {""};
    int ret=0;
	int resCode=0;
    int hstmt;
    int stat;

	if (p_num <=0) return(DB_ER_NONE);

    sprintf(sql_mess,"SELECT LOCAL_MODEL, USE_MAC, USE_LOCAL FROM MODEL_INDEX WHERE P_NUMBER = %d",p_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    if (is_sub_model != NULL) resCode = resCode | DBBindColInt  (hstmt, 1, is_sub_model, &stat);
    if (has_mac_num != NULL) resCode = resCode | DBBindColInt  (hstmt, 2, has_mac_num, &stat);
    if (has_sub_model != NULL) resCode = resCode | DBBindColInt  (hstmt, 3, has_sub_model, &stat);

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0) return(DB_ER_BADNAME);

    return(DB_ER_NONE);
}


int get_job_info(int p_num,int s_num,char* job_name,int* scrap,int* year,int* month,int* day,int* stage_prof)
{
	char sql_mess[1000] = {""};
    int ret,resCode=0;
    int hstmt;
    int stat;

	int loc_year=0;
	int loc_month = 0;
	int loc_day = 0;

	char p_date[50] = {""};

    sprintf(sql_mess,"SELECT JOB_NO, P_DATE, STAGE_PROFILE ,SCRAP_REPLACE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
	if (job_name != NULL)   resCode = resCode | DBBindColChar (hstmt, 1, 10, job_name, &stat, "");
	if (p_date != NULL)     resCode = resCode | DBBindColChar (hstmt, 2, 30, p_date, &stat, "yyyy mm dd");
    if (stage_prof != NULL) resCode = resCode | DBBindColInt  (hstmt, 3, stage_prof, &stat);
	if (scrap != NULL)      resCode = resCode | DBBindColInt  (hstmt, 4, scrap, &stat);

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0) return(DB_ER_BADNAME);

	sscanf(p_date,"%d %d %d",&loc_year,&loc_month,&loc_day);

	if (year != NULL) *year = loc_year;
	if (month != NULL) *month = loc_month;
	if (day != NULL) *day = loc_day;

	return(0);
}

int get_count_of_binds(int p_num,int sp_num,int* count)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;

	*count = 0;

	sprintf(sql_mess,"SELECT COUNT(*) FROM AP_LINK WHERE P_NUM = %d AND SP_NUM = %d AND VALID_FLAG = 1",p_num,sp_num);
	hstmt = DBActivateSQL (db_hdbc, sql_mess);
    DBBindColInt  (hstmt, 1, count, &stat);

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-1);

	return(0);
}

int get_bind_continue_permission(char* comp_type,int a_num,int sa_num,int p_num,int sp_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;

    int loc_a_num=0;
    int loc_sa_num=0;
    int loc_p_num=0;
    int loc_sp_num=0;

    if (a_num <= 0) return(DB_ER_DB);
    if (sa_num <= 0) return(DB_ER_DB);
    if (p_num <= 0) return(DB_ER_DB);
    if (sp_num <= 0) return(DB_ER_DB);

	// comp_type is the type of the assy component and matches the FAMILY_NAME field in MODEL_INDEX
	// this is the same as the Family Name field accessed from model_edit.exe
	// Currently 2 types are used "PSU" for Jazz power supplies and "OHB" for OHBs. This is introduced for Alacam.

    //given a_num and sa_num check if already used. If are already used ensure
    // already used with existing p_num and sp_num
    sprintf(sql_mess,"SELECT a.P_NUM, a.SP_NUM FROM AP_LINK a,MODEL_INDEX b WHERE a.A_NUM = b.P_NUMBER AND b.FAMILY_NAME = '%s' AND a.A_NUM = %d AND a.SA_NUM = %d AND a.VALID_FLAG = 1",comp_type,a_num,sa_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    DBBindColInt  (hstmt, 1, &loc_p_num, &stat);
    DBBindColInt  (hstmt, 2, &loc_sp_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    // if ret != 0 then no record found so can continue with binding operation
    if (ret == 0)
    {
        // record found so check if retest
        if ((p_num != loc_p_num)||(sp_num != loc_sp_num)) return(DB_ER_DB);
    }


    //given p_num and sp_num check if already used. If are already used ensure
    // already used with existing a_num and sa_num
    sprintf(sql_mess,"SELECT a.A_NUM, a.SA_NUM FROM AP_LINK a,MODEL_INDEX b WHERE  a.A_NUM = b.P_NUMBER AND b.FAMILY_NAME = '%s' AND a.P_NUM = %d AND a.SP_NUM = %d AND a.VALID_FLAG = 1",comp_type,p_num,sp_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    DBBindColInt  (hstmt, 1, &loc_a_num, &stat);
    DBBindColInt  (hstmt, 2, &loc_sa_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    // if ret != 0 then no record found so can continue with binding operation
    if (ret == 0)
    {
        // record found so check if retest
        if ((a_num != loc_a_num)||(sa_num != loc_sa_num)) return(DB_ER_DB);
    }

    return(DB_ER_NONE);
}

int get_bind_component(char* comp_type,int p_num,int sp_num,int* a_num,int* sa_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;

    int loc_a_num=0;
    int loc_sa_num=0;

    if (p_num <= 0) return(DB_ER_DB);
    if (sp_num <= 0) return(DB_ER_DB);

	resCode = 0;

	// comp_type is the type of the assy component and matches the FAMILY_NAME field in MODEL_INDEX
	// this is the same as the Family Name field accessed from model_edit.exe
	// Currently 2 types are used "PSU" for Jazz power supplies and "OHB" for OHBs. This is introduced for Alacam.

    //given a_num and sa_num check if already used. If are already used ensure
    // already used with existing p_num and sp_num
    //sprintf(sql_mess,"SELECT A_NUM, SA_NUM FROM AP_LINK WHERE P_NUM = %d AND SP_NUM = %d AND VALID_FLAG = 1",p_num,sp_num);
    sprintf(sql_mess,"SELECT a.A_NUM, a.SA_NUM FROM AP_LINK a,MODEL_INDEX b WHERE  a.A_NUM = b.P_NUMBER AND b.FAMILY_NAME = '%s' AND a.P_NUM = %d AND a.SP_NUM = %d AND a.VALID_FLAG = 1",comp_type,p_num,sp_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = resCode | DBBindColInt  (hstmt, 1, &loc_a_num, &stat);
    resCode = resCode | DBBindColInt  (hstmt, 2, &loc_sa_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);

	if (resCode != 0)
    {
        return(DB_ER_DB);
    }

    // if ret != 0 then no record found so can continue with binding operation
    if (ret != 0)
    {
        // no record found so fail but with differebt error code.
        return(DB_ER_DATA);
    }

	if (a_num != NULL) *a_num = loc_a_num;
	if (sa_num != NULL) *sa_num = loc_sa_num;

    return(DB_ER_NONE);
}

int get_bind_parent(int a_num,int sa_num,int* p_num,int* sp_num)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;

    int loc_p_num=0;
    int loc_sp_num=0;

    if (a_num <= 0) return(DB_ER_DB);
    if (sa_num <= 0) return(DB_ER_DB);

	resCode = 0;

	// comp_type is the type of the assy component and matches the FAMILY_NAME field in MODEL_INDEX 
	// this is the same as the Family Name field accessed from model_edit.exe
	// Currently 2 types are used "PSU" for Jazz power supplies and "OHB" for OHBs. This is introduced for Alacam.

    //given a_num and sa_num check if already used. If are already used ensure
    // already used with existing p_num and sp_num
    sprintf(sql_mess,"SELECT P_NUM, SP_NUM FROM AP_LINK WHERE A_NUM = %d AND SA_NUM = %d AND VALID_FLAG = 1",a_num,sa_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = resCode | DBBindColInt  (hstmt, 1, &loc_p_num, &stat);
    resCode = resCode | DBBindColInt  (hstmt, 2, &loc_sp_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);

	if (resCode != 0)
    {
        return(DB_ER_DB);
    }

    // if ret != 0 then no record found so can continue with binding operation
    if (ret != 0)
    {
        // no record found so fail but with differebt error code.
        return(DB_ER_DATA);
    }

	if (p_num != NULL) *p_num = loc_p_num;
	if (sp_num != NULL) *sp_num = loc_sp_num;

    return(DB_ER_NONE);
}

int write_bind_link(int a_num,int sa_num,int p_num,int sp_num,int operator,int responsible)
{
    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    int new_ref=0;

	char family_name[100] = {""};
	char model_name[100] = {""};
	char origin_name[100] = {""};
	char destination[100] = {""};
	char oracle_name[100] = {""};

	ret = get_product_info_from_pnum(a_num,family_name,model_name,origin_name,destination,oracle_name);
	if (ret != DB_ER_NONE) return(DB_ER_DB);

	if ((strcmp(family_name,"OHB") != 0)&&(strcmp(family_name,"PSU") != 0)) return(DB_ER_DB);

    ret = get_bind_continue_permission(family_name,a_num,sa_num,p_num,sp_num);
    if (ret != DB_ER_NONE) return(ret);

    // check if the combination is already used
    sprintf(sql_mess,"SELECT * FROM AP_LINK WHERE P_NUM = %d AND SP_NUM = %d AND A_NUM = %d AND SA_NUM = %d AND VALID_FLAG = 1",p_num,sp_num,a_num,sa_num);


    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret == 0) return(DB_ER_NONE);   // retest so don't write record again


    sprintf(sql_mess,"SELECT MAX(REF) FROM AP_LINK");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &new_ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    new_ref++;

    //create new record
    sprintf(sql_mess,"INSERT INTO AP_LINK ( REF, P_NUM, SP_NUM, A_NUM, SA_NUM, OPERATOR ) \
                VALUES ( %d, %d, %d, %d, %d, %d)",
                new_ref,p_num,sp_num,a_num,sa_num,operator);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // check if write successfull
    sprintf(sql_mess,"SELECT * FROM AP_LINK WHERE P_NUM = %d AND SP_NUM = %d AND A_NUM = %d AND SA_NUM = %d AND VALID_FLAG = 1",p_num,sp_num,a_num,sa_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);


    if (ret != 0) return(DB_ER_DB);   // not writen for some reason

    return(DB_ER_NONE);
}

// PERSONNEL STUFF

int get_clock_data(int clock_id,int* clock_ref,char* full_name,char* filepath)
{

    char network_people_pic_path[100] = {"\\\\cam-serv1\\pictures\\people_bitmaps"};
    char loc_people_pic_path[] = {"c:\\local_alchemy\\people_pics"};            // local copy of operator pics

    char filepath1[300] = {""};
    char filepath2[300] = {""};

    int ret=0;
    int hstmt;
    int stat;
    char sql_mess[10000];

    char first_name[20] = {""};
    char mid_name[20] = {""};
    char last_name[20] = {""};

    int pic_ref=0;

    int long fsize = 0;

    sprintf(sql_mess,"SELECT REF,FIRST_NAME, MID_NAME, LAST_NAME, PIC_REF FROM PERSONNEL_INDEX WHERE CLOCK_ID = %d AND ACT_STATUS = 1",clock_id);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);

    DBBindColInt  (hstmt, 1, clock_ref, &stat);
    DBBindColChar (hstmt, 2, 15, first_name, &stat, "");
    DBBindColChar (hstmt, 3, 15, mid_name, &stat, "");
    DBBindColChar (hstmt, 4, 15, last_name, &stat, "");
    DBBindColInt  (hstmt, 5, &pic_ref, &stat);

    ret = DBFetchNext (hstmt);
    DBDeactivateSQL (hstmt);

    if (ret != 0) return(-1);

    if (strlen(mid_name) == 0) sprintf(full_name,"%s %s",first_name,last_name);
    else sprintf(full_name,"%s %s %s",first_name,mid_name,last_name);


    if (pic_ref != 0)
    {
        sprintf(filepath1,"%s\\%05d_%02d.bmp",loc_people_pic_path,*clock_ref,pic_ref);

		//ret = rename(filepath1,filepath1);
		ret = GetFileInfo (filepath1, &fsize);
        if (ret == 0)   // file does not exist locally
        {
            sprintf(filepath2,"%s\\%05d_%02d.bmp",network_people_pic_path,*clock_ref,pic_ref);
            CopyFile (filepath2, filepath1);
        }
    }

    strcpy(filepath,filepath1);

    return(0);
}

int get_clock_permission(int clock_ref,char* ts_num,int* rights)
{
    int ret=1;
    int hstmt;
    int stat;
    char sql_mess[10000];

    int loc_rights = 0;

	*rights = 2;
	return(0);

    sprintf(sql_mess,"SELECT SKILL_LEVEL FROM WORK_LICENCE WHERE CLOCK_REF = %d AND TS_NUM = '%s' ORDER BY CHANGE_DATE,CHANGE_TIME DESC",clock_ref,ts_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);

    DBBindColInt  (hstmt, 1, &loc_rights, &stat);
    ret = DBFetchNext (hstmt);
    DBDeactivateSQL (hstmt);

    *rights = loc_rights;

    return(ret);
}

// *******************************************************************

int check_if_completed(int p_num,int s_num)
{
    int ret = 0;
    int resCode = 0;
    int hstmt;
    int stat;
    char sql_mess[10000];

	char p_date[50] = {""};
	int stage_profile = 0;
	int stage_count = 0;
	char y_str[50] = {""};
	char m_str[50] = {""};
	char table_str[50] = {""};
	//resCode = DBBindColChar (hstmt, 7, 50 , xp.op_date, &stat, "dd.mm.yyyy");

    sprintf(sql_mess,"SELECT P_DATE, STAGE_PROFILE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);

	resCode=0;
    resCode = resCode | DBBindColChar (hstmt, 1, 20, p_date, &stat, "yyyy mm");
    resCode = resCode | DBBindColInt  (hstmt, 2, &stage_profile, &stat);
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-1);
	if (ret != 0) return(-2);


    sprintf(sql_mess,"SELECT STAGE_COUNT FROM PROFILE_INDEX WHERE STAGE_PROFILE = %d",stage_profile);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);

	resCode=0;
    resCode = resCode | DBBindColInt  (hstmt, 1, &stage_count, &stat);
    ret = DBFetchNext (hstmt);
    resCode = resCode | DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-3);
	if (ret != 0) return(-4);


	ret = sscanf(p_date,"%s %s",y_str,m_str);
	if (ret != 2) return(-3);

	sprintf(table_str,"XP_%s_%s",y_str,m_str);

	sprintf(sql_mess,"SELECT * FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS'",table_str,p_num,s_num,stage_count);

	hstmt = DBActivateSQL (db_hdbc, sql_mess);
	resCode=0;
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-4);
	if (ret != 0) return(-5);

	return(0);
}

int check_if_logged(int p_num,int s_num,char* key)
{
    int ret = 0;
    int resCode = 0;
    int hstmt;
    int stat;
    char sql_mess[10000];

	char log_table[200] = {""};
	char table[200] = {""};

	// make log table name
	sprintf(log_table,"LOG_M%08d",p_num);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = resCode | DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,log_table) == 0)
        {
        	break;
        }
    }
    resCode = resCode | DBDeactivateSQL(hstmt);
	if (resCode != 0) return(-1);

	if (ret != DB_SUCCESS) return(-2);

	sprintf(sql_mess,"SELECT * FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND KEY_NAME = '%s' ",log_table,p_num,s_num,key);

	hstmt = DBActivateSQL (db_hdbc, sql_mess);
	resCode=0;
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-1);
	if (ret != 0) return(-2);

	return(0);
}

int read_logged_val(int p_num,int s_num,char* key,char* val)
{
    int ret = 0;
    int resCode = 0;
    int hstmt;
    int stat;
    char sql_mess[10000];

    char data[500] = {""};

	char log_table[200] = {""};
	char table[200] = {""};

	if (val != NULL) val[0] = 0;

	// make log table name
	sprintf(log_table,"LOG_M%08d",p_num);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = resCode | DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,log_table) == 0)
        {
        	break;
        }
    }
    resCode = resCode | DBDeactivateSQL(hstmt);
	if (resCode != 0) return(-1);

	if (ret != DB_SUCCESS) return(0);


	sprintf(sql_mess,"SELECT VAL FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND KEY_NAME = '%s' ORDER BY INSTANCE DESC",log_table,p_num,s_num,key);

	hstmt = DBActivateSQL (db_hdbc, sql_mess);
	resCode=0;
	resCode = resCode | DBBindColChar (hstmt, 1, 200, data, &stat, "");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-1);
	if (ret != 0) return(0);

	if (val != NULL) strcpy(val,data);

	return(0);
}

int read_serial_from_logged_val(int p_num,int* s_num,char* key,char* val)
{
    int ret = 0;
    int resCode = 0;
    int hstmt;
    int stat;
    char sql_mess[10000];

    int loc_s_num=0;

	char log_table[200] = {""};
	char table[200] = {""};

	if (s_num != NULL) *s_num = 0;

	// make log table name
	sprintf(log_table,"LOG_M%08d",p_num);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = resCode | DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,log_table) == 0)
        {
        	break;
        }
    }
    resCode = resCode | DBDeactivateSQL(hstmt);
	if (resCode != 0) return(-1);

	if (ret != DB_SUCCESS) return(0);

	sprintf(sql_mess,"SELECT S_NUMBER FROM %s WHERE P_NUMBER = %d AND VAL = '%s' AND KEY_NAME = '%s' ORDER BY INSTANCE DESC",log_table,p_num,val,key);

	hstmt = DBActivateSQL (db_hdbc, sql_mess);
	resCode=0;
	resCode = resCode | DBBindColInt  (hstmt, 1, &loc_s_num, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	if (resCode != 0) return(-1);
	if (ret != 0) return(0);

	if (s_num != NULL) *s_num = loc_s_num;

	return(0);
}


int gen_instance_id(char* instance_id)
{
	int year,month,day,hour,min,sec;
	char* ptr= NULL;
	int k=0;
	int i=0;
	int j=0;
	char stemp[100] = {""};

	// generate ref id
	GetSystemDate (&month, &day, &year);
	GetSystemTime (&hour, &min, &sec);

	ptr = getenv ("COMPUTERNAME");
	if (ptr != NULL) strcpy(stemp,ptr);

	k=0;
	j = strlen(stemp);
	for(i=0;i<j;i++)
	{
		k = k + stemp[i];
	}
	k = k&0xffff;

	if (instance_id != NULL) sprintf(instance_id,"Z%04d%02d%02d%02d%02d%02d%04X",year,month,day,hour,min,day,k);
	return(0);
}


int save_data_item(
		char* loc_instance_id,
		int loc_prod_num,
		int loc_serial,
		char* loc_computer_name,
		char* loc_ts_front,
		char* loc_ts_page,
		int loc_jig_id,
		int loc_log_data_mode,
		int loc_id_ref_operator,
		int loc_id_ref_responsible,
		char* loc_model_name,
		char* loc_operation,
		char* loc_current_seq,
		char* loc_date_str,
		char* loc_time_str,
		char* loc_log_data_key,
		char* loc_log_data_unit,
		char* loc_log_data_value,
		int loc_fail_flag,
		int loc_system_flag
				)
{
	char title[500] = {""};
	char data[1000] = {""};
	int handle,ret=0;

	char filepath[500] = {""};
	char filename[500] = {""};

	int day=0,month=0,year=0;
	char table[500] = {""};
	char sql_mess[5500] = {""};

	char loc_id_file_path[500] = {"c:\\C2"};

	//if (loc_jig_id > 0) sprintf(loc_id_file_path,"c:\\c2\\id%d",loc_jig_id);

	make_log_dir_struct(loc_jig_id,loc_model_name,loc_operation,loc_prod_num,loc_serial);

	sprintf(title,"%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
		"P_NUMBER",
		"S_NUMBER",
		"COMPUTER_NAME",
		"TS_FRONT",
		"TS_PAGE",
		"JIG_ID",
		"MODE",
		"ID_OPERATOR",
		"ID_RESPONSIBLE",
		"JIG_MODEL_NAME",
		"JIG_OPERATION",
		"JIG_SEQ",
		"DATE",
		"TIME",
		"KEY_NAME",
		"UNIT",
		"VALUE",
		"INSTANCE",
		"FAIL_FLAG",
		"SYSTEN_FLAG"
		);


	sprintf(data,"%08d,%07d,%s,%s,%s,%02d,%d,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d",
		loc_prod_num,
		loc_serial,
		loc_computer_name,
		loc_ts_front,
		loc_ts_page,
		loc_jig_id,
		loc_log_data_mode,
		loc_id_ref_operator,
		loc_id_ref_responsible,
		loc_model_name,
		loc_operation,
		loc_current_seq,
		loc_date_str,
		loc_time_str,
		loc_log_data_key,
		loc_log_data_unit,
		loc_log_data_value,
		loc_instance_id,
		loc_fail_flag,
		loc_system_flag
		);

		//sprintf(data,"%s,%d,%s,%s,%s",data_start,log_data_mode[i],log_data_key[i],log_data_unit[i],log_data_value[i]);

					//FAIL_FLAG INT DEFAULT 0 NOT NULL,
					//SYSTEM_FLAG INT DEFAULT 0 NOT NULL)",match);


	if (log_use_network == YES)
	{
		// write date based log file
		GetSystemDate (&month, &day, &year);
	    sprintf(table,"LOG_D%04d%02d",year,month);

		sprintf(sql_mess,"INSERT INTO %s ( \
				INSTANCE, \
				P_NUMBER, \
				S_NUMBER, \
				COMPUTER_NAME, \
				TS_FRONT, \
				TS_PAGE, \
				JIG_ID, \
				MODE, \
				ID_OPERATOR, \
				ID_RESPONSIBLE, \
				JIG_MODEL_NAME, \
				JIG_OPERATION, \
				JIG_SEQ, \
				OP_DATE, \
				OP_TIME, \
				KEY_NAME, \
				UNIT, \
				VAL, \
				FAIL_FLAG, \
				SYSTEM_FLAG \
				) VALUES ('%s',%d,%d,'%s','%s','%s',%d,%d,%d,%d,'%s','%s','%s','NOW','NOW','%s','%s','%s',%d,%d)",
				table,
				loc_instance_id,
				loc_prod_num,
				loc_serial,
				loc_computer_name,
				loc_ts_front,
				loc_ts_page,
				loc_jig_id,
				loc_log_data_mode,
				loc_id_ref_operator,
				loc_id_ref_responsible,
				loc_model_name,
				loc_operation,
				loc_current_seq,
				loc_log_data_key,
				loc_log_data_unit,
				loc_log_data_value,
				loc_fail_flag,
				loc_system_flag
				);
		ret = DBImmediateSQL(db_hdbc, sql_mess);

		// write model based log file
		sprintf(table,"LOG_M%08d",loc_prod_num);

		sprintf(sql_mess,"INSERT INTO %s ( \
				INSTANCE, \
				P_NUMBER, \
				S_NUMBER, \
				COMPUTER_NAME, \
				TS_FRONT, \
				TS_PAGE, \
				JIG_ID, \
				MODE, \
				ID_OPERATOR, \
				ID_RESPONSIBLE, \
				JIG_MODEL_NAME, \
				JIG_OPERATION, \
				JIG_SEQ, \
				OP_DATE, \
				OP_TIME, \
				KEY_NAME, \
				UNIT, \
				VAL, \
				FAIL_FLAG, \
				SYSTEM_FLAG \
				) VALUES ('%s',%d,%d,'%s','%s','%s',%d,%d,%d,%d,'%s','%s','%s','NOW','NOW','%s','%s','%s',%d,%d)",
				table,
				loc_instance_id,
				loc_prod_num,
				loc_serial,
				loc_computer_name,
				loc_ts_front,
				loc_ts_page,
				loc_jig_id,
				loc_log_data_mode,
				loc_id_ref_operator,
				loc_id_ref_responsible,
				loc_model_name,
				loc_operation,
				loc_current_seq,
				loc_log_data_key,
				loc_log_data_unit,
				loc_log_data_value,
				loc_fail_flag,
				loc_system_flag
				);
		if (loc_prod_num > 0) ret = DBImmediateSQL(db_hdbc, sql_mess);

		// write single log file table of all data
		sprintf(sql_mess,"INSERT INTO %s ( \
				INSTANCE, \
				P_NUMBER, \
				S_NUMBER, \
				COMPUTER_NAME, \
				TS_FRONT, \
				TS_PAGE, \
				JIG_ID, \
				MODE, \
				ID_OPERATOR, \
				ID_RESPONSIBLE, \
				JIG_MODEL_NAME, \
				JIG_OPERATION, \
				JIG_SEQ, \
				OP_DATE, \
				OP_TIME, \
				KEY_NAME, \
				UNIT, \
				VAL, \
				FAIL_FLAG, \
				SYSTEM_FLAG \
				) VALUES ('%s',%d,%d,'%s','%s','%s',%d,%d,%d,%d,'%s','%s','%s','NOW','NOW','%s','%s','%s',%d,%d)",
				"LOG_DATA",
				loc_instance_id,
				loc_prod_num,
				loc_serial,
				loc_computer_name,
				loc_ts_front,
				loc_ts_page,
				loc_jig_id,
				loc_log_data_mode,
				loc_id_ref_operator,
				loc_id_ref_responsible,
				loc_model_name,
				loc_operation,
				loc_current_seq,
				loc_log_data_key,
				loc_log_data_unit,
				loc_log_data_value,
				loc_fail_flag,
				loc_system_flag
				);
		ret = DBImmediateSQL(db_hdbc, sql_mess);

		// write single log file table of recent data
		sprintf(sql_mess,"INSERT INTO %s ( \
				INSTANCE, \
				P_NUMBER, \
				S_NUMBER, \
				COMPUTER_NAME, \
				TS_FRONT, \
				TS_PAGE, \
				JIG_ID, \
				MODE, \
				ID_OPERATOR, \
				ID_RESPONSIBLE, \
				JIG_MODEL_NAME, \
				JIG_OPERATION, \
				JIG_SEQ, \
				OP_DATE, \
				OP_TIME, \
				KEY_NAME, \
				UNIT, \
				VAL, \
				FAIL_FLAG, \
				SYSTEM_FLAG \
				) VALUES ('%s',%d,%d,'%s','%s','%s',%d,%d,%d,%d,'%s','%s','%s','NOW','NOW','%s','%s','%s',%d,%d)",
				"LOG_DATA_RECENT",
				loc_instance_id,
				loc_prod_num,
				loc_serial,
				loc_computer_name,
				loc_ts_front,
				loc_ts_page,
				loc_jig_id,
				loc_log_data_mode,
				loc_id_ref_operator,
				loc_id_ref_responsible,
				loc_model_name,
				loc_operation,
				loc_current_seq,
				loc_log_data_key,
				loc_log_data_unit,
				loc_log_data_value,
				loc_fail_flag,
				loc_system_flag
				);
		ret = DBImmediateSQL(db_hdbc, sql_mess);

		sprintf(filepath,"%s\\%s\\%s\\%s.csv",net_log_file_path,loc_model_name,loc_operation,loc_log_data_key);
		ret = GetFirstFile (filepath, 1, 0, 0, 0, 0, 0, filename);
		if (ret != 0)
		{
			handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
			WriteLine (handle, title, strlen(title));
			CloseFile(handle);
		}

		handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
		WriteLine (handle, data, strlen(data));
		CloseFile(handle);

		if ((loc_prod_num != 0)&&(loc_serial != 0))	// only do if non dummy tally card used
		{
			sprintf(filepath,"%s\\%08d\\%07d\\%s.csv",net_log_file_path,loc_prod_num,loc_serial,loc_log_data_key);
			ret = GetFirstFile (filepath, 1, 0, 0, 0, 0, 0, filename);
			if (ret != 0)
			{
				handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
				WriteLine (handle, title, strlen(title));
				CloseFile(handle);
			}

			handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
			WriteLine (handle, data, strlen(data));
			CloseFile(handle);
		}
	}

	sprintf(filepath,"%s\\log_data\\%s\\%s\\%s.csv",loc_id_file_path,loc_model_name,loc_operation,loc_log_data_key);
	ret = GetFirstFile (filepath, 1, 0, 0, 0, 0, 0, filename);
	if (ret != 0)
	{
		handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
		WriteLine (handle, title, strlen(title));
		CloseFile(handle);
	}

	handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
	WriteLine (handle, data, strlen(data));
	CloseFile(handle);

	if ((loc_prod_num != 0)&&(loc_serial != 0))	// only do if non dummy tally card used
	{
		sprintf(filepath,"%s\\log_data\\%08d\\%07d\\%s.csv",loc_id_file_path,loc_prod_num,loc_serial,loc_log_data_key);
		ret = GetFirstFile (filepath, 1, 0, 0, 0, 0, 0, filename);
		if (ret != 0)
		{
			handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
			WriteLine (handle, title, strlen(title));
			CloseFile(handle);
		}

		handle = OpenFile (filepath, VAL_WRITE_ONLY, VAL_APPEND, VAL_ASCII);
		WriteLine (handle, data, strlen(data));
		CloseFile(handle);
	}

	return(0);
}

int make_log_dir_struct(int jig_id,char* model_name,char* operation,int loc_prod_num,int loc_serial)
{
	char filepath[500] = {""};
	char filename[500] = {""};
	int ret = 0;

	static char last_id[500] = {""};
	static char now_id[500] = {""};

	char id_file_path[500] = {"c:\\C2"};

	//if (jig_id > 0) sprintf(id_file_path,"c:\\c2\\id%d",jig_id);

	//sprintf(now_id,"%d %d %d",jig_id,loc_prod_num,loc_serial);
	sprintf(now_id,"%d %d",loc_prod_num,loc_serial);

	if (strcmp(now_id,last_id) == 0) return(0);

	// make folders
	sprintf(filepath,"%s\\log_data",id_file_path);
	ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
	if (ret != 0) MakeDir (filepath);

	sprintf(filepath,"%s\\log_data\\%s",id_file_path,model_name);
	ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
	if (ret != 0) MakeDir (filepath);

	sprintf(filepath,"%s\\log_data\\%s\\%s",id_file_path,model_name,operation);
	ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
	if (ret != 0) MakeDir (filepath);

	if ((loc_prod_num != 0)&&(loc_serial != 0))	// only do if non dummy tally card used
	{
		sprintf(filepath,"%s\\log_data\\%08d",id_file_path,loc_prod_num);
		ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
		if (ret != 0) MakeDir (filepath);

		sprintf(filepath,"%s\\log_data\\%08d\\%07d",id_file_path,loc_prod_num,loc_serial);
		ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
		if (ret != 0) MakeDir (filepath);
	}

	if (log_use_network == YES)
	{
		sprintf(filepath,"%s\\%s",net_log_file_path,model_name);
		ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
		if (ret != 0) MakeDir (filepath);

		sprintf(filepath,"%s\\%s\\%s",net_log_file_path,model_name,operation);
		ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
		if (ret != 0) MakeDir (filepath);

		if ((loc_prod_num != 0)&&(loc_serial != 0))	// only do if non dummy tally card used
		{
			sprintf(filepath,"%s\\%08d",net_log_file_path,loc_prod_num);
			ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
			if (ret != 0) MakeDir (filepath);

			sprintf(filepath,"%s\\%08d\\%07d",net_log_file_path,loc_prod_num,loc_serial);
			ret = GetFirstFile (filepath, 0, 0, 0, 0, 0, 1, filename);
			if (ret != 0) MakeDir (filepath);
		}
	}

	strcpy(last_id,now_id);

	return(0);
}


int create_log_if_none(int p_number)
{
    int hstmt;
    int resCode;
    int stat;
    int ret;
    char match[50];
    char sql_mess[5500] = {""};

    int year,month,day;
    //char sql_tmp[1000];

    char table[200] = {""};

	if (log_use_network != YES) return(0);

	GetSystemDate (&month, &day, &year);

    sprintf(match,"LOG_D%04d%02d",year,month);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);


    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
		sprintf(sql_mess,"CREATE TABLE %s (  \
    				INSTANCE VARCHAR(50) DEFAULT'' NOT NULL, \
					P_NUMBER INT DEFAULT 0 NOT NULL, \
					S_NUMBER INT DEFAULT 0 NOT NULL, \
					COMPUTER_NAME VARCHAR(50) DEFAULT'' NOT NULL, \
					TS_FRONT VARCHAR(50) DEFAULT'' NOT NULL, \
					TS_PAGE VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_ID INT DEFAULT 0 NOT NULL, \
					MODE INT DEFAULT 0 NOT NULL, \
					ID_OPERATOR INT DEFAULT 0 NOT NULL, \
					ID_RESPONSIBLE INT DEFAULT 0 NOT NULL, \
					JIG_MODEL_NAME VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_OPERATION VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_SEQ VARCHAR(50) DEFAULT'' NOT NULL, \
					OP_DATE DATE DEFAULT 'NOW' NOT NULL, \
					OP_TIME TIME DEFAULT 'NOW' NOT NULL, \
					KEY_NAME VARCHAR(100) DEFAULT'' NOT NULL, \
					UNIT VARCHAR(50) DEFAULT'' NOT NULL, \
					VAL VARCHAR(100) DEFAULT'' NOT NULL, \
					FAIL_FLAG INT DEFAULT 0 NOT NULL, \
					SYSTEM_FLAG INT DEFAULT 0 NOT NULL)",match);

        resCode = DBImmediateSQL(db_hdbc, sql_mess);
    }

    if (p_number == 0) return(0);

    sprintf(match,"LOG_M%08d",p_number);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
		sprintf(sql_mess,"CREATE TABLE %s (  \
    				INSTANCE VARCHAR(50) DEFAULT'' NOT NULL, \
					P_NUMBER INT DEFAULT 0 NOT NULL, \
					S_NUMBER INT DEFAULT 0 NOT NULL, \
					COMPUTER_NAME VARCHAR(50) DEFAULT'' NOT NULL, \
					TS_FRONT VARCHAR(50) DEFAULT'' NOT NULL, \
					TS_PAGE VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_ID INT DEFAULT 0 NOT NULL, \
					MODE INT DEFAULT 0 NOT NULL, \
					ID_OPERATOR INT DEFAULT 0 NOT NULL, \
					ID_RESPONSIBLE INT DEFAULT 0 NOT NULL, \
					JIG_MODEL_NAME VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_OPERATION VARCHAR(50) DEFAULT'' NOT NULL, \
					JIG_SEQ VARCHAR(50) DEFAULT'' NOT NULL, \
					OP_DATE DATE DEFAULT 'NOW' NOT NULL, \
					OP_TIME TIME DEFAULT 'NOW' NOT NULL, \
					KEY_NAME VARCHAR(100) DEFAULT'' NOT NULL, \
					UNIT VARCHAR(50) DEFAULT'' NOT NULL, \
					VAL VARCHAR(100) DEFAULT'' NOT NULL, \
					FAIL_FLAG INT DEFAULT 0 NOT NULL, \
					SYSTEM_FLAG INT DEFAULT 0 NOT NULL)",match);

        resCode = DBImmediateSQL(db_hdbc, sql_mess);
    }
    return(0);
}



// ****************** old recnet.c *******************

int show_db_error_string(int error,char* e_mess)
{
    char loc_mess[][100] = {"Pass",
                            "A table or Record that should exist can not be found",
                            "The Given stage name can not be found in the profiles table",
                            "The previous automated stage has not been passed",
                            "The current stage has already been passed once",
                            "Can't reprint before 1st print",
                            "This MAC address has already been used",
                            "This Unit is already MAC Linked",
                            "This Unit is Scraped"
                            };

    //error = error * -1;
    if ((error > 0)||(error <= DB_ER_MAX))
    {
        strcpy(e_mess,"invlaid error code");
        return(-1);
    }
    strcpy(e_mess,loc_mess[error]);
    return(0);
}

int create_xp_if_none(int year,int month)
{
    int hstmt;
    int resCode;
    int stat;
    int ret;
    char match[50];
    char sql_mess[5500] = {""};
    //char sql_tmp[1000];

    char table[200] = {""};

    sprintf(match,"XP_%04d_%02d",year,month);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
        sprintf(sql_mess,"CREATE TABLE %s ( REF INT DEFAULT 0 , P_NUMBER INT DEFAULT 0 , \
                    S_NUMBER INT DEFAULT 0, OPERATION VARCHAR(15) DEFAULT '' , STAGE INT DEFAULT 0, STAGE_NUM INT DEFAULT 0, \
                    OPERATOR INT DEFAULT 0, OP_DATE DATE, OP_TIME TIME, \
                    TOT_INDEX INT DEFAULT 0, STAGE_INDEX INT DEFAULT 0, \
                    TYPE_INDEX INT DEFAULT 0, CONT_FLAG INT DEFAULT 0, \
                    MORE_FLAG INT DEFAULT 0, LAST_FLAG INT DEFAULT 0, FIN_FLAG INT DEFAULT 0, \
                    JOB_NO VARCHAR(10) DEFAULT '', JIG_STAGE VARCHAR(15) DEFAULT '', \
                    CYCLE_TIME INT DEFAULT 0, \
                    MANUAL_FLAG INT DEFAULT 0 , \
                    JIG_NAME VARCHAR(50) DEFAULT '', \
                    MODE_NUM INT DEFAULT 0, \
                    SUBMODE_NUM INT DEFAULT 0, \
                    MEASURE_NUM INT DEFAULT 0, \
                    MODE_NAME VARCHAR(50) DEFAULT '', \
                    SUBMODE_NAME VARCHAR(50) DEFAULT '', \
                    MEASURE_NAME VARCHAR(50) DEFAULT '', \
                    HI_SPEC DOUBLE PRECISION DEFAULT 0, \
                    LO_SPEC DOUBLE PRECISION DEFAULT 0, \
                    READING DOUBLE PRECISION DEFAULT 0 \
                    )",match);
        resCode = DBImmediateSQL(db_hdbc, sql_mess);

    }
    return(0);
}


// xc tables are the list of completions
int create_xc_if_none(int year,int month)
{
    int hstmt;
    int resCode;
    int stat;
    int ret;
    char match[50];
    char sql_mess[5000] = {""};
    //char sql_tmp[1000];

    char table[200] = {""};

    sprintf(match,"XC_%04d_%02d",year,month);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
        sprintf(sql_mess,"CREATE TABLE %s ( REF INT DEFAULT 0 , P_NUMBER INT DEFAULT 0 , S_NUMBER INT DEFAULT 0 , \
                    JOB_NO VARCHAR(25) DEFAULT '', TEST_FLAG INT DEFAULT 0, SCRAP_REPLACE INT DEFAULT 0, \
                    NUM_JOB_TOT INT DEFAULT 0, NUM_JOB_COUNT INT DEFAULT 0, \
                    FIN_DATE DATE, FIN_TIME TIME, SENT_NOTIFY INT DEFAULT 0 )",match);

        /*
        sprintf(sql_mess,"CREATE TABLE %s ( REF INT DEFAULT 0 , P_NUMBER INT DEFAULT 0 , \
                    S_NUMBER INT DEFAULT 0, OPERATION VARCHAR(15) DEFAULT '' , STAGE INT DEFAULT 0, \
                    OPERATOR INT DEFAULT 0, OP_DATE DATE, OP_TIME TIME, \
                    TOT_INDEX INT DEFAULT 0, STAGE_INDEX INT DEFAULT 0, \
                    TYPE_INDEX INT DEFAULT 0, CONT_FLAG INT DEFAULT 0, MORE_FLAG INT DEFAULT 0 )",match);
        */

        resCode = DBImmediateSQL(db_hdbc, sql_mess);
    }
    return(0);
}

int create_pro_if_none(int p_number)
{
    int hstmt;
    int resCode;
    int stat;
    int ret;
    char match[50];
    char sql_mess[5500] = {""};

    int year,month,day;
    //char sql_tmp[1000];

    char table[200] = {""};

	GetSystemDate (&month, &day, &year);

    sprintf(match,"PRO_D%04d%02d",year,month);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
        sprintf(sql_mess,"CREATE TABLE %s ( \
        			P_NUMBER INT DEFAULT 0 NOT NULL,\
        			S_NUMBER INT DEFAULT 0 NOT NULL,\
        			STAGE_NUM INT DEFAULT 0 NOT NULL,\
        			TEST_FLAG INT DEFAULT 0 NOT NULL,\
        			OPERATION INT DEFAULT 0 NOT NULL,\
        			OPERATOR INT DEFAULT 0 NOT NULL,\
        			RESPONSIBLE INT DEFAULT 0 NOT NULL,\
                    OP_DATE DATE DEFAULT 'NOW' NOT NULL,\
                    OP_TIME TIME DEFAULT 'NOW' NOT NULL,\
                    COMPLETE_FLAG INT DEFAULT 0 NOT NULL,\
                    EXTRA_DATA INT DEFAULT 0 NOT NULL\
                    )",match);
        resCode = DBImmediateSQL(db_hdbc, sql_mess);

    }

    sprintf(match,"PRO_M%08d",p_number);

    //sprintf(sql_mess,"DROP TABLE %s",match);
    //resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // find is table already exists
    hstmt = DBTables(db_hdbc,"","","",DB_TBL_TABLE);
    resCode = DBBindColChar(hstmt,3,128,table,&stat,"");
    while ((ret = DBFetchNext (hstmt)) == DB_SUCCESS)
    {
        if (strcmp(table,match) == 0) break;
    }
    resCode = DBDeactivateSQL(hstmt);

    if (ret != DB_SUCCESS)  //if no table create new table
    {
        sprintf(sql_mess,"CREATE TABLE %s ( \
        			P_NUMBER INT DEFAULT 0 NOT NULL,\
        			S_NUMBER INT DEFAULT 0 NOT NULL,\
        			STAGE_NUM INT DEFAULT 0 NOT NULL,\
        			TEST_FLAG INT DEFAULT 0 NOT NULL,\
        			OPERATION INT DEFAULT 0 NOT NULL,\
        			OPERATOR INT DEFAULT 0 NOT NULL,\
        			RESPONSIBLE INT DEFAULT 0 NOT NULL,\
                    OP_DATE DATE DEFAULT 'NOW' NOT NULL,\
                    OP_TIME TIME DEFAULT 'NOW' NOT NULL,\
                    COMPLETE_FLAG INT DEFAULT 0 NOT NULL,\
                    EXTRA_DATA INT DEFAULT 0 NOT NULL\
                    )",match);
        resCode = DBImmediateSQL(db_hdbc, sql_mess);

    }

    return(0);
}

int get_db_stage_validity(int s_num,int p_num,char* jig_type_in1,char* jig_type_in2,char* jig_type_in3,char* jig_type_out1,char* jig_type_out2,char* jig_type_out3)
{
    int i;
    char stage_title[35][25];
    short stage_auto[35];
    char sql_mess[1500] = {""};
    int ret,resCode;
    int hstmt,stat;
    char stemp[100];

    int stage_prof;

    int stage_count;

	ret = check_module_version("recnet",103);
	if (ret != DB_ER_NONE) return(ret);

    if (p_num == 0) return(DB_ER_NONE);
    if (s_num == 0) return(DB_ER_NONE);

	if (jig_type_out1 != NULL) jig_type_out1[0] = 0;
	if (jig_type_out2 != NULL) jig_type_out2[0] = 0;
	if (jig_type_out3 != NULL) jig_type_out3[0] = 0;

    // get the job, stage profile and job date from the job_index given p_num and ser_num
    sprintf(sql_mess,"SELECT STAGE_PROFILE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);

    resCode = DBBindColInt (hstmt, 1, &stage_prof, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);             // can't find valid job table (probably network fault


    // now using the stage profile and the jig type
    // get the stage number that matches this jig
    // and the previous automatic stage number
    sprintf(sql_mess,"SELECT STAGE_COUNT ");

    for(i=0;i<24;i++)
    {
        sprintf(stemp,", STAGE%d_TITLE, STAGE%d_AUTO",(i+1),(i+1));
        strcat(sql_mess,stemp);
    }

    sprintf(stemp," FROM PROFILE_INDEX WHERE STAGE_PROFILE = %d",stage_prof);
    strcat(sql_mess,stemp);


    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage_count, &stat);
    for (i=0;i<24;i++)
    {
        resCode = DBBindColChar (hstmt, (2+(i*2)), 20, stage_title[i], &stat, "");
        resCode = DBBindColShort (hstmt, (3+(i*2)), &stage_auto[i], &stat);
    }
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);     // can't find valid Prifole (probalby network error)

	if (jig_type_in1 != NULL)
	{
		if (strlen(jig_type_in1) > 0)
		{
    		for (i=0;i<stage_count;i++)
    		{
        		if (strcmp(stage_title[i],jig_type_in1) == 0)
        		{
        			if (jig_type_out1 != NULL) strcpy(jig_type_out1,stage_title[i]);
        		}
    		}
    	}
	}

	if (jig_type_in2 != NULL)
	{
		if (strlen(jig_type_in2) > 0)
		{
    		for (i=0;i<stage_count;i++)
    		{
        		if (strcmp(stage_title[i],jig_type_in2) == 0)
        		{
        			if (jig_type_out2 != NULL) strcpy(jig_type_out2,stage_title[i]);
        		}
    		}
    	}
	}

	if (jig_type_in3 != NULL)
	{
		if (strlen(jig_type_in3) > 0)
		{
    		for (i=0;i<stage_count;i++)
    		{
        		if (strcmp(stage_title[i],jig_type_in3) == 0)
        		{
        			if (jig_type_out3 != NULL) strcpy(jig_type_out3,stage_title[i]);
        		}
    		}
    	}
	}

	return(DB_ER_NONE);
}

int get_db_continue_permission(char* jig_type,int s_num,int p_num,int once_or_many)
{
    int i,tmp;
    char stage_title[35][25];
    short stage_auto[35];
    char sql_mess[1500] = {""};
    int ret,resCode;
    int hstmt,stat;
    char stemp[100];
    char local_job[100] = {""};

    int stage_prof;
    int year,month,day;
    char table_name[100];

    int stage_count;
    int active_stage;
    int last_check_stage;

	ret = check_module_version("recnet",103);
	if (ret != DB_ER_NONE) return(ret);

    if (p_num == 0) return(DB_ER_NONE);
    if (s_num == 0) return(DB_ER_NONE);

    if (strlen(jig_type) == 0) return(DB_ER_BADNAME);

    // get the job, stage profile and job date from the job_index given p_num and ser_num
    sprintf(sql_mess,"SELECT JOB_NO, STAGE_PROFILE, P_DATE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar (hstmt, 1, 20, local_job, &stat, "");
    resCode = DBBindColInt (hstmt, 2, &stage_prof, &stat);
    resCode = DBBindColChar (hstmt, 3, 20, stemp, &stat, "dd.mm.yyyy");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);             // can't find valid job table (probably network fault

    sscanf(stemp,"%d.%d.%d",&day,&month,&year);
    sprintf(table_name,"XP_%04d_%02d",year,month);

    // if XP table for this month does not exits then create it empty
    create_xp_if_none(year,month);

	// if PRO (process) tables do not exist for this date and model then create
	create_pro_if_none(p_num);

    // check serial number has not been scraped
    sprintf(sql_mess,"SELECT * FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND OPERATION = 'SCRAP'",table_name,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret == 0) return(DB_ER_SCRAPED); // UNIT HAS ALREADY BEEN SCRAPED


    // ***********************************************

    // now using the stage profile and the jig type
    // get the stage number that matches this jig
    // and the previous automatic stage number
    sprintf(sql_mess,"SELECT STAGE_COUNT ");

    for(i=0;i<24;i++)
    {
        sprintf(stemp,", STAGE%d_TITLE, STAGE%d_AUTO",(i+1),(i+1));
        strcat(sql_mess,stemp);
    }

    sprintf(stemp," FROM PROFILE_INDEX WHERE STAGE_PROFILE = %d",stage_prof);
    strcat(sql_mess,stemp);


    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage_count, &stat);
    for (i=0;i<24;i++)
    {
        resCode = DBBindColChar (hstmt, (2+(i*2)), 20, stage_title[i], &stat, "");
        resCode = DBBindColShort (hstmt, (3+(i*2)), &stage_auto[i], &stat);
    }
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);     // can't find valid Prifole (probalby network error)

    for (i=0;i<stage_count;i++)
    {
        if (strcmp(stage_title[i],jig_type) == 0) break;
    }
    if (i == stage_count) return(DB_ER_NONE);   // no match found for stage name (eg TEST_END not found)
                                                // probably old profile with no TEST_END. No need to write.

    active_stage = i+1;                         // number of current stage


    last_check_stage = 0;
    if (active_stage > 1)
    {
        for (i=0;i<active_stage-1;i++)  // run through prev stages to find last automatic one
        {                               // if no previous stages then let equal to 0
            if (stage_auto[i] == 1) last_check_stage = (i+1);
        }
    }

    //printf("table_name = %s , active_stage = %d, last_check_stage = %d\n",table_name,active_stage,last_check_stage);

    // now check that the unit has passed the previous auto stage (don't if there wasnt one)
    if (last_check_stage != 0)
    {
        sprintf(sql_mess,"SELECT REF FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS' AND CONT_FLAG = 1",table_name,p_num,s_num,last_check_stage);
        hstmt = DBActivateSQL (db_hdbc, sql_mess);
        resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
        ret = DBFetchNext (hstmt);
        resCode = DBDeactivateSQL (hstmt);
        if (ret != 0) return(DB_ER_NO_PREV_PASS); // not passed previous stage
    }

    // now if only allowed once (eg packing) then check current stage not already passed
    if (once_or_many == DB_ONCE)
    {
        i = active_stage;
        strcpy(stemp,"PASS");
        if (strcmp(jig_type,"CREATE") == 0) strcpy(stemp,jig_type);
        if (strcmp(jig_type,"PRINT") == 0) strcpy(stemp,jig_type);

        sprintf(sql_mess,"SELECT REF FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS' ",table_name,p_num,s_num,active_stage);
        hstmt = DBActivateSQL (db_hdbc, sql_mess);
        resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
        ret = DBFetchNext (hstmt);
        resCode = DBDeactivateSQL (hstmt);
        if (ret == 0) return(DB_ER_ALREADY_PASS); // if found, already passed this stage
    }

    //printf("table_name = %s , active_stage = %d\n",table_name,active_stage);

    return(0);
}

int write_db_result(char* jig_type,int s_num,int p_num,int once_or_many,int operation,int operator,int responsible)
{
    int handle;
    char loc_job[100];
    int loc_test_flag = 0;
    int loc_scrap_replace = 0;
    int day,month,year;
    int day_now,month_now,year_now;
    char stemp[3000];
    int stage_prof;
    char table_name[100];
    char comp_table_name[100];
    char stage_title[35][25];
    char stage_text[35][25];
    char stage_text2[35][25];
    int stage_count;
    short stage_auto[35];
    int has_already_passed = 0;

	int jig_type_num = 0;
	int fin_flag = 0;

    char sql_mess[2000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    int i;
    char op_mess[30] = {""};

    int ref = 0;
    int tot_index = 0;
    int stage_index = 0;
    int type_index = 0;
    //int last_repair = 0;
    int active_stage=0;
    //int next_stage=0;
    int last_auto_stage = 1;
    int last_check_stage;
    int tmp;
    int continue_flag = 0;

    int count_req_quant = 0;
    int count_start_quant = 0;
    int count_fin_quant = 0;

	char next_type[50] = {""};
    int next_type_num=0;

	char process_date_table[50] = {""};
	char process_model_table[50] = {""};

    if (p_num == 0) return(DB_ER_NONE);
    if (s_num == 0) return(DB_ER_NONE);

    if (strlen(jig_type) == 0) return(DB_ER_BADNAME);

	GetSystemDate (&month, &day, &year);
	sprintf(process_date_table,"PRO_D%04d%02d",year,month);

	sprintf(process_model_table,"PRO_M%08d",p_num);

    if ((operation < 1) || (operation > 3)) return(-1);
    if (operation == DB_PASS)
    {
        strcpy(op_mess,"PASS");
        continue_flag = 1;
    }
    if (operation == DB_FAIL)
    {
        strcpy(op_mess,"FAIL");
        continue_flag = 1;
    }

    if (operation == DB_REPAIR)
    {
        strcpy(op_mess,"REPAIR");
        continue_flag = 0;
    }

	//**********************************
	i=0;
	while(stage_names[i][0] != 0)
	{
		if (strcmp(stage_names[i],jig_type) == 0) break;
		i++;
	}

	if (stage_names[i][0] == 0) return(DB_ER_BADNAME);
	jig_type_num = stage_nums[i];
	//**********************************

    //****************** duplicate of get permission ****************

    // get the job, stage profile and job date from the job_index given p_num and ser_num
    sprintf(sql_mess,"SELECT JOB_NO, STAGE_PROFILE, P_DATE, REQ_QUANTITY, STARTED_QUANTITY, FINISHED_QUANTITY, TEST_FLAG, SCRAP_REPLACE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar (hstmt, 1, 20, loc_job, &stat, "");
    resCode = DBBindColInt (hstmt, 2, &stage_prof, &stat);
    resCode = DBBindColChar (hstmt, 3, 20, stemp, &stat, "dd.mm.yyyy");
    resCode = DBBindColInt (hstmt, 4, &count_req_quant, &stat);
    resCode = DBBindColInt (hstmt, 5, &count_start_quant, &stat);
    resCode = DBBindColInt (hstmt, 6, &count_fin_quant, &stat);
    resCode = DBBindColInt (hstmt, 7, &loc_test_flag, &stat);
    resCode = DBBindColInt (hstmt, 8, &loc_scrap_replace, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);             // can't find valid job table (probably network fault

    sscanf(stemp,"%d.%d.%d",&day,&month,&year);
    sprintf(table_name,"XP_%04d_%02d",year,month);

    sscanf(stemp,"%d.%d.%d",&day,&month,&year);

	// if XP table for this month does not exits then create it empty
    create_xp_if_none(year,month);

	// if PRO (process) tables do not exist for this date and model then create
	create_pro_if_none(p_num);

    // check serial number has not been scraped
    sprintf(sql_mess,"SELECT * FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND OPERATION = 'SCRAP'",table_name,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret == 0) return(DB_ER_SCRAPED); // UNIT HAS ALREADY BEEN SCRAPED

    GetSystemDate (&month_now, &day_now, &year_now);
    sprintf(comp_table_name,"XC_%04d_%02d",year_now,month_now);

    // if XP table for this month does not exits then create it empty

    // ***********************************************

    // now using the stage profile and the jig type
    // get the stage number that matches this jig
    // and the previous automatic stage number
    sprintf(sql_mess,"SELECT STAGE_COUNT ");

    for(i=0;i<24;i++)
    {
        sprintf(stemp,", STAGE%d_TITLE, STAGE%d_AUTO, STAGE%d_TEXT1, STAGE%d_TEXT2",(i+1),(i+1),(i+1),(i+1));
        strcat(sql_mess,stemp);
    }

    sprintf(stemp," FROM PROFILE_INDEX WHERE STAGE_PROFILE = %d",stage_prof);
    strcat(sql_mess,stemp);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage_count, &stat);
    for (i=0;i<24;i++)
    {
        resCode = DBBindColChar (hstmt, (2+(i*4)), 20, stage_title[i], &stat, "");
        resCode = DBBindColShort (hstmt, (3+(i*4)), &stage_auto[i], &stat);
        resCode = DBBindColChar (hstmt, (4+(i*4)), 20, stage_text[i], &stat, "");
        resCode = DBBindColChar (hstmt, (5+(i*4)), 20, stage_text2[i], &stat, "");
    }
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);     // can't find valid Profile (probalby network error)

	// find number of jig stage
    for (i=0;i<stage_count;i++)
    {
        if (strcmp(stage_title[i],jig_type) == 0) break;
    }
    if (i == stage_count) return(DB_ER_NONE);   // no match found for stage name (eg TEST_END not found)
                                                // probably old profile with no TEST_END. No need to write.
    active_stage = i+1;                 // number of current stage
	sprintf(com_stage_text,"%s %s",stage_text[i],stage_text2[i]);

	// find number next active stage
    for (i=(active_stage);i<stage_count;i++)
    {
        if (strlen(stage_title[i]) > 0) break;
    }
	if (i != stage_count)
	{
		strcpy(next_type,stage_title[i]);
		i=0;
		while(stage_names[i][0] != 0)
		{
			if (strcmp(stage_names[i],next_type) == 0) break;
			i++;
		}
		if (stage_names[i][0] != 0) next_type_num = stage_nums[i];
	}

    //jig_type,jig_type_num
	//next_type,next_type_num

    // find number of the last automatic stage
    for (i=(stage_count-1);i>=0;i--)
    {
        if (stage_auto[i] == 1) break;
    }
    if (i<0) last_auto_stage = 1;
    else last_auto_stage = i+1;

    last_check_stage = 0;
    if (active_stage > 1)
    {
        for (i=0;i<active_stage-1;i++)  // run through prev stages to find last automatic one
        {                               // if no previous stages then let equal to 0
            if (stage_auto[i] == 1) last_check_stage = (i+1);
        }
    }

    // now check that the unit has passed the previous auto stage (don't if there wasnt one)
    if (last_check_stage != 0)
    {
        sprintf(sql_mess,"SELECT REF FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS' AND CONT_FLAG = 1",table_name,p_num,s_num,last_check_stage);
        hstmt = DBActivateSQL (db_hdbc, sql_mess);
        resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
        ret = DBFetchNext (hstmt);
        resCode = DBDeactivateSQL (hstmt);
        if (ret != 0) return(DB_ER_NO_PREV_PASS); // not passed previous stage
    }

    // now if only allowed once (eg packing) then check current stage not already passed
    sprintf(sql_mess,"SELECT REF FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS' ",table_name,p_num,s_num,active_stage);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret == 0) has_already_passed = 1;   // pass;
    if ((ret == 0)&&(once_or_many == DB_ONCE)) return(DB_ER_ALREADY_PASS); // if found, already passed this stage


	// now check if the unit is already packed and is being re-worked. (this is allowd but a flag must be set in record)
	fin_flag = 0;
    sprintf(sql_mess,"SELECT REF FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = 'PASS'",table_name,p_num,s_num,last_auto_stage);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
	if (ret == 0) fin_flag = 1;

    //***************************************************************

    //DBBeginTran (db_hdbc);

    // get the next global_index (g_index) this is the number of non create/print operation for all types and stages
    sprintf(sql_mess,"SELECT MAX(TOT_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0",table_name,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &tot_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    tot_index++;

    // get the next stage_index (g_index) this is the number of non create/print operation for this type of stage
    sprintf(sql_mess,"SELECT MAX(STAGE_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0 AND STAGE = %d",table_name,p_num,s_num,active_stage);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    stage_index++;

    // get the next loc_index (l_index) this is the number of operations for this type of stage & type of operation
    sprintf(sql_mess,"SELECT MAX(TYPE_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0 AND STAGE = %d AND OPERATION = '%s' ",table_name,p_num,s_num,active_stage,op_mess);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &type_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    type_index++;

    // clear last flag on previous record
    sprintf(sql_mess,"UPDATE %s SET LAST_FLAG = 0 WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0",table_name,p_num,s_num);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // get the next ref number
    sprintf(sql_mess,"SELECT MAX(REF) FROM %s",table_name);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    ref++;

    //create new record (with LAST_FLAG = 1)
    sprintf(sql_mess,"INSERT INTO %s ( REF, P_NUMBER, S_NUMBER, OPERATION, STAGE, STAGE_NUM, OPERATOR, OP_DATE, OP_TIME, TOT_INDEX, STAGE_INDEX, TYPE_INDEX, CONT_FLAG, LAST_FLAG, FIN_FLAG, JOB_NO, JIG_STAGE ) \
                VALUES ( %d, %d, %d, '%s', %d, %d, %d, 'NOW', 'NOW',  %d, %d, %d, %d, 1, %d, '%s', '%s' )",
                table_name,ref,p_num,s_num,op_mess,active_stage,jig_type_num,operator,tot_index,stage_index,type_index,continue_flag,fin_flag,loc_job,jig_type);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);


    // check insert is done
    sprintf(stemp,"SELECT * FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND OPERATION = '%s' AND TYPE_INDEX = %d AND STAGE_INDEX = %d",table_name,p_num,s_num,active_stage,op_mess,type_index,stage_index);
    hstmt = DBActivateSQL (db_hdbc, stemp);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

    if (ret != 0)
    {
        handle = OpenFile ("c:\\temp\\alchemy_log.txt", VAL_READ_ONLY,
                           VAL_APPEND, VAL_ASCII);
        WriteLine (handle, sql_mess, strlen(sql_mess));
        CloseFile(handle);
        return(DB_ER_NOWRITE);
    }
/*
    // duplicate of write process for new single XP_TABLE
    // clear last flag on previous record
    sprintf(sql_mess,"UPDATE XP_TABLE SET LAST_FLAG = 0 WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0",p_num,s_num);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

    // get the next ref number (XP_TABLE)
    sprintf(sql_mess,"SELECT MAX(REF) FROM XP_TABLE");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    ref++;

    //create new record (with LAST_FLAG = 1)
    sprintf(sql_mess,"INSERT INTO XP_TABLE ( REF, P_NUMBER, S_NUMBER, OPERATION, STAGE_PROFILE, STAGE, STAGE_NUM, OPERATOR, OP_DATE, OP_TIME, TOT_INDEX, STAGE_INDEX, TYPE_INDEX, CONT_FLAG, LAST_FLAG, FIN_FLAG, JOB_NO, SCRAP_REPLACE, TEST_FLAG, JIG_STAGE ) \
                VALUES ( %d, %d, %d, '%s', %d, %d, %d, %d, 'NOW', 'NOW',  %d, %d, %d, %d, 1, %d, '%s', %d, %d, '%s' )",
                ref,p_num,s_num,op_mess,stage_prof,active_stage,jig_type_num,operator,tot_index,stage_index,type_index,continue_flag, fin_flag, loc_job, loc_test_flag, loc_scrap_replace, jig_type);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

*/
    //DBCommit (db_hdbc);

	// ALL THE NEW STUFF
	if (operation == DB_PASS)	// update only on pass
	{
		if (active_stage == 1)
		{
			// create record
			sprintf(sql_mess,"INSERT INTO PROCESS_LOCK (P_NUMBER,S_NUMBER,STAGE_PASS,STAGE_NEXT,TEST_FLAG) VALUES (%d,%d,%d,%d,%d)",p_num,s_num,jig_type_num,next_type_num,loc_test_flag);
        	DBImmediateSQL(db_hdbc, sql_mess);
		}

    	sprintf(sql_mess,"SELECT STAGE_PASS FROM PROCESS_LOCK WHERE P_NUMBER = %d AND S_NUMBER = %d",p_num,s_num);
    	hstmt = DBActivateSQL (db_hdbc, sql_mess);
    	resCode = DBBindColInt (hstmt, 1, &tmp, &stat);
    	ret = DBFetchNext (hstmt);
    	resCode = DBDeactivateSQL (hstmt);
		if (ret == 0)
		{
			if (jig_type_num > tmp) // update only if new record is further along in process than old record
			{
				sprintf(sql_mess,"UPDATE PROCESS_LOCK SET STAGE_PASS = %d, STAGE_NEXT = %d WHERE P_NUMBER = %d AND S_NUMBER = %d",jig_type_num,next_type_num,p_num,s_num);
        		DBImmediateSQL(db_hdbc, sql_mess);
        	}
        }

        if (active_stage == last_auto_stage)
        {
			// NEW STUFF DELECT RECORD FROM PROCESS_LOCK TABLE
			sprintf(sql_mess,"DELETE FROM PROCESS_LOCK WHERE P_NUMBER = %d AND S_NUMBER = %d",p_num,s_num);
        	DBImmediateSQL(db_hdbc, sql_mess);
        }
	}

	if ((operation == DB_PASS)&&(active_stage == last_auto_stage))
	{

		// WRITE PASS AND FAIL INFO TO NEW TABLES (WITH COMPLETE_FLAG SET TO 1)
		sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,COMPLETE_FLAG,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d,%d)",process_date_table,p_num,s_num,jig_type_num,1,loc_test_flag,operation,operator,responsible);
    	DBImmediateSQL(db_hdbc, sql_mess);

		sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,COMPLETE_FLAG,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d,%d)",process_model_table,p_num,s_num,jig_type_num,1,loc_test_flag,operation,operator,responsible);
    	DBImmediateSQL(db_hdbc, sql_mess);
	}
    else
    {
		// WRITE PASS AND FAIL INFO TO NEW TABLES (LEAVE COMPLETE_FLAG SET TO 0)
		sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d)",process_date_table,p_num,s_num,jig_type_num,loc_test_flag,operation,operator,responsible);
    	DBImmediateSQL(db_hdbc, sql_mess);

		sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d)",process_model_table,p_num,s_num,jig_type_num,loc_test_flag,operation,operator,responsible);
    	DBImmediateSQL(db_hdbc, sql_mess);
    }
	// END OF ALL THE NEW STUFF

    com_pnum = p_num;
    com_snum = s_num;
    com_stage = active_stage;
    com_stage_index = stage_index;
    strcpy(com_table,table_name);

    if (has_already_passed == 1) return(0); // has already passed so don't update job or completions list

    if ((operation == DB_PASS)&&(active_stage == 1))    // tally card print stage flag
    {
        count_start_quant++;
        if (count_start_quant == 1)
        {
            sprintf(sql_mess,"UPDATE JOB_INDEX SET STARTED_QUANTITY = (STARTED_QUANTITY + 1) , START_DATE = 'NOW' ,START_TIME = 'NOW' WHERE JOB_NO = '%s' AND SCRAP_REPLACE = %d",loc_job,loc_scrap_replace);
            DBImmediateSQL(db_hdbc, sql_mess);
        }
        else
        {
            sprintf(sql_mess,"UPDATE JOB_INDEX SET STARTED_QUANTITY = (STARTED_QUANTITY + 1) WHERE JOB_NO = '%s' AND SCRAP_REPLACE = %d",loc_job,loc_scrap_replace);
            DBImmediateSQL(db_hdbc, sql_mess);
        }
    }

    //if ((operation == DB_PASS)&&(active_stage == stage_count))
    if ((operation == DB_PASS)&&(active_stage == last_auto_stage))  // this is the final stage in the process
    {
        count_fin_quant++;

        create_xc_if_none(year_now,month_now);  // if no completions table create a new one.

        ref = 0;
        sprintf(sql_mess,"SELECT MAX(REF) FROM %s",comp_table_name);
        hstmt = DBActivateSQL (db_hdbc, sql_mess);
        resCode = DBBindColInt (hstmt, 1, &ref, &stat);
        ret = DBFetchNext (hstmt);
        resCode = DBDeactivateSQL (hstmt);
        ref++;


        sprintf(sql_mess,"INSERT INTO %s ( REF, P_NUMBER, S_NUMBER, JOB_NO, TEST_FLAG, SCRAP_REPLACE, NUM_JOB_TOT, NUM_JOB_COUNT, FIN_DATE, FIN_TIME ) \
                VALUES ( %d, %d, %d, '%s', %d, %d, %d, %d, 'NOW', 'NOW' )",comp_table_name,ref,p_num,s_num,loc_job,loc_test_flag,loc_scrap_replace,count_req_quant,count_fin_quant);
        resCode = DBImmediateSQL(db_hdbc, sql_mess);


        if (count_req_quant == count_fin_quant) // the item is last in job (mark job as finished with date and time)
        {
            // update job table
            sprintf(sql_mess,"UPDATE JOB_INDEX SET FINISHED_QUANTITY = %d , COMP_DATE = 'NOW' ,COMP_TIME = 'NOW' WHERE JOB_NO = '%s' AND SCRAP_REPLACE = %d",count_fin_quant,loc_job,loc_scrap_replace);
            DBImmediateSQL(db_hdbc, sql_mess);
        }
        else    // not last item in job so just update finished count.
        {
            sprintf(sql_mess,"UPDATE JOB_INDEX SET FINISHED_QUANTITY = %d WHERE JOB_NO = '%s' AND SCRAP_REPLACE = %d",count_fin_quant,loc_job,loc_scrap_replace);
            DBImmediateSQL(db_hdbc, sql_mess);
        }
    }

    return(0);
}

int write_db_extra_fail(int manual,char* mode_name,char* sub_name,char* meas_name,
                            int mode_num,int sub_num,int meas_num,
                            double hi_spec,double lo_spec,double reading)
{
    char sql_mess[1000] = {""};
    //int ret,resCode;
    //int hstmt;
    //int stat;
    int i,len;

    char loc_mode_name[50];

    strcpy(loc_mode_name,mode_name);

    len = strlen(loc_mode_name);

    for(i=0;i<len;i++)
    {
        if (isprint(loc_mode_name[i]) == 0) loc_mode_name[i] = ' ';
    }

    if ((com_pnum == 0)||(com_snum == 0)) return(0);


    sprintf(sql_mess,"UPDATE %s SET MANUAL_FLAG = %d , JIG_NAME = '%s' ,MODE_NAME = '%s', SUBMODE_NAME = '%s', MEASURE_NAME = '%s',MODE_NUM = %d, SUBMODE_NUM = %d, MEASURE_NUM = %d, HI_SPEC = %5.5le, LO_SPEC = %5.5le, READING = %5.5le WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND STAGE_INDEX = %d",
        com_table,
        manual,com_stage_text,loc_mode_name,sub_name,meas_name,mode_num,sub_num,meas_num,hi_spec,lo_spec,reading,
        com_pnum,com_snum,com_stage,com_stage_index);

    DBImmediateSQL(db_hdbc, sql_mess);
/*
    // do same for XP_TABLE
    sprintf(sql_mess,"UPDATE XP_TABLE SET MANUAL_FLAG = %d , JIG_NAME = '%s' ,MODE_NAME = '%s', SUBMODE_NAME = '%s', MEASURE_NAME = '%s',MODE_NUM = %d, SUBMODE_NUM = %d, MEASURE_NUM = %d, HI_SPEC = %lf, LO_SPEC = %lf, READING = %lf WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE = %d AND STAGE_INDEX = %d",
        manual,com_stage_text,loc_mode_name,sub_name,meas_name,mode_num,sub_num,meas_num,hi_spec,lo_spec,reading,
        com_pnum,com_snum,com_stage,com_stage_index);

    DBImmediateSQL(db_hdbc, sql_mess);
*/

    com_pnum = 0;
    com_snum = 0;
    com_stage = 0;
    com_stage_index = 0;
    strcpy(com_table,"");
    strcpy(com_stage_text,"");

    return(0);
}

int scrap_serial_number(int p_num,int s_num,int op_id)
{
	int stat;
	int ret = 0;
	int resCode = 0;
	int hstmt;
	char sql_mess[500] = {""};

	int day,month,year;
	char table[100] = {""};
	int i =0;

	char local_job[100] = {""};
	char stemp[100] = {""};
	int j_ref=0;
	int xp_ref=0;

	int stage = 0;
	int tot_index = 0;
	int stage_index = 0;
	char jig_stage[100] = {""};
	int jig_stage_num = 0;

	int req_quant = 0;
	int fin_quant = 0;

	int test_flag = 0;

	char process_date_table[50] = {""};
	char process_model_table[50] = {""};

	//DBBeginTran (db_hdbc);

	GetSystemDate (&month, &day, &year);
	sprintf(process_date_table,"PRO_D%04d%02d",year,month);

	sprintf(process_model_table,"PRO_M%08d",p_num);

	ret = check_module_version("recnet",103);
	if (ret != DB_ER_NONE) return(ret);


	create_pro_if_none(p_num);

 	//select stage profile and job date from the job_index given p_num and ser_num
    sprintf(sql_mess,"SELECT JOB_NO, REF, P_DATE, REQ_QUANTITY, FINISHED_QUANTITY, TEST_FLAG FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar (hstmt, 1, 20, local_job, &stat, "");
    resCode = DBBindColInt (hstmt, 2, &j_ref, &stat);
    resCode = DBBindColChar (hstmt, 3, 20, stemp, &stat, "dd.mm.yyyy");
    resCode = DBBindColInt (hstmt, 4, &req_quant, &stat);
    resCode = DBBindColInt (hstmt, 5, &fin_quant, &stat);
    resCode = DBBindColInt (hstmt, 6, &test_flag, &stat);

    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0)
    {
    	//DBCommit (db_hdbc);
    	return(-1);             // can't find valid job table (probably network fault
    }

	sscanf(stemp,"%d.%d.%d",&day,&month,&year);

	// create table name
	sprintf(table,"XP_%04d_%02d",year,month);

	sprintf(sql_mess,"SELECT STAGE, TOT_INDEX, STAGE_INDEX, JIG_STAGE FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND OPERATION <> 'SCRAP' ORDER BY REF DESC",table,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage, &stat);
    resCode = DBBindColInt (hstmt, 2, &tot_index, &stat);
    resCode = DBBindColInt (hstmt, 3, &stage_index, &stat);
    resCode = DBBindColChar (hstmt, 4, 15, jig_stage, &stat, "");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	i=0;
	while(stage_names[i][0] != 0)
	{
		if (strcmp(stage_names[i],jig_stage) == 0) break;
		i++;
	}

	if (stage_names[i][0] == 0) return(-2);
	jig_stage_num = stage_nums[i];

	stage = stage;
	tot_index++;
	stage_index++;

	// select current highest ref number
	sprintf(sql_mess,"SELECT MAX(REF) FROM %s",table);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &xp_ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	xp_ref++;

    // clear last flag on previous record
    sprintf(sql_mess,"UPDATE %s SET LAST_FLAG = 0 WHERE P_NUMBER = %d AND S_NUMBER = %d",table,p_num,s_num);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

	sprintf(sql_mess,"INSERT INTO %s ( REF, P_NUMBER, S_NUMBER, OPERATION, OPERATOR, OP_DATE, OP_TIME, STAGE, JIG_STAGE, STAGE_NUM, TOT_INDEX, STAGE_INDEX, LAST_FLAG, CONT_FLAG, TYPE_INDEX, JOB_NO) VALUES ( %d , %d , %d , 'SCRAP', %d, 'NOW', 'NOW', %d, '%s' ,%d, %d, %d, 1, 1, 1, '%s' )",table,xp_ref,p_num,s_num,op_id,stage,jig_stage,jig_stage_num,tot_index,stage_index,local_job);
	resCode = DBImmediateSQL(db_hdbc, sql_mess);

	// new stuff to delete PROCESS_LOCK information
	sprintf(sql_mess,"DELETE FROM PROCESS_LOCK WHERE P_NUMBER = %d AND S_NUMBER = %d",p_num,s_num);
    DBImmediateSQL(db_hdbc, sql_mess);

	// WRITE PASS AND FAIL INFO TO NEW TABLES (WITH COMPLETE_FLAG SET TO 1)
	sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,COMPLETE_FLAG,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d,%d)",process_date_table,p_num,s_num,jig_stage_num,1,test_flag,DB_SCRAP,op_id,op_id);
    DBImmediateSQL(db_hdbc, sql_mess);

	sprintf(sql_mess,"INSERT INTO %s (P_NUMBER,S_NUMBER,STAGE_NUM,COMPLETE_FLAG,TEST_FLAG,OPERATION,OPERATOR,RESPONSIBLE) VALUES (%d,%d,%d,%d,%d,%d,%d,%d)",process_model_table,p_num,s_num,jig_stage_num,1,test_flag,DB_SCRAP,op_id,op_id);
    DBImmediateSQL(db_hdbc, sql_mess);

/*
	// select current highest ref number XP_TABLE
	sprintf(sql_mess,"SELECT MAX(REF) FROM XP_TABLE");
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &xp_ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);

	xp_ref++;

    // clear last flag on previous record XP_TABLE
    sprintf(sql_mess,"UPDATE XP_TABLE SET LAST_FLAG = 0 WHERE P_NUMBER = %d AND S_NUMBER = %d",p_num,s_num);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);

	sprintf(sql_mess,"INSERT INTO XP_TABLE ( REF, P_NUMBER, S_NUMBER, OPERATION, OPERATOR, OP_DATE, OP_TIME, STAGE, JIG_STAGE, STAGE_NUM, TOT_INDEX, STAGE_INDEX, LAST_FLAG, CONT_FLAG, TYPE_INDEX, JOB_NO, TEST_FLAG) VALUES ( %d , %d , %d , 'SCRAP', %d, 'NOW', 'NOW', %d, '%s', %d, %d, %d, 1, 1, 1, '%s' ,%d)",xp_ref,p_num,s_num,op_id,stage,jig_stage,jig_stage_num,tot_index,stage_index,local_job,test_flag);
	resCode = DBImmediateSQL(db_hdbc, sql_mess);
*/
	// INC JOB VALUES
	fin_quant++;
	if (fin_quant > req_quant) fin_quant = req_quant;
	if (fin_quant != req_quant)
	{
		sprintf(sql_mess,"UPDATE JOB_INDEX SET FINISHED_QUANTITY = %d WHERE REF = %d",fin_quant,j_ref);
	}
	else
	{
		sprintf(sql_mess,"UPDATE JOB_INDEX SET FINISHED_QUANTITY = %d, COMP_DATE = 'NOW', COMP_TIME = 'NOW' WHERE REF = %d",fin_quant,j_ref);
	}

	//sprintf(sql_mess,"UPDATE JOB_INDEX SET FINISHED_QUANTITY = (FINISHED_QUANTITY + 1) WHERE REF = %d",j_ref);
	resCode = DBImmediateSQL(db_hdbc, sql_mess);

	// clear any MAC address link and ATO link
	if ((p_num != 0) && (s_num != 0))
	{
		// clear MAC address link
		sprintf(sql_mess,"UPDATE MAC_SER_LINK SET VALID_FLAG = 0 WHERE P_NUMBER = %d AND S_NUMBER = %d",p_num,s_num);
		resCode = DBImmediateSQL(db_hdbc, sql_mess);

		// clear ATO link
		sprintf(sql_mess,"UPDATE AP_LINK SET VALID_FLAG = 0 WHERE P_NUM = %d AND SP_NUM = %d",p_num,s_num);
		resCode = DBImmediateSQL(db_hdbc, sql_mess);
	}

	//DBCommit (db_hdbc);

	return(0);
}

/*
int write_db_reprint(int s_num,int p_num,int operator)
{
    char loc_job[100];
    int day,month,year;
    char stemp[100];
    int stage_prof;
    char table_name[100];
    //char stage_title[25][25];
    //int stage_count;
    //short stage_auto[25];

    char sql_mess[1000] = {""};
    int ret,resCode;
    int hstmt;
    int stat;
    //int i;
    char op_mess[] = {"REPRINT"};

    int ref = 0;
    int tot_index = 0;
    int stage_index = 0;
    int type_index = 0;
    //int last_repair = 0;
    int active_stage = 0;
    //int last_check_stage;
    //int tmp;
    int continue_flag = 1;

    // get the job, stage profile and job date from the job_index given p_num and ser_num
    sprintf(sql_mess,"SELECT JOB_NO, STAGE_PROFILE, P_DATE FROM JOB_INDEX WHERE P_NUMBER = %d AND SERIAL_START <= %d AND SERIAL_FINISH >= %d",p_num,s_num,s_num);

    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColChar (hstmt, 1, 20, loc_job, &stat, "");
    resCode = DBBindColInt (hstmt, 2, &stage_prof, &stat);
    resCode = DBBindColChar (hstmt, 3, 20, stemp, &stat, "dd.mm.yyyy");
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (ret != 0) return(DB_ER_DB);             // can't find valid job table (probably network fault

    sscanf(stemp,"%d.%d.%d",&day,&month,&year);
    sprintf(table_name,"XP_%04d_%02d",year,month);

    // if XP table for this month does not exits then create it empty
    create_xp_if_none(year,month);

    // get the stage of the process that the unit is currently at.
    sprintf(sql_mess,"SELECT MAX(STAGE) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0 AND CONT_FLAG = 1",table_name,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &active_stage, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    if (active_stage == 0) return(DB_ER_NOT_PRINTED);   // trying to reprint that which is not yet printed

    // get the next ref number
    sprintf(sql_mess,"SELECT MAX(REF) FROM %s",table_name);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &ref, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    ref++;

    // get the next global_index (g_index) this is the number of non create/print operation for all types and stages
    sprintf(sql_mess,"SELECT MAX(TOT_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0",table_name,p_num,s_num);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &tot_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    tot_index++;

    // get the next stage_index (g_index) this is the number of non create/print operation for this type of stage
    sprintf(sql_mess,"SELECT MAX(STAGE_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0 AND STAGE = %d",table_name,p_num,s_num,active_stage);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &stage_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    stage_index++;

    // get the next loc_index (l_index) this is the number of operations for this type of stage & type of operation
    sprintf(sql_mess,"SELECT MAX(TYPE_INDEX) FROM %s WHERE P_NUMBER = %d AND S_NUMBER = %d AND STAGE <> 0 AND STAGE = %d AND OPERATION = '%s' ",table_name,p_num,s_num,active_stage,op_mess);
    hstmt = DBActivateSQL (db_hdbc, sql_mess);
    resCode = DBBindColInt (hstmt, 1, &type_index, &stat);
    ret = DBFetchNext (hstmt);
    resCode = DBDeactivateSQL (hstmt);
    type_index++;

    //create new record
    sprintf(sql_mess,"INSERT INTO %s ( REF, P_NUMBER, S_NUMBER, OPERATION, STAGE, OPERATOR, OP_DATE, OP_TIME, TOT_INDEX, STAGE_INDEX, TYPE_INDEX, CONT_FLAG ) \
                VALUES ( %d, %d, %d, '%s', %d, %d, 'NOW', 'NOW',  %d, %d, %d, %d )",
                table_name,ref,p_num,s_num,op_mess,active_stage,operator,tot_index,stage_index,type_index,continue_flag);
    resCode = DBImmediateSQL(db_hdbc, sql_mess);


    return(0);
}
*/

int check_module_version(char* app_name,int app_ver)
{
	int ver=0;

	int ret = 0;
	int resCode = 0;
	int hstmt;
	char sql_mess[500] = {""};
	int stat;
	int i,j;

	//char stemp[500] = {""};

	j = strlen(app_name);
	for(i=0;i<j;i++)
	{
		app_name[i] = tolower(app_name[i]);
	}

	sprintf(sql_mess,"SELECT APP_VER_NUM FROM APP_REF WHERE APP_NAME = '%s'",app_name);
	hstmt = DBActivateSQL (db_hdbc, sql_mess);
	resCode = DBBindColInt  (hstmt, 1, &ver, &stat);
	ret = DBFetchNext (hstmt);
	resCode = resCode | DBDeactivateSQL (hstmt);

	if (resCode != 0) return(DB_ER_DB);	// an error occured

	if (ret != 0)
	{
		// new app to register
		ver = 0;
		sprintf(sql_mess,"INSERT INTO APP_REF (APP_NAME, APP_VER_NUM) VALUES ('%s', %d)",app_name,ver);
		resCode = DBImmediateSQL(db_hdbc, sql_mess);
	}

	if (app_ver < ver)
	{
		// the application is less than the minimum val listed in the table so fail
		return(DB_ER_VERSION);
	}
	return(0);
}
