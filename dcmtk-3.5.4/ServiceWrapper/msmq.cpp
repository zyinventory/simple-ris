#include "stdafx.h"
#include "commonlib.h"
#import <mqoa.dll>

using namespace std;
using namespace MSMQ;

IMSMQQueuePtr OpenOrCreateQueue(const char *queueName, MQACCESS access) throw(...)
{
	HRESULT hr;
	IMSMQQueueInfoPtr pInfo;
	hr = pInfo.CreateInstance(OLESTR("MSMQ.MSMQQueueInfo"));
	if(FAILED(hr)) throw _com_error(hr, NULL);
	if(queueName)
	{
		string qname(queueName);
		if(qname.find(".\\private$\\") != 0)
			qname.insert(0, ".\\private$\\");
		pInfo->PathName = qname.c_str();
	}
	else
		pInfo->PathName = QUEUE_NAME;
	IMSMQQueuePtr pQueue;
	try
	{
		pQueue = pInfo->Open(access, MQ_DENY_NONE);
	}
	catch(_com_error &openerr)
	{
		if(openerr.Error() == MQ_ERROR_QUEUE_NOT_FOUND)
		{
			hr = pInfo->Create();
			if(FAILED(hr)) throw _com_error(hr);
			pQueue = pInfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
		}
		else if(openerr.Error() == MQ_ERROR_ILLEGAL_QUEUE_PATHNAME)
		{
			cerr << "OpenOrCreateQueue error: Queue name: " << queueName << " invalid" << endl;
			throw;
		}
		else
			throw;
	}
	return pQueue;
}

bool SendCommonMessageToCurrentQueue(const char *label, const char *body, const long priority, IMSMQQueuePtr pQueue) throw(...)
{
	HRESULT hr;
	IMSMQMessagePtr pMsg;
	hr = pMsg.CreateInstance(OLESTR("MSMQ.MSMQMessage"));
	if(FAILED(hr)) throw _com_error(hr, NULL);
	pMsg->Delivery = MQMSG_DELIVERY_RECOVERABLE;
	pMsg->Priority = priority;
	pMsg->Label = _bstr_t(label);
	pMsg->Body = _variant_t(body);
	hr = pMsg->Send(pQueue, NULL); //_variant_t notrans = _variant_t(MQ_NO_TRANSACTION);
	if(FAILED(hr)) throw _com_error(hr, NULL);
	return true;
}

bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName)
{
	try
	{
		IMSMQQueuePtr pQueue = OpenOrCreateQueue(queueName, MQ_SEND_ACCESS);
		SendCommonMessageToCurrentQueue(label, body, priority, pQueue);
		return true;
	}
	catch(_com_error &comErr)
	{
		cerr << "SendCommonMessage´íÎó: " << comErr.ErrorMessage() << endl;
		return false;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "SendCommonMessage unknown error: " << ce.ErrorMessage() << endl;
		return false;
	}

}
