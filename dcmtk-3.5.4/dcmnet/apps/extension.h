#pragma once
void rollbackDB();
bool generateImageStoreDirectory(DcmDataset **imageDataSet, OFString& subdirectoryName, OFString& imageManageNumber);
bool insertImage(DcmDataset *imageDataSet, OFString& imageManageNumber, OFString& outputDirectory, OFString& relateFilePathName, OFString& volumeLabel, OFBool needCompress);