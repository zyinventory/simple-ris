#include "stdafx.h"
#include <iterator>
#include <iostream>
#include <functional>
#include <atlcom.h>
#include "commonlib.h"
#import <mqoa.dll>

using namespace std;
using namespace MSMQ;

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

bool RedirectMessageLabelEqualWith(const char *equalWith, const char *queueName)
{
	HRESULT hr;
	char label[MQ_MAX_MSG_LABEL_LEN + 1], body[MQ_MAX_MSG_LABEL_LEN + 1];
	try
	{
		IMSMQQueueInfoPtr pInfo;
		hr = pInfo.CreateInstance(OLESTR("MSMQ.MSMQQueueInfo"));
		if(FAILED(hr)) throw _com_error(hr, NULL);
		string qname(queueName);
		if(qname.find(".\\private$\\") != 0)
			qname.insert(0, ".\\private$\\");
		pInfo->PathName = qname.c_str();
		IMSMQQueuePtr pQueue;
		try
		{
			pQueue = pInfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
		}
		catch(_com_error &openerr)
		{
			if(openerr.Error() == MQ_ERROR_QUEUE_NOT_FOUND)
			{
				hr = pInfo->Create();
				if(FAILED(hr)) throw _com_error(hr);
				pQueue = pInfo->Open(MQ_RECEIVE_ACCESS, MQ_DENY_NONE);
			}
			else
			{
				throw;
			}
		}

		bool result = false;
		VARIANT immediately = { (WORD)VT_I4, (WORD)0, (WORD)0, (WORD)0, 0L }, 
			vtFalse = { (WORD)VT_BOOL, (WORD)0, (WORD)0, (WORD)0, VARIANT_FALSE };
traversal_restart:
		try
		{
			IMSMQMessagePtr pMsg = pQueue->PeekCurrent(&vtMissing, &vtFalse, &immediately);
			while(pMsg)
			{
				string pl(pMsg->Label);
				if(pl.find("archiving") == 0)
				{
					if(pl.compare(equalWith) == 0)
					{
						pMsg = pQueue->ReceiveCurrent();
						copy(pl.begin(), pl.end(), stdext::checked_array_iterator<char*>(label, pl.length()));
						label[pl.length()] = '\0';
						string strbody(_bstr_t(pMsg->Body.bstrVal));
						copy(strbody.begin(), strbody.end(), stdext::checked_array_iterator<char*>(body, MQ_MAX_MSG_LABEL_LEN));
						body[strbody.length()] = '\0';
						string::size_type pos = pl.find(' ');
						if(pos != string::npos)
						{
							string compressedLabel("compressed");
							compressedLabel.append(pl.substr(pos));
							IMSMQQueuePtr pQueueSend = pInfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
							result = SendCommonMessageToCurrentQueue(compressedLabel.c_str(), body, MQ_PRIORITY_COMPRESSED, pQueueSend);
						}
						break;
					}
					else
						pMsg = pQueue->PeekNext(&vtMissing, &vtFalse, &immediately);
				}
				else
					break;
			}
		}
		catch(_com_error &comErr)
		{
			if(comErr.Error() == MQ_ERROR_MESSAGE_ALREADY_RECEIVED)
			{
				pQueue->Reset();
				cout << "RedirectMessage: concurrence of receiving message, restart" << endl;
				goto traversal_restart;
			}
		}
		hr = pQueue->Close();
		if(FAILED(hr)) throw _com_error(hr, NULL);
		return result;
	}
	catch(_com_error &comErr)
	{
		cerr << "RedirectMessage: " << comErr.ErrorMessage() << endl;
		return false;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "RedirectMessage: unknown error: " << ce.ErrorMessage() << endl;
		return false;
	}
}

bool SendCommonMessageToQueue(const char *label, const char *body, const long priority, const char *queueName)
{
	HRESULT hr;
	try
	{
		IMSMQQueueInfoPtr pInfo;
		hr = pInfo.CreateInstance(OLESTR("MSMQ.MSMQQueueInfo"));
		if(FAILED(hr)) throw _com_error(hr, NULL);
		string qname(queueName);
		if(qname.find(".\\private$\\") != 0)
			qname.insert(0, ".\\private$\\");
		pInfo->PathName = qname.c_str();
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
			else if(openerr.Error() == MQ_ERROR_ILLEGAL_QUEUE_PATHNAME)
			{
				cerr << "SendCommonMessage´íÎó: Queue name: " << queueName << " invalid" << endl;
				throw;
			}
			else
				throw;
		}
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
