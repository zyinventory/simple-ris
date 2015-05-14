//#include <cstring>
//#include <cstdlib>
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"     /* for dcmtk version name */
#include "dcmtk/dcmjpeg/djdecode.h"  /* for dcmjpeg decoders */
#include "dcmtk/dcmjpeg/djencode.h"  /* for dcmjpeg encoders */
#include "dcmtk/dcmjpeg/djdec2k.h"  /* for j2k decoders */
#include "dcmtk/dcmjpeg/djenc2k.h"  /* for j2k encoders */
#include "dcmtk/dcmjpeg/djrplol.h"   /* for DJ_RPLossless */
#include "dcmtk/dcmjpeg/djrploss.h"  /* for DJ_RPLossy */
#include "dcmtk/dcmjpeg/dipijpeg.h"  /* for dcmimage JPEG plugin */
#include "dcmtk/dcmimage/diregist.h"  /* include to support color images */


int main(int argc, char *argv[])
{
  DJDecoderRegistration::registerCodecs(); // register JPEG codecs
  DJEncoderRegistration::registerCodecs(); // register JPEG codecs
  DcmFileFormat fileformat;
  std::cout << "Loading file: " << argv[1] << std::endl;
  if (fileformat.loadFile(argv[1]).good())
  {
    DcmDataset *dataset = fileformat.getDataset();
    DcmItem *metaInfo = fileformat.getMetaInfo();
    E_TransferSyntax ts = EXS_JPEG2000LosslessOnly;
    
    // this causes the JPEG2000 version of the dataset to be created
    dataset->chooseRepresentation(EXS_JPEG2000LosslessOnly, NULL);
    // check if everything went well
    if (dataset->canWriteXfer(EXS_JPEG2000LosslessOnly))
    {
      // force the meta-header UIDs to be re-generated when storing the file
      // since the UIDs in the data set may have changed
      delete metaInfo->remove(DCM_MediaStorageSOPClassUID);
      delete metaInfo->remove(DCM_MediaStorageSOPInstanceUID);
      // store in lossless JPEG format
      fileformat.saveFile(argv[2], EXS_JPEG2000LosslessOnly);
    }
  }

  DJDecoderRegistration::cleanup(); // deregister JPEG codecs
  DJEncoderRegistration::cleanup(); // deregister JPEG codecs
}
