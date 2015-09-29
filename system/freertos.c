#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

#include "stm32f4xx_hal.h"

#include "GUI.h"
#include "BUTTON.h"
#include "TREEVIEW.h"
#include "DIALOG.h"
#include "MULTIPAGE.h"
#include "WM.h"

#include "usbd_cdc.h"
#include "usb_device.h"
#include "ltdc.h"
#include "sdram.h"
#include "dma2d.h"
#include "i2c.h"
#include "crc.h"
#include "sd.h"
#include "spi.h"
#include "tim.h"
#include "adc.h"
#include "fatfs.h"
#include "json.h"
#include "json-builder.h"
#include "led.h"
#include "console.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * Firmware Tasks
 */

osThreadId initTaskHandle;
void StartInitTask(void const * argument);

osThreadId touchTaskHandle;
extern void StartTouchTask(void const * argument);

osThreadId pwmTaskHandle;
extern void StartPwmTask(void const * argument);

osThreadId ledTaskHandle;
extern void StartLedTask(void const * argument);

osThreadId fsTaskHandle;
extern void StartFsTask(void const * argument);

osThreadId guiTaskHandle;
extern void StartGuiTask(void const * argument);



/**
 * FreeRTOS Initialisation function
 */
void FREERTOS_Init(void)
{
	osThreadDef(initTask, StartInitTask, osPriorityHigh, 0, 8192);
	initTaskHandle = osThreadCreate(osThread(initTask), NULL);

}


void StartInitTask(void const * argument)
{
	console_init();

	osThreadDef(touchTask, StartTouchTask, osPriorityHigh, 0, 8192);
	touchTaskHandle = osThreadCreate(osThread(touchTask), NULL);

	osThreadDef(pwmTask, StartPwmTask, osPriorityNormal, 0, 8192);
	pwmTaskHandle = osThreadCreate(osThread(pwmTask), NULL);

	osThreadDef(ledTask, StartLedTask, osPriorityNormal, 0, 8192);
	ledTaskHandle = osThreadCreate(osThread(ledTask), NULL);

	osThreadDef(fsTask, StartFsTask, osPriorityNormal, 0, 32768);
	fsTaskHandle = osThreadCreate(osThread(fsTask), NULL);

	osThreadDef(guiTask, StartGuiTask, osPriorityNormal, 0, 8192);
	guiTaskHandle = osThreadCreate(osThread(guiTask), NULL);

	vTaskDelete(initTaskHandle);
}



bool correct_extention(char * filename)
{
	char * point;
	if((point = strrchr(filename, '.')) != NULL)
	{
		if ((strcmp(point, ".ccdb") == 0) || (strcmp(point, ".lua") == 0))
		{
			return true;
		} else {
			return false;
		}	
	} else {
		return false;
	}
}


FRESULT scan_files (
	char* path,        /* Start node to be scanned (also used as work area) */
	TREEVIEW_Handle *hpath
)
{
	printf("calling scan_files(%s)\n", path);
    FRESULT res;
    FILINFO fno;
    DIR dir;
    int i;
    char *fn;   /* This function assumes non-Unicode configuration */
    static char lfn[_MAX_LFN + 1];   /* Buffer to store the LFN */
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
		// Create directory in treeview
		TREEVIEW_ITEM_Handle node =  TREEVIEW_InsertItem(	*hpath,
															TREEVIEW_ITEM_TYPE_NODE,
															0,
															0,
															(const char *)path
														);
		int nb_child = 0;
		TREEVIEW_ITEM_Handle last_item = NULL;
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') 
			{
					continue;             /* Ignore dot entry */
			}
            fn = *fno.lfname ? fno.lfname : fno.fname;
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
				int j = strlen(fn);
				char * dirname = malloc(j);
				strcpy(dirname, fn);
				char newpath[i+j]; // "/0" du i renplacé par "/" plus bas
				newpath[0] = '\0';
				strcat(newpath, path);
				strcat(newpath, "/");
				strcat(newpath, fn);
				TREEVIEW_Handle child_tree = TREEVIEW_CreateEx(0,0,0,0,0,0,0,0);
                res = scan_files(newpath, &child_tree);
				TREEVIEW_ITEM_Handle child_tree_first_node = TREEVIEW_GetItem(	child_tree,
																				0,
																				TREEVIEW_GET_FIRST
																				);
				if (child_tree_first_node != 0)
				{
						child_tree_first_node = TREEVIEW_ITEM_SetText(	child_tree_first_node,
								dirname);
						TREEVIEW_AttachItem(	*hpath,
								child_tree_first_node,
								nb_child ? last_item : node,
								nb_child ? TREEVIEW_INSERT_BELOW : TREEVIEW_INSERT_FIRST_CHILD
								);
						last_item = child_tree_first_node;
				}
				free(dirname);
                if (res != FR_OK) break;
				nb_child++;
            } else {                                       /* It is a file. */
				//if (correct_extention(fn))
				{
					last_item = TREEVIEW_InsertItem(	*hpath,
														TREEVIEW_ITEM_TYPE_LEAF,
														nb_child ? last_item : node,
														nb_child ? TREEVIEW_INSERT_BELOW : TREEVIEW_INSERT_FIRST_CHILD,
														(const char *) fn
													);
					nb_child++;
					//printf("%s/%s\n", path, fn);
				}
            }
        }
        f_closedir(&dir);
    }

    return res;
}

void _cbHBKWIN(WM_MESSAGE * pMsg)
{
	int NCode;
	TREEVIEW_ITEM_Handle Sel;
	int Id;
	char pBuffer[128];
	
	switch (pMsg->MsgId)
	{
		case WM_NOTIFY_PARENT:
			NCode = pMsg->Data.v;
			Id = WM_GetId(pMsg->hWinSrc);
			if (Id == GUI_ID_TREEVIEW0)
			{
				switch (NCode)
				{
					case WM_NOTIFICATION_CLICKED:
						//printf("switching focus to treeview\n");
						WM_SetFocus(pMsg->hWinSrc);
						break;
					case WM_NOTIFICATION_RELEASED:
						Sel = TREEVIEW_GetSel(pMsg->hWinSrc);
						TREEVIEW_ITEM_GetText(Sel, (U8*)pBuffer, 128);
						printf("Selecting \"%s\"\n", pBuffer);
						break;
					case WM_NOTIFICATION_SEL_CHANGED:
						printf("Selection changed\n");
						break;
				}
			}
			else if (Id == GUI_ID_MULTIEDIT0)
			{
				switch (NCode)
				{
					case WM_NOTIFICATION_CLICKED:
						//printf("switching focus to terminal\n");
						WM_SetFocus(pMsg->hWinSrc);
						break;
					case WM_NOTIFICATION_VALUE_CHANGED:
						//printf("WM_NOTIFICATION_VALUE_CHANGED\n");
						break;
				}
			}
			break;

		default:
			WM_DefaultProc(pMsg);
	}
}

void StartFsTask(void const * argument)
{
	printf("Fs task started\n");
	taskENTER_CRITICAL();
	TREEVIEW_Handle filesystem  = TREEVIEW_CreateEx(	500, 0,
														300, 480,
														0, WM_CF_SHOW,
														TREEVIEW_CF_AUTOSCROLLBAR_V |
														TREEVIEW_CF_AUTOSCROLLBAR_H |
														TREEVIEW_CF_ROWSELECT,
														GUI_ID_TREEVIEW0
													);
	TREEVIEW_SetFont(filesystem, GUI_FONT_24_ASCII);
	scan_files("", &filesystem);
	TREEVIEW_ITEM_ExpandAll(TREEVIEW_GetItem(filesystem, 0, TREEVIEW_GET_FIRST));
	WM_SetCallback(WM_HBKWIN, _cbHBKWIN);
	WM_SetFocus(filesystem);
	taskEXIT_CRITICAL();
	FIL my_file;
	char* str;
	FRESULT res1 = f_open(&my_file, "soutenance.ccdb", FA_READ);
	if (res1 != FR_OK)
	{
		printf("f_open error\n");
	} else {
		// kwerky way to get clean file size
		uint32_t file_size = f_size(&my_file);
		str = malloc(file_size);

		// read the file in a buffer
		f_lseek(&my_file, 0);
		uint32_t bytesread; // TODO check bytesead agains lenght
		//taskENTER_CRITICAL();
		FRESULT res = f_read(&my_file, str, file_size, (UINT*)&bytesread);
		//taskEXIT_CRITICAL();
		str[file_size-1] = '\0';
		f_close(&my_file);
		if (res != FR_OK)
		{
			printf("f_read error\n");
		} else {
			//printf("%s\n", str);
			
			json_settings settings = {};
			settings.value_extra = json_builder_extra;

			char error[128];
			json_value * arr = json_parse_ex(&settings, str, strlen(str), error);

			/* Now serialize it again.
			 
			char * buf = malloc(json_measure(arr));
			json_serialize(buf, arr);

			printf("%s\n", buf);
			*/

			int nb_points = arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.length;

			//printf("%i\n", nb_points);

			for (int i = 0; i < nb_points; i++)
			{
				/*
				printf("[%i,%i,%i]\n",
						(int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[0]->u.integer,
						(int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[1]->u.integer,
						(int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[2]->u.integer
				);
				*/
				led_set((int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[0]->u.integer,
						(int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[1]->u.integer,
						(int) arr->u.object.values[0].value->u.array.values[0]->u.object.values[4].value->u.array.values[i]->u.array.values[2]->u.integer
				);
			}
			
		}
	}

    while(1)
    {
		osDelay(5000);
    }
}

