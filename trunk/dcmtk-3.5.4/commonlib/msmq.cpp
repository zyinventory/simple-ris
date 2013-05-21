#include "stdafx.h"
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

bool RedirectMessageLabelEqualWith(char *label, char *body, const int bodyLength, const char *equalWith, const char *queueName)
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
		IMSMQMessagePtr pMsg = pQueue->PeekCurrent();
		do
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
					copy(strbody.begin(), strbody.end(), stdext::checked_array_iterator<char*>(body, bodyLength));
					body[strbody.length()] = '\0';
					string::size_type pos = pl.find(' ');
					if(pos != string::npos)
					{
						string compressedLabel("compressed");
						compressedLabel.append(pl.substr(pos));
						IMSMQQueuePtr pQueueSend = pInfo->Open(MQ_SEND_ACCESS, MQ_DENY_NONE);
						result = SendCommonMessageToCurrentQueue(compressedLabel.c_str(), body, 4, pQueueSend);
					}
					break;
				}
			}
			else
				break;
		} while(pMsg = pQueue->PeekNext());
		hr = pQueue->Close();
		if(FAILED(hr)) throw _com_error(hr, NULL);
		return result;
	}
	catch(_com_error &comErr)
	{
		cerr << "RedirectMessage´íÎó£º" << comErr.ErrorMessage() << endl;
		return false;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "RedirectMessage unknown error: " << ce.ErrorMessage() << endl;
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

bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd)
{
	HRESULT hr;
	try
	{
		string bodyXml(body);
		string::size_type p;
		if(strcmp(label, ARCHIVE_STUDY))
		{
			if( string::npos != (p = bodyXml.find(REPLACE_PLACE_HOLDER)) ) // uncompressed file, call dcmcjpeg
				bodyXml.replace(p, strnlen_s(REPLACE_PLACE_HOLDER, sizeof(REPLACE_PLACE_HOLDER)), cmd);
			else if(string::npos != (p = bodyXml.find(MOVE_PLACE_HOLDER))) // compressed file, move only
			{
				string command(cmd);
				string::size_type sp = string::npos;
				if(string::npos != (sp = command.rfind(' ', sp)))
				{
					--sp;
					sp = command.rfind(' ', sp);
				}
				if(string::npos != sp) // find src_file and dest_file OK, construct command: move src_file dest_file
				{
					string iofile("move");
					iofile.append(command.substr(sp));
					bodyXml.replace(p, strnlen_s(MOVE_PLACE_HOLDER, sizeof(MOVE_PLACE_HOLDER)), iofile.c_str());
				}
				else  // find src_file and dest_file failed, call cmd directly
					bodyXml.replace(p, sizeof(MOVE_PLACE_HOLDER), cmd);
			}
			else
			{
				cerr << "SendArchiveMessage: command place holder not found:" << endl;
				cerr << body << endl;
				return false;
			}
		}
		else
		{
			if( string::npos != (p = bodyXml.find(REPLACE_PLACE_HOLDER)) )
				bodyXml.replace(p, strnlen_s(REPLACE_PLACE_HOLDER, sizeof(REPLACE_PLACE_HOLDER)), cmd);
			else if( string::npos != (p = bodyXml.find(MOVE_PLACE_HOLDER)) )
				bodyXml.replace(p, strnlen_s(MOVE_PLACE_HOLDER, sizeof(MOVE_PLACE_HOLDER)), cmd);
			else
			{
				cerr << "SendArchiveMessage: command place holder not found:" << endl;
				cerr << body << endl;
				return false;
			}
		}

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
		cerr << "SendArchiveMessage´íÎó£º" << comErr.ErrorMessage() << endl;
		return false;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "SendArchiveMessage unknown error: " << ce.ErrorMessage() << endl;
		return false;
	}
}