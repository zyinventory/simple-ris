// getindex.c : 定义控制台应用程序的入口点。
//
#include <stdlib.h>
#include "cgic.h"

#define SAVED_ENVIRONMENT "D:\\share\\capcgi.dat"

void LoadEnvironment()
{
	if (cgiReadEnvironment(SAVED_ENVIRONMENT) != 
		cgiEnvironmentSuccess) 
	{
		cgiHeaderContentType("text/html");
		fprintf(cgiOut, "<head>Error</head>\n");
		fprintf(cgiOut, "<body><h1>Error</h1>\n");
		fprintf(cgiOut, "cgiReadEnvironment failed. Most "
			"likely you have not saved an environment "
			"yet.\n");
		exit(0);
	}
	/* OK, return now and show the results of the saved environment */
}

extern int cppWrapper();

int cgiMain()
{
#ifdef _DEBUG
	LoadEnvironment();
#endif /* DEBUG */
	return cppWrapper();
}
