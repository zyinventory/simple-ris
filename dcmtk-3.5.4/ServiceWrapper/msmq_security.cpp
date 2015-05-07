#include <iostream>
#include <atlbase.h>
#include <Mq.h>

using namespace std;

HRESULT ModifyDaclInfo(PSECURITY_DESCRIPTOR pSecurityDescriptor, LPCWSTR wszFormatName)
{
	// Validate the input parameters.
	if (pSecurityDescriptor == NULL || wszFormatName == NULL)
	{
		return MQ_ERROR_INVALID_PARAMETER;
	}

	PSID pEveryoneSid = NULL;
	DWORD dwSidSize = 0;
	PACL pDacl = NULL;
	ACL_SIZE_INFORMATION aclsizeinfo;
	ACCESS_ALLOWED_ACE * pOldAce = NULL;
	ACCESS_ALLOWED_ACE * pNewAce = NULL;
	DWORD cAce;
	SECURITY_DESCRIPTOR sdNew;
	DWORD dwErrorCode = 0;
	HRESULT hr = MQ_OK;


	// Obtain the SID of the Everyone group.
	SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
	if (AllocateAndInitializeSid(
			&WorldAuth,          // Top-level SID authority
			1,                   // Number of subauthorities
			SECURITY_WORLD_RID,  // Subauthority value
			0, 0, 0, 0, 0, 0, 0,
			&pEveryoneSid        // SID returned as OUT parameter
			) == FALSE)
	{
		dwErrorCode = GetLastError();
		cerr << "AllocateAndInitializeSid failed. GetLastError returned: " << dwErrorCode << endl;
		return HRESULT_FROM_WIN32(dwErrorCode);
	}


	// Retrieve the DACL from the security descriptor buffer.
	BOOL fDaclPresent = FALSE;
	BOOL fDaclDefaulted = TRUE;
	if (GetSecurityDescriptorDacl(pSecurityDescriptor, &fDaclPresent, &pDacl, &fDaclDefaulted) == FALSE)
	{
		dwErrorCode = GetLastError();
		cerr << "GetSecurityDescriptorDacl failed. GetLastError returned: " << dwErrorCode << endl;
		return HRESULT_FROM_WIN32(dwErrorCode);
	}

	// Check whether no DACL or a NULL DACL was retrieved from the security descriptor buffer.
	if ((fDaclPresent == FALSE) || (pDacl == NULL))
	{
		cerr << "No DACL was found (all access is denied), or a NULL DACL (unrestricted access) was found." << endl;
		return MQ_OK;
	}

	// Retrieve the ACL_SIZE_INFORMATION structure containing the number of ACEs in the DACL.
	if (GetAclInformation(pDacl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation) == FALSE)
	{
		dwErrorCode = GetLastError();
		cerr << "GetAclInformation failed. GetLastError returned: " << dwErrorCode << endl;
		return HRESULT_FROM_WIN32(dwErrorCode);
	}

	// Loop through the ACEs to find the ACE for the Everyone group.
	for (cAce = 0; cAce < aclsizeinfo.AceCount && hr == MQ_OK; cAce++)
	{
		// Retrieve the security information in the ACE.
		if (GetAce(pDacl,            // Pointer to the DACL
				cAce,                // Index of the ACE in the DACL
				(LPVOID*)&pOldAce    // Pointer to an ACE structure
				) == FALSE)
		{
			cerr << "GetAce failed. GetLastError returned: " << GetLastError() << endl;
			continue;
		}

		// Compare the SID in the ACE with the SID of the Everyone group.
		if (EqualSid((PSID)&pOldAce->SidStart, pEveryoneSid))
		{
			// Allocate memory for a new ACE, set the values of 
			// Header.AceType and Header.AceSize in the new ACE,
			// and copy the modified mask and the SID to the new ACE.
			dwSidSize = GetLengthSid((PSID)&pOldAce->SidStart);
			DWORD dwAceSize = sizeof(ACCESS_ALLOWED_ACE) + dwSidSize - sizeof(DWORD);
			pNewAce = (ACCESS_ALLOWED_ACE*) new BYTE[dwAceSize];
			memset(pNewAce, 0, dwAceSize);
			pNewAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
			pNewAce->Header.AceSize = (WORD)dwAceSize;
			pNewAce->Mask = pOldAce->Mask | MQSEC_QUEUE_GENERIC_ALL;
			if (CopySid(dwSidSize, (PSID)&pNewAce->SidStart, (PSID)&pOldAce->SidStart) == FALSE)
			{
				dwErrorCode = GetLastError();
				cerr << "CopySid failed. GetLastError returned: " << dwErrorCode << endl;
				hr = HRESULT_FROM_WIN32(dwErrorCode);
				break;
			}

			// Delete the old ACE from the DACL.
			if (DeleteAce(pDacl,  // Pointer to the DACL
					cAce          // Index of the ACE in the DACL
					) == FALSE)
			{
				dwErrorCode = GetLastError();
				cerr << "DeleteAce failed. GetLastError returned: " << dwErrorCode << endl;
				hr = HRESULT_FROM_WIN32(dwErrorCode);
				break;
			}

			// Insert the modified ACE into the DACL.
			if (AddAce(pDacl,                 // Pointer to the DACL
					ACL_REVISION,             // For ACCESS_ALLOWED_ACE_TYPE
					cAce,                     // Index of the ACE
					pNewAce,                  // Pointer to the new ACE
					dwAceSize                 // Length of the new ACE
					) == FALSE)
			{
				dwErrorCode = GetLastError();
				cerr << "AddAce failed. GetLastError returned: " << dwErrorCode << endl;
				hr = HRESULT_FROM_WIN32(dwErrorCode);
				break;
			}

			// Initialize a new absolute SECURITY_DESCRIPTOR structure.
			if (InitializeSecurityDescriptor(&sdNew, SECURITY_DESCRIPTOR_REVISION  // Required constant
				) == FALSE)
			{
				dwErrorCode = GetLastError();
				cerr << "InitializeSecurityDescriptor failed. GetLastError returned: " << dwErrorCode << endl;
				hr = HRESULT_FROM_WIN32(dwErrorCode);
				break;
			}

			// Insert the modified DACL into the new absolute
			// SECURITY_DESCRIPTOR structure.
			if (SetSecurityDescriptorDacl(&sdNew, TRUE, pDacl, FALSE) == FALSE)
			{
				dwErrorCode = GetLastError();
				cerr << "SetSecurityDescriptorDacl failed. GetLastError returned: " << dwErrorCode << endl;
				hr = HRESULT_FROM_WIN32(dwErrorCode);
				break;
			}

			// Use the absolute SECURITY_DESCRIPTOR structure containing the modified
			// DACL to set the DACL in the queue's security descriptor.
			hr = MQSetQueueSecurity(wszFormatName, DACL_SECURITY_INFORMATION, &sdNew);
			if (FAILED(hr)) cerr <<"The call to MQSetQueueSecurity failed. Error code: 0x" << hex << hr << endl;
			break;
		}
	}
	// Free the memory allocated for buffers.
	delete [] pNewAce;
	FreeSid(pEveryoneSid);
	return hr;
}

HRESULT QLetEveryoneFullControl(LPCWSTR wszFormatNameBuffer)
{
	// Define structures and variables to retrieve security information.
	PSECURITY_DESCRIPTOR pSecurityDescriptor = NULL;  // Pointer to the security descriptor buffer
	DWORD dwBufferLength = 1;
	DWORD dwBufferLengthNeeded = 1;
	HRESULT hr;                                       // Define results

	// Retrieve the DACL from the queue's security descriptor.
	for ( ; ; )
	{
		pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new byte[dwBufferLength];
		hr = MQGetQueueSecurity(wszFormatNameBuffer, 
			DACL_SECURITY_INFORMATION,  // Retrieving only the DACL
			pSecurityDescriptor, dwBufferLength, &dwBufferLengthNeeded);
		if (SUCCEEDED(hr))
		{
			break;
		}
		if (hr == MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL)
		{

			// Allocate the memory needed for the security descriptor buffer.
			delete [] pSecurityDescriptor;
			dwBufferLength = dwBufferLengthNeeded;
			pSecurityDescriptor = (PSECURITY_DESCRIPTOR) new byte[dwBufferLength];
			if(pSecurityDescriptor == NULL)
			{
				cerr << "Memory could not be allocated for the security descriptor buffer." << endl;
				return MQ_ERROR_INSUFFICIENT_RESOURCES;
			}
			memset(pSecurityDescriptor, 0, dwBufferLength);
			continue;
		}
		cerr << "The call to MQGetQueueSecurity failed. Error code: 0x" << hex << hr << endl;
		delete [] pSecurityDescriptor;
		return hr;
	}

	hr = ModifyDaclInfo(pSecurityDescriptor, wszFormatNameBuffer);

	if (FAILED(hr))
		cerr << "ModifyDaclInfo failed. Error code: 0x" << hex << hr << endl;
	else
		hr = MQ_OK;

	//Free the memory allocated for the security descriptor buffer.
	delete [] pSecurityDescriptor;
	return hr;
}
