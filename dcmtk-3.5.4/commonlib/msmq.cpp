#include "stdafx.h"
#include "commonlib.h"
#import <mqoa.dll>

using namespace std;
using namespace MSMQ;

static IMSMQQueuePtr OpenOrCreateQueue(const char *queueName) throw(...)
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
			cerr << "OpenOrCreateQueue error: Queue name: " << queueName << " invalid" << endl;
			throw;
		}
		else
			throw;
	}
	return pQueue;
}

COMMONLIB_API bool SendArchiveMessageToQueue(const char *label, const char *body, const char *cmd)
{
	HRESULT hr;
	try
	{
		string bodyXml(body);
		string::size_type p;
		if(strcmp(label, ARCHIVE_STUDY) && strcmp(label, ARCHIVE_STUDY_NOT_INTEGRITY))
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
				cerr << "Send Archive Image Message: command place holder not found:" << endl;
				cerr << "label: " << label << endl;
				cerr << "body: " << body << endl;
				cerr << "cmd: " << cmd << endl;
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
				cerr << "Send Archive Study Message: command place holder not found:" << endl;
				cerr << "label: " << label << endl;
				cerr << "body: " << body << endl;
				cerr << "cmd: " << cmd << endl;
				return false;
			}
		}

		IMSMQQueuePtr pQueue = OpenOrCreateQueue(NULL);
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

COMMONLIB_API bool DeleteQueue(const char *queueName)
{
	bool queueExist = false;
	try
	{
		IMSMQQueueInfoPtr pInfo;
		HRESULT hr = pInfo.CreateInstance(OLESTR("MSMQ.MSMQQueueInfo"));
		if(FAILED(hr)) throw _com_error(hr, NULL);
		if(queueName)
		{
			string qname(queueName);
			if(qname.find(".\\private$\\") != 0)
				qname.insert(0, ".\\private$\\");
			pInfo->PathName = qname.c_str();
			IMSMQQueuePtr pQueue = pInfo->Open(MQ_PEEK_ACCESS, MQ_DENY_RECEIVE_SHARE);
			if(pQueue)
			{
				queueExist = true;
				VARIANT timeout = { (WORD)VT_I4, (WORD)0, (WORD)0, (WORD)0, 0L }, 
					vtFalse = { (WORD)VT_BOOL, (WORD)0, (WORD)0, (WORD)0, VARIANT_FALSE };
				IMSMQMessagePtr pMsg = pQueue->PeekCurrent(&vtMissing, &vtFalse, &timeout);
				if(pMsg)
				{
					cerr << "DeleteQueue " << qname << " error, message remain: " << (LPCSTR)pMsg->Label << endl;
					pQueue->Close();
				}
				else
				{
					pQueue->Close();
					queueExist = FAILED(pInfo->Delete());
				}
			}
			return queueExist;
		}
		else
		{
			cerr << "DeleteQueue error£ºqueue name is NULL" << endl;
			return queueExist;
		}
	}
	catch(_com_error &comErr)
	{
		cerr << "DeleteQueue error£º" << comErr.ErrorMessage() << endl;
		return queueExist;
	}
	catch(...)
	{
		_com_error ce(AtlHresultFromLastError());
		cerr << "DeleteQueue unknown error: " << ce.ErrorMessage() << endl;
		return queueExist;
	}
}
