#pragma once

void generateImageStoreDirectory(DcmDataset **imageDataSet, OFString& subdirectoryName, OFString& imageManageNumber);
bool insertImage(DcmDataset *imageDataSet, OFString& imageManageNumber, OFString& outputDirectory, OFString& relateFilePathName);