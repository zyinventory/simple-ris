#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h"
#include "dcmtk/ofstd/ofstd.h"
#include "dcmtk/ofstd/ofconsol.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/ofstd/ofstring.h"
#ifdef HAVE_WINDOWS_H
#include <direct.h>        /* for _mkdir() */
#endif

OFCondition MkdirRecursive(OFString& subdirectoryPath)
{
  // check if the subdirectory is already existent
  if( OFStandard::dirExists(subdirectoryPath) )
  {
    return EC_Normal;
  }
  else
  {
    size_t position = subdirectoryPath.rfind(PATH_SEPARATOR);
    if(position != OFString_npos)
    {
      OFString upperLevel = subdirectoryPath.substr(0, position);
      OFCondition mkResult = MkdirRecursive(upperLevel);
      if(mkResult != EC_Normal)
      {
        return mkResult;
      }
      // else: upper level exist, create current level
    }

    // if it is not existent create it
    #ifdef HAVE_WINDOWS_H
    if( _mkdir( subdirectoryPath.c_str() ) == -1 )
    #else
    if( mkdir( subdirectoryPathAndName.c_str(), S_IRWXU | S_IRWXG | S_IRWXO ) == -1 )
    #endif
    {
	  CERR << "Could not create subdirectory " << subdirectoryPath.c_str() << endl;
      return EC_IllegalParameter;
    }
    else
    {
      return EC_Normal;
    }
  }
}
