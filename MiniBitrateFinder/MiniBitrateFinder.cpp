// MiniBitratePatcher by Pulsarz @ dashcamtalk.com
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>

float fourBytesToFloat(unsigned char c3, unsigned char c2, unsigned char c1, unsigned char c0)
{
	union {
		float f;
		unsigned char b[4];
	} u;

	u.b[3] = c3;
	u.b[2] = c2;
	u.b[1] = c1;
	u.b[0] = c0;

	return u.f;
}

int main()
{
	FILE *memdump = NULL;
	FILE *autoexec = NULL;
	OPENFILENAME sfn, ofn;
	wchar_t autoexecFileName[MAX_PATH] = L"";
	wchar_t memdumpFileName[MAX_PATH] = L"";

	ZeroMemory(&ofn, sizeof(ofn));
	ZeroMemory(&sfn, sizeof(sfn));

	//Open dialog
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = memdumpFileName;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"BIN (*.bin)\0*.bin\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
	GetOpenFileName(&ofn);
	wprintf(L"Memdump path : %s\n", memdumpFileName);

	//Save dialog

	sfn.lStructSize = sizeof(sfn);
	sfn.hwndOwner = NULL;
	sfn.lpstrFilter = L"Autoexec (*.ash)\0*.ash\0All Files (*.*)\0*.*\0";
	sfn.lpstrFile = autoexecFileName;
	sfn.nMaxFile = MAX_PATH;
	sfn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	sfn.lpstrDefExt = L"ash";

	GetSaveFileName(&sfn);
	wprintf(L"Autoexec.ash path : %s\n", autoexecFileName);

	_wfopen_s(&autoexec, autoexecFileName, L"wb+");
	fseek(autoexec, 0, SEEK_SET);


	//Code

	_wfopen_s(&memdump, memdumpFileName, L"rb");

	fseek(memdump, 0, SEEK_END);
	long fsize = ftell(memdump);
	fseek(memdump, 0, SEEK_SET);

	unsigned char *buffer = (unsigned char*)malloc(fsize);
	fread(buffer, fsize, 1, memdump);
	fclose(memdump);


	//We search for the standard VBR quaity options in mini0806 to find FW table
	for (int i = 0; i < fsize - 7; i++)
	{
		if (buffer[i] == 0x00 && buffer[i + 1] == 0x00 && buffer[i + 2] == 0x40 && buffer[i + 3] == 0x3F)//VBR_MIN 0.75
		{
			if (buffer[i + 4] == 0x00 && buffer[i + 5] == 0x00 && buffer[i + 6] == 0xA0 && buffer[i + 7] == 0x3F)//VBR_MAX 1.25
			{
				//printf("FOUND! Adress: 0x%08x \r\n", i);
				
				float bitrate = fourBytesToFloat(buffer[i - 1], buffer[i - 2], buffer[i - 3], buffer[i - 4]);
				float VBR_MIN = fourBytesToFloat(buffer[i + 3], buffer[i + 2], buffer[i + 1], buffer[i + 0]);
				float VBR_MAX = fourBytesToFloat(buffer[i + 7], buffer[i + 6], buffer[i + 5], buffer[i + 4]);

				printf("0x%08x Bitrate:%2.2f    VBRR_MIN:%2.2f    VBR_MAX:%2.2f", i-8+0xC0000000, bitrate, VBR_MIN, VBR_MAX);

				//Patch all 18MBit +-25% VBR to 24Mbit +-0% VBR
				if (bitrate == 18.0 && VBR_MIN == 0.75 && VBR_MAX == 1.25)
				{
					//Patch bitrate
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 - 2, 0x41c0);
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 - 4, 0x0000);
					

					//Patch VBR_MIN
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 + 2, 0x3f80);
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 + 0, 0x0000);

					//Patch VBR_MAX
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 + 6, 0x3f80);
					fprintf_s(autoexec, "writew 0x%08x 0x%04x\n", i + 0xC0000000 + 4, 0x0000);

					printf("  ... Patched!");
				}

				printf("\r\n");
			}

			
		}
		
	}

	fprintf_s(autoexec, "\n");//Empty line needed at end of file
	fclose(autoexec);

	printf("Press any key to exit ...");
	getchar();
	
    return 0;
}

