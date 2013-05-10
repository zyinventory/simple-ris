#include "stdafx.h"
#include <iostream>
#include <atlcom.h>
#import <mqoa.dll>

using namespace std;
using namespace MSMQ;

#define QUEUE_NAME OLESTR(".\\private$\\archive")
#define CMD_PLACE_HOLDER "%cmd%"

bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd)
{
	HRESULT hr;
	try
	{
		string bodyXml(body);
		string::size_type p = bodyXml.find(CMD_PLACE_HOLDER);
		if(p != string::npos) bodyXml.replace(p, strnlen_s(CMD_PLACE_HOLDER, sizeof(CMD_PLACE_HOLDER)), cmd);
		IMSMQQueueInfoPtr pInfo;
		hr = pInfo.CreateInstance(OLESTR("MSMQ.MSMQQueueInfo"));
		if(FAILED(hr)) throw _com_error(hr, NULL);
		pInfo->PathName = QUEUE_NAME;
		IMSMQQueuePtr pQueue;
		try
		{
			pQueue = pInfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
		}
		catch(_com_error &openerr)
		{
			if(openerr.Error() == MQ_ERROR_QUEUE_NOT_FOUND)
			{
				hr = pInfo->Create();
				if(FAILED(hr)) throw _com_error(hr);
				pQueue = pInfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
			}
			else
			{
				throw;
			}
		}

		IMSMQMessagePtr pMsg;
		hr = pMsg.CreateInstance(OLESTR("MSMQ.MSMQMessage"));
		if(FAILED(hr)) throw _com_error(hr, NULL);
		pMsg->Delivery = MQMSG_DELIVERY_RECOVERABLE;
		pMsg->Priority = 3;
		pMsg->Label = _bstr_t(label);
		pMsg->Body = _variant_t(bodyXml.c_str());
		hr = pMsg->Send(pQueue, NULL); //_variant_t notrans = _variant_t(MQ_NO_TRANSACTION);
		if(FAILED(hr)) throw _com_error(hr, NULL);
		hr = pQueue->Close();
		if(FAILED(hr)) throw _com_error(hr, NULL);
		return true;
	}
	catch(_com_error &comErr)
	{
		cerr << "´íÎó£º" << comErr.ErrorMessage() << endl;
		return false;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "Unknown error: " << ce.ErrorMessage() << endl;
		return false;
	}
}